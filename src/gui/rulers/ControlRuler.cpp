/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[ControlRuler]"

#include "ControlRuler.h"

#include "base/Event.h"
#include "misc/Debug.h"
#include "base/RulerScale.h"
#include "document/Command.h"
#include "commands/notation/NormalizeRestsCommand.h"
#include "gui/editors/notation/NotationStaff.h"
#include "ControlMouseEvent.h"
#include "ControlItem.h"
#include "ControlSelector.h"
#include "ControlTool.h"
#include "ControlToolBox.h"
#include "ControlChangeCommand.h"
#include "DefaultVelocityColour.h"
#include "document/CommandHistory.h"
#include <algorithm>
#include <cfloat>

#include <QMainWindow>
#include <QColor>
#include <QPoint>
#include <QPolygonF>
#include <QPolygon>
#include <QMenu>
#include <QString>
#include <QWidget>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QPainter>
#include <QBrush>
#include <QPen>

namespace Rosegarden
{

const int ControlRuler::DefaultRulerHeight = 75;
const int ControlRuler::MinItemHeight = 5;
const int ControlRuler::MaxItemHeight = 64 + 5;
const int ControlRuler::ItemHeightRange = 64;

    ControlRuler::ControlRuler(ViewSegment * /*viewsegment*/,
                           RulerScale* rulerScale,
                           QWidget* parent) :
        QWidget(parent),
        m_rulerScale(rulerScale),
        m_eventSelection(0),
        m_viewSegment(0),
        m_notationStaff(0),
        m_segment(0),
        m_firstVisibleItem(m_controlItemMap.end()),
        m_lastVisibleItem(m_controlItemMap.end()),
        m_nextItemLeft(m_controlItemMap.end()),
        m_currentIndex(0),
        m_currentTool(0),
        m_xScale(1.0),
        m_yScale(1.0),
        m_maxItemValue(127),
        m_minItemValue(0),
        m_viewSegmentOffset(0),
        m_xOffset(0),
        m_currentX(0.0),
        m_itemMoved(false),
        m_selecting(false),
        m_selectionRect(0),
        m_menu(0)
{

    setFixedHeight(sizeHint().height());
    setMouseTracking(true);

    m_toolBox = new ControlToolBox(this);

    emit stateChange("have_controller_item_selected", false);
}

ControlRuler::~ControlRuler()
{
}

void ControlRuler::setSegment(Segment *segment)
{
    m_segment = segment;

    if (m_eventSelection) delete m_eventSelection;

    m_eventSelection = new EventSelection(*segment);
}

void ControlRuler::setViewSegment(ViewSegment *viewSegment)
{
    m_viewSegment = viewSegment;
    
    // If this ruler is connected to a NotationStaff then this will return a valid pointer
    //   otherwise, it will return zero. This can be used to check whether we're connected
    //   to a matrix or notation editor later
    m_notationStaff = dynamic_cast <NotationStaff *> (viewSegment);

    setSegment(&m_viewSegment->getSegment());
}

ControlItemMap::iterator ControlRuler::findControlItem(float x)
{
    ControlItemMap::iterator it;
    it = m_controlItemMap.upper_bound(x);
    return it;
}

ControlItemMap::iterator ControlRuler::findControlItem(const Event *event)
{
    // double xstart = getRulerScale()->getXForTime(event->getAbsoluteTime());

    ControlItemMap::iterator it;
    std::pair <ControlItemMap::iterator,ControlItemMap::iterator> ret;

    for (it = m_controlItemMap.begin(); it != m_controlItemMap.end(); ++it) {
        if (it->second->getEvent() == event) break;
    }

    ///@TODO equal_range (above) is not behaving as expected - sort it out
//    if (it != m_controlItemMap.end() && it->second->getEvent() != event) {
//        it = m_controlItemMap.end();
//    }

    return it;
}

ControlItemMap::iterator ControlRuler::findControlItem(const ControlItem* item)
{
    // Basic loop through until I get the equal_range thing working properly
    ControlItemMap::iterator it;
    for (it = m_controlItemMap.begin(); it != m_controlItemMap.end(); ++it) {
        if (it->second == item) break;
    }
    return it;
}

void ControlRuler::addControlItem(ControlItem* item)
{
    // Add a ControlItem to the ruler
    RG_DEBUG << "ControlItem added: " << hex << (long)item;
    
    // ControlItem may not have an assigned event but must have x position
    ControlItemMap::iterator it = m_controlItemMap.insert(ControlItemMap::value_type(item->xStart(),item));
    addCheckVisibleLimits(it);    
    if (it->second->isSelected()) m_selectedItems.push_back(it->second);

}

void ControlRuler::addCheckVisibleLimits(ControlItemMap::iterator it)
{
    // Referenced item is has just been added to m_controlItemMap
    // If it is visible, add it to the list and correct first/last
    // visible item iterators
    ControlItem *item = it->second;
    
    // If this new item is visible
    if (visiblePosition(item)==0) {
        // put it in the visible list
        m_visibleItems.push_back(item);
        // If there is no first visible item or this one is further left
        if (m_firstVisibleItem == m_controlItemMap.end() || 
                item->xStart() < m_firstVisibleItem->second->xStart()) {
            // make it the first visible item
            m_firstVisibleItem = it;
        }

        // If there is no last visible item or this is further right
        if (m_lastVisibleItem == m_controlItemMap.end() ||
                item->xStart() > m_lastVisibleItem->second->xStart()) {
            // make it the last visible item
            m_lastVisibleItem = it;
        }
    }

    // If the new item is invisible to the left
    if (visiblePosition(item) == -1) {
        if (m_nextItemLeft == m_controlItemMap.end() ||
                item->xStart() > m_nextItemLeft->second->xStart()) {
            // make it the next item to the left
            m_nextItemLeft = it;
        }
    }
}

void ControlRuler::removeControlItem(ControlItem* item)
{
    // Remove control item by item pointer
    // No search by Value provided for std::multimap so find items with the requested item's
    //  xstart position and sweep these for the correct entry
    ControlItemMap::iterator it = findControlItem(item);

    if (it != m_controlItemMap.end()) removeControlItem(it);
}

void ControlRuler::removeControlItem(const Event *event)
{
    // Remove the ControlItem matching the received event if one exists
    ControlItemMap::iterator it = findControlItem(event);

    if (it != m_controlItemMap.end()) {
        RG_DEBUG << "removeControlItem at x = " << it->first;
        removeControlItem(it);
    }
}

void ControlRuler::removeControlItem(const ControlItemMap::iterator &it)
{
    RG_DEBUG << "removeControlItem: iterator->item: " << hex << (long) it->second;
    RG_DEBUG << "m_selectedItems.front(): " << hex << (long) m_selectedItems.front();
    
    if (it->second->isSelected()) m_selectedItems.remove(it->second);
    removeCheckVisibleLimits(it);
    m_controlItemMap.erase(it);
}

void ControlRuler::removeCheckVisibleLimits(const ControlItemMap::iterator &it)
{
    // Referenced item is being removed from m_controlItemMap
    // If it was visible, remove it from the list and correct first/last
    // visible item iterators
    // Note, we can't check if it _was_ visible. It may have just become invisible
    // Try to remove from list and check iterators.
    m_visibleItems.remove(it->second);
    
    // If necessary, correct the first and lastVisibleItem iterators 
    // If this was the first visible item
    if (it == m_firstVisibleItem) {
        // Check the next item to the right
        ++m_firstVisibleItem;
        // If the next item to the right is invisible, there are no visible items
        // Note we have to check .end() before we dereference ->second
        if (m_firstVisibleItem != m_controlItemMap.end() &&
                visiblePosition(m_firstVisibleItem->second)!=0)
            m_firstVisibleItem = m_controlItemMap.end();
    }
    
    // If this was the last visible item
    if (it == m_lastVisibleItem) {
        // and not the first in the list
        if (it != m_controlItemMap.begin()) {
            // check the next item to the left
            --m_lastVisibleItem;
            // If this is invisible, there are no visible items
            if (visiblePosition(m_lastVisibleItem->second)!=0) m_lastVisibleItem = m_controlItemMap.end();
        }
        // if it's first in the list then there are no visible items
        else m_lastVisibleItem = m_controlItemMap.end();
    }
    
    // If this was the first invisible item left (could be part of a selection moved off screen)
    if (it == m_nextItemLeft) {
        // and not the first in the list
        if (it != m_controlItemMap.begin()) {
            // use the next to the left (we know it is invisible)
            --m_nextItemLeft;
        }
        // if it's first in the list then there are no invisible items to the left
        else m_nextItemLeft = m_controlItemMap.end();
    }
}

void ControlRuler::eraseControlItem(const Event *event)
{
    ControlItemMap::iterator it = findControlItem(event);
    if (it != m_controlItemMap.end()) eraseControlItem(it);
}

void ControlRuler::eraseControlItem(const ControlItemMap::iterator &it)
{
    ControlItem *item = it->second;
    removeControlItem(it);
    delete item;
}

void ControlRuler::moveItem(ControlItem* item)
{
    // Move the item within m_controlItemMap
    // Need to check changes in visibility
    // DO NOT change isSelected or m_selectedItems as this is used to loop this
    ControlItemMap::iterator it = findControlItem(item);
    if (it == m_controlItemMap.end()) return;

    removeCheckVisibleLimits(it);
    m_controlItemMap.erase(it);
    it = static_cast <ControlItemMap::iterator> (m_controlItemMap.insert(ControlItemMap::value_type(item->xStart(),item)));
    addCheckVisibleLimits(it);
}

int ControlRuler::visiblePosition(ControlItem* item)
{
    // Check visibility of an item
    // Returns: -1 - item is off screen left
    //           0 - item is visible
    //          +1 - item is off screen right
    if (item->xEnd() < m_pannedRect.left()) return -1;
    if (item->xStart() > m_pannedRect.right()) return 1;

    return 0;
}

float ControlRuler::getXMax()
{
    return (m_rulerScale->getXForTime(m_segment->getEndTime()));
}

float ControlRuler::getXMin()
{
    return (m_rulerScale->getXForTime(m_segment->getStartTime()));
}

void ControlRuler::updateSegment()
{
    // Bring the segment up to date with the ControlRuler's items
    // A number of different actions take place here:
    // 1) m_eventSelection is empty
    // 2) m_eventSelection has events
    //      a) Events in the selection have been modified in value only
    //      b) Events in the selection have moved in time
    //
    // Either run through the ruler's EventSelection, updating from each item
    //  or, if there isn't one, go through m_selectedItems
    timeT start,end;

    QString commandLabel = "Adjust control/property";

    MacroCommand *macro = new MacroCommand(commandLabel);

    // Find the extent of the selected items
    float xmin=FLT_MAX,xmax=-1.0;

    // EventSelection::addEvent adds timeT(1) to its extentt for zero duration events so need to mimic this here
    timeT durationAdd = 0;

    for (ControlItemList::iterator it = m_selectedItems.begin(); it != m_selectedItems.end(); ++it) {
        if ((*it)->xStart() < xmin) xmin = (*it)->xStart();
        if ((*it)->xEnd() > xmax) {
            xmax = (*it)->xEnd();
            if ((*it)->xEnd() == (*it)->xStart())
                durationAdd = 1;
            else
                durationAdd = 0;
        }
    }

    start = getRulerScale()->getTimeForX(xmin);
    end = getRulerScale()->getTimeForX(xmax)+durationAdd;

    if (m_eventSelection->getAddedEvents() == 0) {
        // We do not have a valid set of selected events to update
        if (m_selectedItems.size() == 0) {
            // There are no selected items, nothing to update
            return;
        }

        // Events will be added by the controlItem->updateSegment methods
        commandLabel = "Add control";
        macro->setName(commandLabel);

    } else {
        // Check for movement in time here and delete events if necessary
        if (start != m_eventSelection->getStartTime() || end != m_eventSelection->getEndTime()) {
            commandLabel = "Move control";
            macro->setName(commandLabel);

            // Get the limits of the change for undo
            start = std::min(start,m_eventSelection->getStartTime());
            end = std::max(end,m_eventSelection->getEndTime());

        }
    }

    // Add change command to macro
    // ControlChangeCommand calls each selected items updateSegment method
    // Note that updateSegment deletes and renews the event whether it has moved or not
    macro->addCommand(new ControlChangeCommand(m_selectedItems,
                                    *m_segment,
                                    start,
                                    end));

    CommandHistory::getInstance()->addCommand(macro);

    updateSelection();
}

void ControlRuler::slotUpdate()
{
    RG_DEBUG << "ControlRuler::slotUpdate()\n";

///TODO Set some update flag?
}

void ControlRuler::notationLayoutUpdated(timeT startTime, timeT /*endTime*/)
{
    // notationLayoutUpdated should be called after notation has adjusted the layout
    // Clearly, for property control rulers, notes may have been moved so their position needs updating
    // The rulers may also have changed so ControllerEventRulers also need updating
    // Property control items may now need to be repositioned within the ControlItemMap
    // as new items are all created with a zero x-position, and have now been put in place.
    // For this reason, we need to collect items into a separate list otherwise we get the
    // dreaded 'modifying a list within a loop of the list' problem which can take quite a long
    // time to fix!
    std::vector<ControlItem*> itemsToUpdate;
    ControlItemMap::iterator it = m_controlItemMap.begin();
    while (it != m_controlItemMap.end() && it->first == 0) {
        itemsToUpdate.push_back(it->second);
        ++it;
    }
    
    while (it != m_controlItemMap.end() && it->first < getRulerScale()->getXForTime(startTime)) ++it;
    
    // Would like to only update in the defined region but, unfortunately, everything after this time
    // may well have moved as well so we have to do everything after startTime
    while (it != m_controlItemMap.end()) {
        itemsToUpdate.push_back(it->second);
        ++it;
    }
    
    for (std::vector<ControlItem*>::iterator vit = itemsToUpdate.begin(); vit != itemsToUpdate.end(); ++vit) {
        (*vit)->update();
        RG_DEBUG << "Updated item: " << hex << (long)(*vit);
    }
    
    update();
}

void ControlRuler::paintEvent(QPaintEvent * /*event*/)
{
    RG_DEBUG << "ControlRuler::paintEvent: width()=" << width() << " height()=" << height();
    QPainter painter(this);

    QPen pen;
    QBrush brush;

    pen.setStyle(Qt::NoPen);
    painter.setPen(pen);

    brush.setStyle(Qt::SolidPattern);
    brush.setColor(Qt::white);
    painter.setBrush(brush);

    painter.drawRect(0,0,width(),height());

    double xstart = m_rulerScale->getXForTime(m_segment->getStartTime());
    double xend = m_rulerScale->getXForTime(m_segment->getEndTime());

    xstart = mapXToWidget(xstart*m_xScale);
    xend = mapXToWidget(xend*m_xScale);

    RG_DEBUG << "ControlRuler::paintEvent: xstart=" << xstart;

    painter.setPen(QColor(127, 127, 127));
    painter.drawLine(xstart, mapYToWidget(0.0f), xend, mapYToWidget(0.0f));
    painter.drawLine(xstart, mapYToWidget(0.5f), xend, mapYToWidget(0.5f));
    painter.drawLine(xstart, mapYToWidget(1.0f), xend, mapYToWidget(1.0f));

    painter.setPen(QColor(192, 192, 192));
    painter.drawLine(xstart, mapYToWidget(0.25f), xend, mapYToWidget(0.25f));
    painter.drawLine(xstart, mapYToWidget(0.75f), xend, mapYToWidget(0.75f));
}

void ControlRuler::slotScrollHorizSmallSteps(int /*step*/)
{
}

int ControlRuler::mapXToWidget(float x)
{
    return (0.5+(m_xOffset+x-m_pannedRect.left()) / m_xScale);
}

int ControlRuler::mapYToWidget(float y)
{
    return (0.5+(-y+1.0f) / m_yScale);
}

QRect ControlRuler::mapItemToWidget(QRectF *rect)
{
    QRect newrect;

    newrect.setTopLeft(QPoint(mapXToWidget(rect->left()),mapYToWidget(rect->top())));
    newrect.setBottomRight(QPoint(mapXToWidget(rect->right()),mapYToWidget(rect->bottom())));

    return newrect;
}

QPolygon ControlRuler::mapItemToWidget(QPolygonF *poly)
{

    QPolygon newpoly;
    QPoint newpoint;
    for (QPolygonF::iterator it = poly->begin(); it != poly->end(); ++it) {
        newpoint.setX(mapXToWidget((*it).x()));
        newpoint.setY(mapYToWidget((*it).y()));
        newpoly.push_back(newpoint);
    }

    return newpoly;
}

QPointF ControlRuler::mapWidgetToItem(QPoint *point)
{

    QPointF newpoint;
    newpoint.setX(m_xScale*(point->x()) + m_pannedRect.left() - m_xOffset);
    newpoint.setY(-m_yScale*(point->y()) + 1.0f);
    return newpoint;
}

void ControlRuler::slotSetPannedRect(QRectF pr)
{
    m_pannedRect = pr;
    m_xScale = (double) m_pannedRect.width() / (double) width();
    m_yScale = 1.0f / (double) height();

    // Create the visible items list
    ///TODO Improve efficiency using xstart and xstop ordered lists of control items
    m_visibleItems.clear();
    bool anyVisibleYet = false;

    m_nextItemLeft = m_controlItemMap.end();
    m_firstVisibleItem = m_controlItemMap.end();
    m_lastVisibleItem = m_controlItemMap.end();

    ControlItemMap::iterator it;
    for (it = m_controlItemMap.begin();it != m_controlItemMap.end(); ++it) {
        int visPos = visiblePosition(it->second);
        
        if (visPos == -1) m_nextItemLeft = it;
        
        if (visPos == 0) {
            if (!anyVisibleYet) {
                m_firstVisibleItem = it;
                anyVisibleYet = true;
            }
                
            m_visibleItems.push_back(it->second);
            m_lastVisibleItem = it;
        }
        
        if (visPos == 1) break;
    }
    
    RG_DEBUG << "ControlRuler::slotSetPannedRect - visible items: " << m_visibleItems.size();
}

void ControlRuler::resizeEvent(QResizeEvent *)
{
    // Note slotSetPannedRect is called (from ControlRulerWidget::slotSetPannedRect)
    //   on a resize event. However, this call is too early and width() has not been
    //   updated. This event handler patches that problem. Could be more efficient.
    slotSetPannedRect(m_pannedRect);
}

void ControlRuler::slotSetTool(const QString &/*matrixtoolname*/)
{
}

ControlMouseEvent ControlRuler::createControlMouseEvent(QMouseEvent* e)
{
    ControlMouseEvent controlMouseEvent;
    QPoint widgetMousePos = e->pos();
    QPointF mousePos = mapWidgetToItem(&widgetMousePos);
    controlMouseEvent.x = mousePos.x();
    controlMouseEvent.y = mousePos.y();

    for (ControlItemList::iterator it = m_visibleItems.begin();
            it != m_visibleItems.end(); ++it) {
        if ((*it)->containsPoint(mousePos,Qt::OddEvenFill)) {
            controlMouseEvent.itemList.push_back(*it);
        }
    }
    
    controlMouseEvent.buttons = e->buttons();
    controlMouseEvent.modifiers = e->modifiers();

    return controlMouseEvent;
}

void ControlRuler::mousePressEvent(QMouseEvent* e)
{
    if (!m_currentTool)
        return;

    ControlMouseEvent controlMouseEvent = createControlMouseEvent(e);
    m_currentTool->handleLeftButtonPress(&controlMouseEvent);
}

void ControlRuler::mouseReleaseEvent(QMouseEvent* e)
{
    if (!m_currentTool)
        return;

    ControlMouseEvent controlMouseEvent = createControlMouseEvent(e);
    m_currentTool->handleMouseRelease(&controlMouseEvent);
}

void ControlRuler::mouseMoveEvent(QMouseEvent* e)
{
    if (!m_currentTool)
        return;

    ControlMouseEvent controlMouseEvent = createControlMouseEvent(e);
    ControlTool::FollowMode mode = m_currentTool->handleMouseMove(&controlMouseEvent);

    if (mode != ControlTool::NoFollow) {
        emit dragScroll(m_rulerScale->getTimeForX(controlMouseEvent.x/m_xScale));
    }
}

void
ControlRuler::wheelEvent(QWheelEvent * /*e*/)
{
    // not sure what to do yet

}

void ControlRuler::contextMenuEvent(QContextMenuEvent* e)
{
    if (!m_menu && !m_menuName.isEmpty())
        createMenu();

    if (m_menu) {
//        RG_DEBUG << "ControlRuler::showMenu() - show menu with" << m_menu->count() << " items\n";
        m_lastEventPos = e->pos(); ///CJ OK ??? - inverseMapPoint(e->pos());
        m_menu->exec(QCursor::pos());
    } else
        RG_DEBUG << "ControlRuler::showMenu() : no menu to show\n";

}

void ControlRuler::createMenu()
{
    RG_DEBUG << "ControlRuler::createMenu()\n";

    QMainWindow* parentMainWindow = dynamic_cast<QMainWindow*>(topLevelWidget());

    if (parentMainWindow ) {     // parentMainWindow->factory()) {
        m_menu = parentMainWindow->findChild<QMenu*>(m_menuName);

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
    for (ControlItemList::iterator it = m_selectedItems.begin(); it != m_selectedItems.end(); ++it) {
        (*it)->setSelected(false);
    }
    m_selectedItems.clear();

    if (m_eventSelection) delete m_eventSelection;

    m_eventSelection = new EventSelection(*m_segment);
}

void ControlRuler::updateSelection()
{
    if (m_eventSelection) delete m_eventSelection;
    m_eventSelection = new EventSelection(*m_segment);

    for (ControlItemList::iterator it = m_selectedItems.begin(); it != m_selectedItems.end(); ++it) {
        m_eventSelection->addEvent((*it)->getEvent());
    }

    RG_DEBUG << "control ruler updating selection";
    emit rulerSelectionChanged(m_eventSelection);
}

void ControlRuler::addToSelection(ControlItem *item)
{
    ControlItemList::iterator found = 
        std::find (m_selectedItems.begin(),
                   m_selectedItems.end(), item);
    
    // If we already have this item, do nothing.
    if (found != m_selectedItems.end()) { return; }
    m_selectedItems.push_back(item);
    item->setSelected(true);
    m_eventSelection->addEvent(item->getEvent());
    emit rulerSelectionChanged(m_eventSelection);
    RG_DEBUG << "control ruler add to selection";
}

void ControlRuler::removeFromSelection(ControlItem*item)
{
    m_selectedItems.remove(item);
    item->setSelected(false);
    m_eventSelection->removeEvent(item->getEvent());
    emit rulerSelectionChanged(m_eventSelection);
}

void ControlRuler::clear()
{
    RG_DEBUG << "ControlRuler::clear - m_controlItemMap.size(): " << m_controlItemMap.size();
    for (ControlItemMap::iterator it = m_controlItemMap.begin(); it != m_controlItemMap.end(); ++it) {
        if (ControlItem *item = dynamic_cast<ControlItem*>(it->second)) {
            RG_DEBUG << "Deleting controlItem";
            delete item;
        }
    }
    m_controlItemMap.clear();
    m_firstVisibleItem = m_controlItemMap.end();
    m_lastVisibleItem = m_controlItemMap.end();
    m_nextItemLeft = m_controlItemMap.end(); 
    
    m_visibleItems.clear();
    m_selectedItems.clear();
}

float ControlRuler::valueToY(long val)
{
    float y = (float)(val-getMinItemValue())
            /(float)(getMaxItemValue()-getMinItemValue());
    return y;
}

long ControlRuler::yToValue(float y)
{
    // NOTE: while debugging #1451 I had debug output here and it confirmed that
    // value is returning very reasonable numbers, which get mangled elsewhere
    long value = (long)(y*(getMaxItemValue()-getMinItemValue()))+getMinItemValue();
    return value;
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

void ControlRuler::flipForwards()
{
    ///CJ Expect to drop tghis with a better way of ordering bars
    // std::pair<int, int> minMax = getZMinMax();

}

void ControlRuler::flipBackwards()
{
    ///CJ Expect to drop tghis with a better way of ordering bars
    // std::pair<int, int> minMax = getZMinMax();

}

std::pair<int, int> ControlRuler::getZMinMax()
{
    std::vector<int> zList;

    std::sort(zList.begin(), zList.end());

    return std::pair<int, int>(zList[0], zList[zList.size() - 1]);
}

}
