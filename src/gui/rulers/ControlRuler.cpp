/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ControlRuler.h"

#include "base/Event.h"
#include "misc/Debug.h"
#include "base/RulerScale.h"
#include "document/Command.h"
#include "commands/notation/NormalizeRestsCommand.h"
//#include "base/Segment.h"
//#include "base/Selection.h"
#include "ControlMouseEvent.h"
#include "ControlItem.h"
#include "ControlSelector.h"
#include "ControlTool.h"
#include "ControlToolBox.h"
#include "ControlChangeCommand.h"
#include "DefaultVelocityColour.h"
//#include "ElementAdapter.h"
//#include "gui/general/EditViewBase.h"
//#include "gui/general/RosegardenCanvasView.h"
#include "document/CommandHistory.h"
//#include "gui/widgets/TextFloat.h"
#include <algorithm>
#include <cfloat>

#include <QMainWindow>
//#include <Q3Canvas>
#include <QColor>
#include <QPoint>
#include <QPolygonF>
#include <QPolygon>
#include <QMenu>
//#include <QScrollBar>
//#include <QScrollArea>
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

ControlRuler::ControlRuler(MatrixViewSegment *viewsegment,
                           RulerScale* rulerScale,
                           QWidget* parent
						  ) :
        QWidget(parent),
        m_rulerScale(rulerScale),
        m_eventSelection(0),
        m_viewSegment(0),
        m_segment(0),
//        m_assignedEventSelection(0),
        m_currentIndex(0),
        m_currentTool(0),
        m_xScale(0),
        m_yScale(0),
        m_maxItemValue(127),
        m_minItemValue(0),
        m_viewSegmentOffset(0),
        m_currentX(0.0),
        m_itemMoved(false),
        m_selecting(false),
//        m_selector(new ControlSelector(this)),
//        m_selectionRect(new Q3CanvasRectangle(canvas())),
        m_selectionRect(0),
        m_menu(0)
{
//    setViewSegment(viewsegment);

    setFixedHeight(sizeHint().height());
    setMouseTracking(true);

///TODO    connect(this, SIGNAL(stateChange(const QString&, bool)),
      //      m_parentEditView, SLOT(slotStateChanged(const QString&, bool)));
    m_toolBox = new ControlToolBox(this);

    emit stateChange("have_controller_item_selected", false);
}

ControlRuler::~ControlRuler()
{
//    if(m_assignedEventSelection)
//	    m_assignedEventSelection->removeObserver(this);
//
//    if (m_viewSegment) {
//        m_viewSegment->removeObserver(this);
//    }
}

void ControlRuler::setSegment(Segment *segment)
{
    m_segment = segment;

    if (m_eventSelection) delete m_eventSelection;

    m_eventSelection = new EventSelection(*segment);
}

void ControlRuler::setViewSegment(MatrixViewSegment *viewSegment)
{
//    if (m_viewSegment) m_viewSegment->removeObserver(this);

    m_viewSegment = viewSegment;
//    m_viewSegment->addObserver(this);

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
    double xstart = getRulerScale()->getXForTime(event->getAbsoluteTime());

    ControlItemMap::iterator it;
    std::pair <ControlItemMap::iterator,ControlItemMap::iterator> ret;

//    ret = m_controlItemMap.equal_range(xstart);
//    for (it = ret.first; it != ret.second; it++) {
    for (it = m_controlItemMap.begin(); it != m_controlItemMap.end(); it++) {
        if (it->second->getEvent() == event) break;
    }

    ///@TODO equal_range (above) is not behaving as expected - sort it out
//    if (it != m_controlItemMap.end() && it->second->getEvent() != event) {
//        it = m_controlItemMap.end();
//    }

    return it;
}

void ControlRuler::addControlItem(ControlItem* item)
{
    // Add a ControlItem to the ruler
    // ControlItem may not have an assigned event but must have x position
    m_controlItemMap.insert(std::pair<double,ControlItem *>
        (item->xStart(),item));
    if (isVisible(item)) {
        m_visibleItems.push_back(item);
    }

//    m_controlItemEnd.insert(std::pair<double,ControlItemList::iterator>
//        (item->xEnd(),--m_controlItemList.end()));
}

void ControlRuler::removeControlItem(ControlItem* item)
{
//    // Remove control item by item pointer
//    // No search by Value provided for std::multimap so find items with the requested item's
//    //  xstart position and sweep these for the correct entry
//    double xstart = item->xStart();
//
//    ControlItemMap::iterator it;
//    std::pair <ControlItemMap::iterator,ControlItemMap::iterator> ret;
//
//    ret = m_controlItemMap.equal_range(xstart);
//    for (it = ret.first; it != ret.second; it++) {
//        if (it->second == item) break;
//    }
//
//    if (it != m_controlItemMap.end()) removeControlItem(it);
    removeControlItem(item->getEvent());
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
    m_controlItemMap.erase(it);
    m_selectedItems.remove(it->second);
    m_visibleItems.remove(it->second);
    delete (it->second);
}

bool ControlRuler::isVisible(ControlItem* item)
{
    if (item->xEnd() > m_pannedRect.left() && item->xStart() < m_pannedRect.right())
        return true;

    return false;
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
    bool segmentModified = false;

    QString commandLabel = "Adjust control/property";

    MacroCommand *macro = new MacroCommand(commandLabel);

    // Find the extent of the selected items
    float xmin=FLT_MAX,xmax=-1.0;

    // EventSelection::addEvent adds timeT(1) to its extentt for zero duration events so need to mimic this here
    timeT durationAdd = 0;

    for (ControlItemList::iterator it = m_selectedItems.begin(); it != m_selectedItems.end(); it++) {
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

    if (m_eventSelection->getAddedEvents()==0) {
        // We do not have a valid set of selected events to update
        if (m_selectedItems.size() == 0) {
            // There are no selected items, nothing to update
            return;
        }

        // Events will be added by the controlItem->updateSegment methods
        commandLabel = "Add control";
        macro->setName(commandLabel);

        segmentModified = true;
    } else {
        // Check for movement in time here and delete events if necessary
        if (start != m_eventSelection->getStartTime() || end != m_eventSelection->getEndTime())
        {
            commandLabel = "Move control";
            macro->setName(commandLabel);

            // Get the limits of the change for undo
            start = std::min(start,m_eventSelection->getStartTime());
            end = std::max(end,m_eventSelection->getEndTime());

            segmentModified = true;
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

void ControlRuler::paintEvent(QPaintEvent *event)
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

    xstart = mapXToWidget(xstart);
    xend = mapXToWidget(xend);

    RG_DEBUG << "ControlRuler::paintEvent: xstart=" << xstart;

    painter.setPen(QColor(127, 127, 127));
    painter.drawLine(xstart, mapYToWidget(0.0f), xend, mapYToWidget(0.0f));
    painter.drawLine(xstart, mapYToWidget(0.5f), xend, mapYToWidget(0.5f));
    painter.drawLine(xstart, mapYToWidget(1.0f), xend, mapYToWidget(1.0f));

    painter.setPen(QColor(192, 192, 192));
    painter.drawLine(xstart, mapYToWidget(0.25f), xend, mapYToWidget(0.25f));
    painter.drawLine(xstart, mapYToWidget(0.75f), xend, mapYToWidget(0.75f));
}

void ControlRuler::slotScrollHorizSmallSteps(int step)
{
}

//void ControlRuler::slotUpdateElementsHPos()
//{
//    // Update the position of control elements based on changes to the segment
//	// This is only called for PropertControlRulers
//    RG_DEBUG << "ControlRuler::slotUpdateElementsHPos()\n";
//
//    // This is only called for PropertControlRulers
//    computeViewSegmentOffset();
//
////    Q3CanvasItemList list = canvas()->allItems();
//
////    Q3CanvasItemList::Iterator it = list.begin();
/////TODO Iterate through m_controlItemList
//    ControlItemList::iterator it = m_controlItemList.begin();
//    for (; it != m_controlItemList.end(); ++it) {
//        ///TODO This test should no be necessary
//        ControlItem* item = dynamic_cast<ControlItem*>(*it);
//        if (!item)
//            continue;
////        layoutItem(item);
//    }
//
////    canvas()->update();
//}

///TODO Use m_xScale, m_yScale for these map functions
int ControlRuler::mapXToWidget(float x)
{
    return (0.5+(x-m_pannedRect.left()) / m_xScale);
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
//    double xscale = width() / m_pannedRect.width();
//    double yscale = height();

    QPolygon newpoly;
    QPoint newpoint;
    for (QPolygonF::iterator it = poly->begin(); it != poly->end(); it++) {
        newpoint.setX(mapXToWidget((*it).x()));
        newpoint.setY(mapYToWidget((*it).y()));
        newpoly.push_back(newpoint);
    }

    return newpoly;
}

QPointF ControlRuler::mapWidgetToItem(QPoint *point)
{
//    double xscale = (double) m_pannedRect.width() / (double) width();
//    double yscale = 1.0f / (double) height();

    QPointF newpoint;
    newpoint.setX(m_xScale*(point->x()) + m_pannedRect.left());
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
	for (ControlItemMap::iterator it = m_controlItemMap.begin();
        it != m_controlItemMap.end(); ++it) {
	    if (isVisible(it->second)) {
	        m_visibleItems.push_back(it->second);
	    }
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

//void ControlRuler::slotSetScale(double factor)
//{
//	m_scale = factor;
//}

void ControlRuler::slotSetTool(const QString &matrixtoolname)
{
}

//void ControlRuler::eventSelected(EventSelection *es,Event *e) {
//    if(es==m_assignedEventSelection) {
////        Q3CanvasItemList list = canvas()->allItems();
////        Q3CanvasItemList::Iterator it = list.begin();
/////TODO Iterate through m_controlItemList
//        //for (; it != list.end(); ++it) {
//            //if (ControlItem *item = dynamic_cast<ControlItem*>(*it)) {
//        		//if(item->getElementAdapter()->getEvent()==e) {
//        			//item->setHighlighted(true);
//    	    		//return;
//    		    //}
//	        //}
//        //}
//    }
//}
//
//void ControlRuler::eventDeselected(EventSelection *es,Event *e) {
//    if(es==m_assignedEventSelection) {
////        Q3CanvasItemList list = canvas()->allItems();
////        Q3CanvasItemList::Iterator it = list.begin();
/////TODO Iterate through m_controlItemList
//        //for (; it != list.end(); ++it) {
//            //if (ControlItem *item = dynamic_cast<ControlItem*>(*it)) {
//    	        //if(item->getElementAdapter()->getEvent()==e) {
//                    //item->setHighlighted(false);
//                    //return;
//    	        //}
//            //}
//        //}
//    }
//}
//
//void ControlRuler::eventSelectionDestroyed(EventSelection *es) {
//	/// Someone destroyed  ES  lets handle that
//	if(es==m_assignedEventSelection)
//		m_assignedEventSelection=NULL;
//}
//
//
//void ControlRuler::assignEventSelection(EventSelection *es)
//{
//    // Clear all selected ControllItem
////    Q3CanvasItemList list = canvas()->allItems();
////    Q3CanvasItemList::Iterator it = list.begin();
/////TODO Iterate through m_controlItemList
//    //for (; it != list.end(); ++it) {
//        //if (ControlItem *item = dynamic_cast<ControlItem*>(*it))
//    	    //item->setHighlighted(false);
//    //}
//
//    if(es) {
//        // Dont observe the old selection anymore
//        m_assignedEventSelection=es;
//
////        Q3CanvasItemList list = canvas()->allItems();
//        const EventSelection::eventcontainer ec=es->getSegmentEvents();
//        for (EventSelection::eventcontainer::iterator e = ec.begin(); e != ec.end(); ++e) {
////            Q3CanvasItemList::Iterator it = list.begin();
/////TODO Iterate through m_controlItemList
//            //for (; it != list.end(); ++it) {
//                //if (ControlItem *item = dynamic_cast<ControlItem*>(*it)) {
//                    //if(item->getElementAdapter()->getEvent()==*e) {
//                        //item->setHighlighted(true);
//                        //break;
//                    //}
//                //}
//            //}
//        }
//
//        es->addObserver(this);
//
//    } else {
//    	m_assignedEventSelection=NULL;
//    }
//
//    slotUpdate();
//}

//void ControlRuler::viewSegmentDeleted(const ViewSegment *)
//{
//    m_viewSegment = 0;
//    m_segment = 0;
//}
//
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
//    if (e->button() != Qt::LeftButton) {
//        TextFloat::getTextFloat()->hide();
//        m_selecting = false;
//        return ;
//    }
//
//    RG_DEBUG << "ControlRuler::contentsMousePressEvent()\n";
//
///TODO Not necessary in th custom widget implementation?
//    QPoint p = inverseMapPoint(e->pos());

//    Q3CanvasItemList l = canvas()->collisions(p);
///TODO Write simple collision detection code for m_controlItemList
/*

    if (l.count() == 0) { // de-select current item
        clearSelectedItems();
        m_selecting = true;
        m_selector->handleMouseButtonPress(e);
        RG_DEBUG << "ControlRuler::contentsMousePressEvent : entering selection mode\n";
        return ;
    }

    // clear selection unless control was pressed, in which case
    // add the event to the current selection
// 	if (!(e->state() && QMouseEvent::ControlButton)) {
	if( ! (e->modifiers() & Qt::CTRL) ){
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

                if (!(e->state() && Qt::CTRL) ){		//@@@ QMouseEvent::ControlButton)) {
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

*/ ///TODO
}

void ControlRuler::mouseReleaseEvent(QMouseEvent* e)
{
    if (!m_currentTool)
        return;

    ControlMouseEvent controlMouseEvent = createControlMouseEvent(e);
    m_currentTool->handleMouseRelease(&controlMouseEvent);

//    if (e->button() != Qt::LeftButton) {
//        TextFloat::getTextFloat()->hide();
//        m_selecting = false;
//        return ;
//    }
//
//    if (m_selecting) {
//        updateSelection();
//        m_selector->handleMouseButtonRelease(e);
//        RG_DEBUG << "ControlRuler::contentsMouseReleaseEvent : leaving selection mode\n";
//        m_selecting = false;
//        return ;
//    }
//
/////TODO Iterate through m_controlItemList
//    //for (Q3CanvasItemList::Iterator it = m_selectedItems.begin(); it != m_selectedItems.end(); ++it) {
//        //if (ControlItem *item = dynamic_cast<ControlItem*>(*it)) {
//
//            //ElementAdapter * adapter = item->getElementAdapter();
//            //m_eventSelection->addEvent(adapter->getEvent());
//            //item->handleMouseButtonRelease(e);
//        //}
//    //}
//
//    emit stateChange("have_controller_item_selected", true);
//
//    if (m_itemMoved) {
//
/////TODO Not necessary?
////        m_lastEventPos = inverseMapPoint(e->pos());
//
//        // Add command to history
//        ControlChangeCommand* command = new ControlChangeCommand(m_selectedItems,
//                                        *m_segment,
//                                        m_eventSelection->getStartTime(),
//                                        m_eventSelection->getEndTime());
//
//        RG_DEBUG << "ControlRuler::contentsMouseReleaseEvent : adding command\n";
//        CommandHistory::getInstance()->addCommand(command);
//
//        m_itemMoved = false;
//    }
//
//    TextFloat::getTextFloat()->hide();
}

void ControlRuler::mouseMoveEvent(QMouseEvent* e)
{
    if (!m_currentTool)
        return;

    ControlMouseEvent controlMouseEvent = createControlMouseEvent(e);
    m_currentTool->handleMouseMove(&controlMouseEvent);

//    QPoint p = e->pos(); ///CJ Is this ok - inverseMapPoint(e->pos());
//
//    int deltaX = p.x() - m_lastEventPos.x(),
//                 deltaY = p.y() - m_lastEventPos.y();
//    m_lastEventPos = p;
//
//    if (m_selecting) {
//        updateSelection();
//        m_selector->handleMouseMove(e, deltaX, deltaY);
//        slotScrollHorizSmallSteps(p.x());
//        return ;
//    }
//
//    m_itemMoved = true;
//
//    TextFloat *numberFloat = TextFloat::getTextFloat();
//    numberFloat->reparent(this);
//    // A better way should be not to call reparent() here, but to
//    // call attach() in enterEvent().
//    // Nevertheless it doesn't work because, for some reason,  enterEvent()
//    // (when defined) is never called when mouse enters the ruler.
//
//    int value = 0;
//
////    for (Q3CanvasItemList::Iterator it = m_selectedItems.begin(); it != m_selectedItems.end(); ++it) {
//    for (ControlItemList::iterator it = m_selectedItems.begin(); it != m_selectedItems.end(); ++it) {
//        if (ControlItem *item = dynamic_cast<ControlItem*>(*it)) {
//            item->handleMouseMove(e, deltaX, deltaY);
//            //            ElementAdapter* adapter = item->getElementAdapter();
//
//            // set value to highest in selection
//            if (item->getValue() >= value) {
//                value = item->getValue();
//                numberFloat->setText(QString("%1").arg(value));
//            }
//        }
//    }
//    ///CJ What to do? canvas()->update();
//
//    // Display text float near mouse cursor
//    QPoint offset = mapFromGlobal(QPoint(QCursor::pos()))
//                    + QPoint(20, + numberFloat->height() / 2);
//    numberFloat->display(offset);
}

void
ControlRuler::wheelEvent(QWheelEvent *e)
{
    // not sure what to do yet
    ///CJ ?? Q3CanvasView::contentsWheelEvent(e);
}

//void ControlRuler::updateSelection()
//{
//    clearSelectedItems();
//
//    bool haveSelectedItems = false;
//
//    //Q3CanvasItemList l = getSelectionRectangle()->collisions(true);
//    ControlItemList l; ///CJ Write collisions code!
//
////    for (Q3CanvasItemList::Iterator it = l.begin(); it != l.end(); ++it) {
//    for (ControlItemList::iterator it = l.begin(); it != l.end(); ++it) {
//
//        if (ControlItem *item = dynamic_cast<ControlItem*>(*it)) {
//            item->setSelected(true);
//            //m_selectedItems << item;
//            m_selectedItems.push_back(item);
//            haveSelectedItems = true;
//
////            ElementAdapter* adapter = item->getElementAdapter();
//            m_eventSelection->addEvent(item->getEvent());
//        }
//    }
//
//    emit stateChange("have_controller_item_selected", haveSelectedItems);
//}

void ControlRuler::contextMenuEvent(QContextMenuEvent* e)
{
    if (!m_menu && !m_menuName.isEmpty())
        createMenu();

    if (m_menu) {
        RG_DEBUG << "ControlRuler::showMenu() - show menu with" << m_menu->count() << " items\n";
        m_lastEventPos = e->pos(); ///CJ OK ??? - inverseMapPoint(e->pos());
        m_menu->exec(QCursor::pos());
    } else
        RG_DEBUG << "ControlRuler::showMenu() : no menu to show\n";

}

void ControlRuler::createMenu()
{
    RG_DEBUG << "ControlRuler::createMenu()\n";

    QMainWindow* parentMainWindow = dynamic_cast<QMainWindow*>(topLevelWidget());

    if (parentMainWindow ) { 	// parentMainWindow->factory()) {
// 		m_menu = static_cast<QMenu*>(parentMainWindow->factory()->container(m_menuName, parentMainWindow));
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

    for (ControlItemList::iterator it = m_selectedItems.begin(); it != m_selectedItems.end(); it++) {
        m_eventSelection->addEvent((*it)->getEvent());
    }
}

void ControlRuler::addToSelection(ControlItem *item)
{
    m_selectedItems.push_back(item);
    item->setSelected(true);
    m_eventSelection->addEvent(item->getEvent());
}

void ControlRuler::removeFromSelection(ControlItem*item)
{
    m_selectedItems.remove(item);
    item->setSelected(false);
    m_eventSelection->removeEvent(item->getEvent());
}

void ControlRuler::clear()
{
//    Q3CanvasItemList allItems = canvas()->allItems();
//    for (Q3CanvasItemList::Iterator it = allItems.begin(); it != allItems.end(); ++it) {
    RG_DEBUG << "ControlRuler::clear - m_controlItemMap.size(): " << m_controlItemMap.size();
    for (ControlItemMap::iterator it = m_controlItemMap.begin(); it != m_controlItemMap.end(); ++it) {
        if (ControlItem *item = dynamic_cast<ControlItem*>(it->second)) {
            RG_DEBUG << "Deleting controlItem";
            delete item;
        }
    }
    m_controlItemMap.clear();
}

//int ControlRuler::valueToHeight(long val)
//{
//    long scaleVal = val * (ItemHeightRange);
//
//    int res = -(int(scaleVal / getMaxItemValue()) + MinItemHeight);
//
//    //RG_DEBUG << "ControlRuler::valueToHeight : val = " << val << " - height = " << res
//    //<< " - scaleVal = " << scaleVal << endl;
//
//    return res;
//}
float ControlRuler::valueToY(long val)
{
    float y = (float)(val-getMinItemValue())
            /(float)(getMaxItemValue()-getMinItemValue());
    return y;
}

long ControlRuler::YToValue(float y)
{
    long value = (long)(y*(getMaxItemValue()-getMinItemValue()))+getMinItemValue();
    return value;
}

//long ControlRuler::heightToValue(int h)
//{
//    long val = -h;
//    val -= MinItemHeight;
//    val *= getMaxItemValue();
//    val /= (ItemHeightRange);
//    val = std::min(val, long(getMaxItemValue()));
//    return val;
//}

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

//int ControlRuler::applyTool(double x, int val)
//{
//    if (m_tool)
//        return (*m_tool)(x, val);
//    return val;
//}

void ControlRuler::flipForwards()
{
    ///CJ Expect to drop tghis with a better way of ordering bars
    std::pair<int, int> minMax = getZMinMax();

//    Q3CanvasItemList l = canvas()->allItems();
//    for (Q3CanvasItemList::Iterator it = l.begin(); it != l.end(); ++it) {
//    for (ControlItemList::iterator it = m_controlItemList.begin(); it != m_controlItemList.end(); ++it) {

        // skip all but rectangles
        ///CJ ??? if ((*it)->rtti() != Q3CanvasItem::Rtti_Rectangle)
            //continue;

        // match min
        //if ((*it)->z() == minMax.second)
            //(*it)->setZ(minMax.first);
        //else
            //(*it)->setZ((*it)->z() + 1);
//    }

    ///CJ ?? canvas()->update();
}

void ControlRuler::flipBackwards()
{
    ///CJ Expect to drop tghis with a better way of ordering bars
    std::pair<int, int> minMax = getZMinMax();

//    Q3CanvasItemList l = canvas()->allItems();
//    for (Q3CanvasItemList::Iterator it = l.begin(); it != l.end(); ++it) {
    //for (ControlItemList::iterator it = m_controlItemList.begin(); it != m_controlItemList.end(); ++it) {

        // skip all but rectangles
        ///CJ ??? if ((*it)->rtti() != Q3CanvasItem::Rtti_Rectangle)
            //continue;

        // match min
        //if ((*it)->z() == minMax.first)
            //(*it)->setZ(minMax.second);
        //else
            //(*it)->setZ((*it)->z() - 1);
//    }

    ///CJ ?? canvas()->update();
}

std::pair<int, int> ControlRuler::getZMinMax()
{
    ///CJ Expect to drop tghis with a better way of ordering bars
//    Q3CanvasItemList l = canvas()->allItems();
    std::vector<int> zList;
//    for (ControlItemList::iterator it=m_controlItemList.begin(); it!=m_controlItemList.end(); ++it) {

        // skip all but rectangles
        ///CJ ???? if ((*it)->rtti() != Q3CanvasItem::Rtti_Rectangle) continue;
//        zList.push_back(int((*it)->z()));
//    }

    std::sort(zList.begin(), zList.end());

    return std::pair<int, int>(zList[0], zList[zList.size() - 1]);
}

///CJ We have no scrollbar so can't return it!
//QScrollBar* ControlRuler::getMainHorizontalScrollBar()
//{
//    return m_mainHorizontalScrollBar ? m_mainHorizontalScrollBar : horizontalScrollBar();
//}

}
#include "ControlRuler.moc"
