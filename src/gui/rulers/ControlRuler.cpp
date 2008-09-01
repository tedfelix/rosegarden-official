/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include <Q3Canvas>
#include <Q3CanvasItem>
#include <Q3CanvasItemList>
#include <Q3CanvasRectangle>
#include <Q3CanvasView>
#include "ControlRuler.h"

#include "base/Event.h"
#include "misc/Debug.h"
#include "base/RulerScale.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "ControlChangeCommand.h"
#include "ControlItem.h"
#include "ControlSelector.h"
#include "ControlTool.h"
#include "DefaultVelocityColour.h"
#include "ElementAdapter.h"
#include "gui/general/EditView.h"
#include "gui/general/RosegardenCanvasView.h"
#include "gui/widgets/TextFloat.h"
#include <kmainwindow.h>
#include <qcanvas.h>
#include <QColor>
#include <QCursor>
#include <QPoint>
#include <qpopupmenu.h>
#include <QScrollBar>
#include <qscrollview.h>
#include <QString>
#include <QWidget>
#include <algorithm>


namespace Rosegarden
{

const int ControlRuler::DefaultRulerHeight = 75;
const int ControlRuler::MinItemHeight = 5;
const int ControlRuler::MaxItemHeight = 64 + 5;
const int ControlRuler::ItemHeightRange = 64;

ControlRuler::ControlRuler(Segment *segment,
                           RulerScale* rulerScale,
                           EditViewBase* parentView,
                           Q3Canvas* c, QWidget* parent,
                           const char* name, WFlags f) :
        RosegardenCanvasView(c, parent, name, f),
        m_parentEditView(parentView),
        m_mainHorizontalScrollBar(0),
        m_rulerScale(rulerScale),
        m_eventSelection(new EventSelection(*segment)),
        m_segment(segment),
        m_currentIndex(0),
        m_tool(0),
        m_maxItemValue(127),
        m_staffOffset(0),
        m_currentX(0.0),
        m_itemMoved(false),
        m_selecting(false),
        m_selector(new ControlSelector(this)),
        m_selectionRect(new Q3CanvasRectangle(canvas())),
        m_menu(0)
{
    setHScrollBarMode(QScrollView::AlwaysOff);

    m_selectionRect->setPen(QColor(Qt::red));

    setFixedHeight(sizeHint().height());

    connect(this, SIGNAL(stateChange(const QString&, bool)),
            m_parentEditView, SLOT(slotStateChanged(const QString&, bool)));

    m_numberFloat = new TextFloat(this);
    m_numberFloat->hide();

    m_segment->addObserver(this);

    emit stateChange("have_controller_item_selected", false);
}

ControlRuler::~ControlRuler()
{
    if (m_segment) {
        m_segment->removeObserver(this);
    }
}

void ControlRuler::slotUpdate()
{
    RG_DEBUG << "ControlRuler::slotUpdate()\n";

    canvas()->setAllChanged(); // TODO: be a bit more subtle, call setChanged(<time area>)

    canvas()->update();
    repaint();
}

void ControlRuler::slotUpdateElementsHPos()
{
    computeStaffOffset();

    Q3CanvasItemList list = canvas()->allItems();

    Q3CanvasItemList::Iterator it = list.begin();
    for (; it != list.end(); ++it) {
        ControlItem* item = dynamic_cast<ControlItem*>(*it);
        if (!item)
            continue;
        layoutItem(item);
    }

    canvas()->update();
}

void ControlRuler::layoutItem(ControlItem* item)
{
    timeT itemTime = item->getElementAdapter()->getTime();

    double x = m_rulerScale->getXForTime(itemTime);

    item->setX(x + m_staffOffset);
    int itemElementDuration = item->getElementAdapter()->getDuration();

    int width = int(m_rulerScale->getXForTime(itemTime + itemElementDuration) - x);

    item->setWidth(width);

    //     RG_DEBUG << "ControlRuler::layoutItem ControlItem x = " << x << " - width = " << width << endl;
}

void ControlRuler::setControlTool(ControlTool* tool)
{
    if (m_tool)
        delete m_tool;
    m_tool = tool;
}

void
ControlRuler::segmentDeleted(const Segment *)
{
    m_segment = 0;
}

void ControlRuler::contentsMousePressEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton) {
        m_numberFloat->hide();
        m_selecting = false;
        return ;
    }

    RG_DEBUG << "ControlRuler::contentsMousePressEvent()\n";

    QPoint p = inverseMapPoint(e->pos());

    Q3CanvasItemList l = canvas()->collisions(p);

    if (l.count() == 0) { // de-select current item
        clearSelectedItems();
        m_selecting = true;
        m_selector->handleMouseButtonPress(e);
        RG_DEBUG << "ControlRuler::contentsMousePressEvent : entering selection mode\n";
        return ;
    }

    // clear selection unless control was pressed, in which case
    // add the event to the current selection
    if (!(e->state() && QMouseEvent::ControlButton)) {
        clearSelectedItems();
    }

    ControlItem *topItem = 0;
    for (Q3CanvasItemList::Iterator it = l.begin(); it != l.end(); ++it) {

        if (ControlItem *item = dynamic_cast<ControlItem*>(*it)) {

            if (topItem == 0)
                topItem = item;

            if (item->isSelected()) { // if the item which was clicked
                // on is part of a selection,
                // propagate mousepress on all
                // selected items

                item->handleMouseButtonPress(e);

                for (Q3CanvasItemList::Iterator it = m_selectedItems.begin();
                        it != m_selectedItems.end(); ++it) {
                    if (ControlItem *selectedItem =
                                dynamic_cast<ControlItem*>(*it)) {
                        selectedItem->handleMouseButtonPress(e);
                    }
                }


            } else { // select it

                if (!(e->state() && QMouseEvent::ControlButton)) {
                    if (item->z() > topItem->z())
                        topItem = item;

                } else {
                    m_selectedItems << item;
                    item->setSelected(true);
                    item->handleMouseButtonPress(e);
                    ElementAdapter* adapter = item->getElementAdapter();
                    m_eventSelection->addEvent(adapter->getEvent());
                }
            }
        }
    }

    if (topItem && !m_selectedItems.contains(topItem)) { // select the top item
        m_selectedItems << topItem;
        topItem->setSelected(true);
        topItem->handleMouseButtonPress(e);
        ElementAdapter* adapter = topItem->getElementAdapter();
        m_eventSelection->addEvent(adapter->getEvent());
    }

    m_itemMoved = false;
    m_lastEventPos = p;
}

void ControlRuler::contentsMouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton) {
        m_numberFloat->hide();
        m_selecting = false;
        return ;
    }

    if (m_selecting) {
        updateSelection();
        m_selector->handleMouseButtonRelease(e);
        RG_DEBUG << "ControlRuler::contentsMouseReleaseEvent : leaving selection mode\n";
        m_selecting = false;
        return ;
    }

    for (Q3CanvasItemList::Iterator it = m_selectedItems.begin(); it != m_selectedItems.end(); ++it) {
        if (ControlItem *item = dynamic_cast<ControlItem*>(*it)) {

            ElementAdapter * adapter = item->getElementAdapter();
            m_eventSelection->addEvent(adapter->getEvent());
            item->handleMouseButtonRelease(e);
        }
    }

    emit stateChange("have_controller_item_selected", true);

    if (m_itemMoved) {

        m_lastEventPos = inverseMapPoint(e->pos());

        // Add command to history
        ControlChangeCommand* command = new ControlChangeCommand(m_selectedItems,
                                        *m_segment,
                                        m_eventSelection->getStartTime(),
                                        m_eventSelection->getEndTime());

        RG_DEBUG << "ControlRuler::contentsMouseReleaseEvent : adding command\n";
        m_parentEditView->addCommandToHistory(command);

        m_itemMoved = false;
    }

    m_numberFloat->hide();
}

void ControlRuler::contentsMouseMoveEvent(QMouseEvent* e)
{
    QPoint p = inverseMapPoint(e->pos());

    int deltaX = p.x() - m_lastEventPos.x(),
                 deltaY = p.y() - m_lastEventPos.y();
    m_lastEventPos = p;

    if (m_selecting) {
        updateSelection();
        m_selector->handleMouseMove(e, deltaX, deltaY);
        slotScrollHorizSmallSteps(p.x());
        return ;
    }

    m_itemMoved = true;

    // Borrowed from Rotary - compute total position within window
    //
    QPoint totalPos = mapTo(topLevelWidget(), QPoint(0, 0));

    int scrollX = dynamic_cast<EditView*>(m_parentEditView)->getRawCanvasView()->
                  horizontalScrollBar()->value();

    /*
    RG_DEBUG << "ControlRuler::contentsMouseMoveEvent - total pos = " << totalPos.x()
             << ",e pos = " << e->pos().x()
             << ", scroll bar = " << scrollX
             << endl;
             */

    // Allow for scrollbar
    //
    m_numberFloat->move(totalPos.x() + e->pos().x() - scrollX + 20,
                        totalPos.y() + e->pos().y() - 10);

    int value = 0;

    for (Q3CanvasItemList::Iterator it = m_selectedItems.begin(); it != m_selectedItems.end(); ++it) {
        if (ControlItem *item = dynamic_cast<ControlItem*>(*it)) {
            item->handleMouseMove(e, deltaX, deltaY);
            //            ElementAdapter* adapter = item->getElementAdapter();

            // set value to highest in selection
            if (item->getValue() >= value) {
                value = item->getValue();
                m_numberFloat->setText(QString("%1").arg(value));
            }
        }
    }
    canvas()->update();

    m_numberFloat->show();

}

void
ControlRuler::contentsWheelEvent(QWheelEvent *e)
{
    // not sure what to do yet
    Q3CanvasView::contentsWheelEvent(e);
}

void ControlRuler::updateSelection()
{
    clearSelectedItems();

    bool haveSelectedItems = false;

    Q3CanvasItemList l = getSelectionRectangle()->collisions(true);

    for (Q3CanvasItemList::Iterator it = l.begin(); it != l.end(); ++it) {

        if (ControlItem *item = dynamic_cast<ControlItem*>(*it)) {
            item->setSelected(true);
            m_selectedItems << item;
            haveSelectedItems = true;

            ElementAdapter* adapter = item->getElementAdapter();
            m_eventSelection->addEvent(adapter->getEvent());
        }
    }

    emit stateChange("have_controller_item_selected", haveSelectedItems);
}

void ControlRuler::contentsContextMenuEvent(QContextMenuEvent* e)
{
    if (!m_menu && !m_menuName.isEmpty())
        createMenu();

    if (m_menu) {
        RG_DEBUG << "ControlRuler::showMenu() - show menu with" << m_menu->count() << " items\n";
        m_lastEventPos = inverseMapPoint(e->pos());
        m_menu->exec(QCursor::pos());
    } else
        RG_DEBUG << "ControlRuler::showMenu() : no menu to show\n";

}

void ControlRuler::createMenu()
{
    RG_DEBUG << "ControlRuler::createMenu()\n";

    KMainWindow* parentMainWindow = dynamic_cast<KMainWindow*>(topLevelWidget());

    if (parentMainWindow && parentMainWindow->factory()) {
        m_menu = static_cast<QPopupMenu*>(parentMainWindow->factory()->container(m_menuName, parentMainWindow));

        if (!m_menu) {
            RG_DEBUG << "ControlRuler::createMenu() failed\n";
        }
    } else {
        RG_DEBUG << "ControlRuler::createMenu() failed: no parent factory\n";
    }
}

void
ControlRuler::clearSelectedItems()
{
    for (Q3CanvasItemList::Iterator it = m_selectedItems.begin(); it != m_selectedItems.end(); ++it) {
        (*it)->setSelected(false);
    }
    m_selectedItems.clear();

    delete m_eventSelection;
    m_eventSelection = new EventSelection(*m_segment);
}

void ControlRuler::clear()
{
    Q3CanvasItemList allItems = canvas()->allItems();

    for (Q3CanvasItemList::Iterator it = allItems.begin(); it != allItems.end(); ++it) {
        if (ControlItem *item = dynamic_cast<ControlItem*>(*it)) {
            delete item;
        }
    }
}

int ControlRuler::valueToHeight(long val)
{
    long scaleVal = val * (ItemHeightRange);

    int res = -(int(scaleVal / getMaxItemValue()) + MinItemHeight);

    //RG_DEBUG << "ControlRuler::valueToHeight : val = " << val << " - height = " << res
    //<< " - scaleVal = " << scaleVal << endl;

    return res;
}

long ControlRuler::heightToValue(int h)
{
    long val = -h;
    val -= MinItemHeight;
    val *= getMaxItemValue();
    val /= (ItemHeightRange);
    val = std::min(val, long(getMaxItemValue()));
    return val;
}

QColor ControlRuler::valueToColour(int max, int val)
{
    int maxDefault = DefaultVelocityColour::getInstance()->getMaxValue();

    int value = val;

    // Scale value accordingly
    //
    if (maxDefault != max)
        value = int(double(maxDefault) * double(val) / double(max));

    return DefaultVelocityColour::getInstance()->getColour(value);
}

int ControlRuler::applyTool(double x, int val)
{
    if (m_tool)
        return (*m_tool)(x, val);
    return val;
}

void ControlRuler::flipForwards()
{
    std::pair<int, int> minMax = getZMinMax();

    Q3CanvasItemList l = canvas()->allItems();
    for (Q3CanvasItemList::Iterator it = l.begin(); it != l.end(); ++it) {

        // skip all but rectangles
        if ((*it)->rtti() != Q3CanvasItem::Rtti_Rectangle)
            continue;

        // match min
        if ((*it)->z() == minMax.second)
            (*it)->setZ(minMax.first);
        else
            (*it)->setZ((*it)->z() + 1);
    }

    canvas()->update();
}

void ControlRuler::flipBackwards()
{
    std::pair<int, int> minMax = getZMinMax();

    Q3CanvasItemList l = canvas()->allItems();
    for (Q3CanvasItemList::Iterator it = l.begin(); it != l.end(); ++it) {

        // skip all but rectangles
        if ((*it)->rtti() != Q3CanvasItem::Rtti_Rectangle)
            continue;

        // match min
        if ((*it)->z() == minMax.first)
            (*it)->setZ(minMax.second);
        else
            (*it)->setZ((*it)->z() - 1);
    }

    canvas()->update();
}

std::pair<int, int> ControlRuler::getZMinMax()
{
    Q3CanvasItemList l = canvas()->allItems();
    std::vector<int> zList;
    for (Q3CanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it) {

        // skip all but rectangles
        if ((*it)->rtti() != Q3CanvasItem::Rtti_Rectangle) continue;
        zList.push_back(int((*it)->z()));
    }

    std::sort(zList.begin(), zList.end());

    return std::pair<int, int>(zList[0], zList[zList.size() - 1]);
}

QScrollBar* ControlRuler::getMainHorizontalScrollBar()
{
    return m_mainHorizontalScrollBar ? m_mainHorizontalScrollBar : horizontalScrollBar();
}

}
#include "ControlRuler.moc"
