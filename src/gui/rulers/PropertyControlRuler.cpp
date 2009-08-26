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


#include <QMouseEvent>
//#include <Q3Canvas>
//#include <Q3CanvasItemList>
//#include <Q3CanvasLine>
#include "PropertyControlRuler.h"

#include "ControlRuler.h"
#include "ControlTool.h"
#include "ControlToolBox.h"
#include "PropertyControlItem.h"
//#include "ViewElementAdapter.h"
#include "misc/Debug.h"
#include "base/BaseProperties.h"
#include "base/NotationTypes.h"
#include "base/PropertyName.h"
#include "base/RulerScale.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "base/ViewElement.h"
#include "commands/edit/SelectionPropertyCommand.h"
#include "document/CommandHistory.h"
#include "gui/general/EditViewBase.h"
#include "gui/general/GUIPalette.h"
#include "gui/editors/matrix/MatrixScene.h"
#include "gui/editors/matrix/MatrixViewSegment.h"
#include "gui/editors/matrix/MatrixElement.h"
#include "gui/widgets/TextFloat.h"
#include <QColor>
#include <QPoint>
#include <QString>
#include <QWidget>
#include <QPainter>


namespace Rosegarden
{

PropertyControlRuler::PropertyControlRuler(PropertyName propertyName,
                                           MatrixViewSegment *segment,
                                           RulerScale *rulerScale,
                                           QWidget *parent,
                                           const char *name) :
    ControlRuler(segment, rulerScale,
                 parent),
    m_propertyName(propertyName)
//    m_propertyLine(new Q3CanvasLine(canvas())),
//    m_propertyLineShowing(false),
//    m_propertyLineX(0),
//    m_propertyLineY(0)
{
//    m_viewSegment->addObserver(this);
//    m_propertyLine->setZ(1000); // bring to front

    setMenuName("property_ruler_menu");
//    drawBackground();
//    init();
    setViewSegment(segment);
}

void PropertyControlRuler::paintEvent(QPaintEvent *event)
{
    ControlRuler::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QBrush brush(Qt::SolidPattern);

    QPen highlightPen(GUIPalette::getColour(GUIPalette::SelectedElement),
            2, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
    QPen pen(GUIPalette::getColour(GUIPalette::MatrixElementBorder),
            0.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);

    // Use a fast vector list to record selected items that are currently visible so that they
    //  can be drawn last - can't use m_selectedItems as this covers all selected, visible or not
    std::vector<ControlItem*> selectedvector;

    for (ControlItemList::iterator it = m_visibleItems.begin(); it != m_visibleItems.end(); ++it) {
        if (!(*it)->isSelected()) {
            brush.setColor((*it)->getColour().lighter());
            painter.setBrush(brush);
            painter.setPen(Qt::NoPen);
            painter.drawPolygon(mapItemToWidget(*it));

            painter.setPen(pen);
            painter.drawPolyline(mapItemToWidget(*it));
        } else {
            selectedvector.push_back(*it);
        }
    }

    for (std::vector<ControlItem*>::iterator it = selectedvector.begin(); it != selectedvector.end(); ++it)
    {
        brush.setColor(((*it)->getColour()));
        painter.setBrush(brush);
        painter.setPen(Qt::NoPen);
        painter.drawPolygon(mapItemToWidget(*it));

        painter.setPen(highlightPen);
        painter.drawPolyline(mapItemToWidget(*it));
    }
}

void
PropertyControlRuler::setViewSegment(MatrixViewSegment *segment)
{
    if (m_viewSegment) m_viewSegment->removeObserver(this);
    m_viewSegment = segment;
    m_viewSegment->addObserver(this);
    ControlRuler::setViewSegment(segment);

    init();
}

PropertyControlRuler::~PropertyControlRuler()
{
    if (m_viewSegment) {
        m_viewSegment->removeObserver(this);
    }
}

QString PropertyControlRuler::getName()
{
    return getPropertyName().c_str();
}

void PropertyControlRuler::addControlItem(MatrixElement *el)
{
//    double x0 = el->getLayoutX();
//    double x1 = el->getWidth() + x0;
//    long val = 0;
//    el->event()->get<Rosegarden::Int>(getPropertyName(), val);
//    double y = (double) val / MIDI_CONTROL_MAX_VALUE;

    size_t s = sizeof(MatrixElement);
    RG_DEBUG << "PropertyControlRuler::addControlItem sizeof(MatrixElement): " << s;

    PropertyControlItem *controlItem = new PropertyControlItem(this, getPropertyName(), el, QPolygonF());
    controlItem->update();

    ControlRuler::addControlItem(controlItem);
//    m_controlItemList.push_back(controlItem);
}

//void PropertyControlRuler::addControlItem(Event *event)
//{
//    if (event->getType()!="note")
//        return;
//
//    RG_DEBUG << "Event Type: " << event->getType();
//    RG_DEBUG << "Event absolute time: " << event->getAbsoluteTime();
//
//    double x1 = m_rulerScale->getXForTime(event->getAbsoluteTime());
//    double x2 = m_rulerScale->getXForTime(event->getAbsoluteTime()+event->getDuration());
//    long val = 0;
//    event->get<Rosegarden::Int>(getPropertyName(), val);
//    double y = (double) val / MIDI_CONTROL_MAX_VALUE;
//
//    ControlItem *controlItem = new ControlItem(this, event, QPolygonF(QRectF(x1,0,x2-x1,y)));
//    m_controlItemList.push_back(controlItem);
//    //        m_controlItemList.push_back(new ControlItem(this, (*it), QPolygonF(QRectF(x1,y,x2-x1,y))));
//
////        new ControlItem(this, new ViewElementAdapter(*i, getPropertyName()), int(x + m_viewSegmentOffset),
////                        int(m_rulerScale->getXForTime((*i)->getViewAbsoluteTime() +
////                                                      (*i)->getViewDuration()) - x));
//
//    update();
//}
//
void PropertyControlRuler::init()
{
    // Clear Control Item list
    clear();

    if (!m_viewSegment)
        return;

    ViewElementList *viewElementList = m_viewSegment->getViewElementList();
    for (ViewElementList::iterator it = viewElementList->begin(); it != viewElementList->end(); ++it) {
        if (MatrixElement *el = dynamic_cast<MatrixElement*>(*it)) {
            addControlItem(el);
        }
    }

    RG_DEBUG << "PropertyControlRuler::init - Segment size: " << m_segment->size();

//    for (Segment::iterator it = m_viewSegment->begin();
//        it != m_viewSegment->end(); it++) {
//        addControlItem(*it);
//    }

    update();
}

void PropertyControlRuler::updateControlItems()
{
    PropertyControlItem *item;
    for (ControlItemList::iterator it = m_selectedItems.begin(); it != m_selectedItems.end(); ++it) {
        item = dynamic_cast <PropertyControlItem *> (*it);
        if (item) item->update();
    }
}

void PropertyControlRuler::updateSelection(std::vector <ViewElement*> *elementList)
{
    // Use base class fcn to set each item as not selected, clear the m_selectedItems lsit
    //  and create a new m_eventSelection member
    clearSelectedItems();

    // For now, simply clear the selected items list and build it afresh
    PropertyControlItem *item = 0;

//    for (ControlItemList::iterator it = m_selectedItems.begin(); it != m_selectedItems.end(); ++it) {
//        item = dynamic_cast <PropertyControlItem *> (*it);
//        if (item) item->setSelected(false);
//    }
//
//    m_selectedItems.clear();
    for (std::vector<ViewElement*>::iterator elit = elementList->begin(); elit != elementList->end();elit++) {
        for (ControlItemMap::iterator it = m_controlItemMap.begin(); it != m_controlItemMap.end(); ++it) {
            item = dynamic_cast<PropertyControlItem*>(it->second);
            if (item) {
                if (item->getElement() == (*elit)) {
                    break;
                } else {
                    item = 0;
                }
            }
        }
        if (!item) continue;

        item->setSelected(true);
        m_selectedItems.push_back(item);
        m_eventSelection->addEvent(item->getEvent());
    }

    update();
}

void PropertyControlRuler::slotHoveredOverNoteChanged(int evPitch, bool haveEvent, timeT evTime)
{
//    if (!m_eventSelection) return;
//
//    PropertyControlItem *item;
//
//    for (EventSelection::eventcontainer::iterator it =
//             m_eventSelection->getSegmentEvents().begin();
//         it != m_eventSelection->getSegmentEvents().end(); ++it) {
//
//        MatrixElement *element = 0;
//        ViewElementList::iterator vi = m_viewSegment->findEvent(*it);
//        if (vi != m_viewSegment->getViewElementList()->end()) {
//            element = static_cast<MatrixElement *>(*vi);
//        }
//        if (!element) continue;
//
//        for (ControlItemList::iterator it = m_controlItemList.begin(); it != m_controlItemList.end(); ++it) {
//            item = dynamic_cast<PropertyControlItem*>(*it);
//            if (item) {
//                if (item->getElement() == element) {
//                    break;
//                } else {
//                    item = 0;
//                }
//            }
//        }
//
//        if (!item) continue;
//
//        item->update();
//    }
    for (ControlItemList::iterator it = m_selectedItems.begin(); it != m_selectedItems.end(); ++it) {
        (*it)->update();
    }
}

void PropertyControlRuler::slotSetTool(const QString &matrixtoolname)
{
    ///TODO Write mechanism to select correct control tool for the given matrix tool
    QString controltoolname = "adjuster";
    ControlTool *tool = dynamic_cast<ControlTool *>(m_toolBox->getTool(controltoolname));
    if (!tool) return;
    if (m_currentTool) m_currentTool->stow();
    m_currentTool = tool;
    m_currentTool->ready();
//    emit toolChanged(name);
}

void PropertyControlRuler::elementAdded(const ViewSegment *, ViewElement *el)
{
    RG_DEBUG << "PropertyControlRuler::elementAdded()\n";

    if (el->event()->isa(Note::EventRestType))
        return ;

    RG_DEBUG << "PropertyControlRuler::eventAdded()";
//    addControlItem(el->event());
    if (MatrixElement *mel = dynamic_cast<MatrixElement*>(el)) {
        addControlItem(mel);
    }

    update();
//    double x = m_rulerScale->getXForTime(el->getViewAbsoluteTime());

//    new ControlItem(this, new ViewElementAdapter(el, getPropertyName()), int(x + m_viewSegmentOffset),
//                    int(m_rulerScale->getXForTime(el->getViewAbsoluteTime() +
//                                                  el->getViewDuration()) - x));
}

void PropertyControlRuler::elementRemoved(const ViewSegment *, ViewElement *el)
{
    RG_DEBUG << "PropertyControlRuler::eventRemoved()";
    for (ControlItemMap::iterator it = m_controlItemMap.begin(); it != m_controlItemMap.end(); ++it) {
        if (PropertyControlItem *item = dynamic_cast<PropertyControlItem*>(it->second)) {
            if (item->getEvent() == el->event()) {
//                m_controlItemList.erase(it);
//                m_selectedItems.remove(item);
//                delete item;
                removeControlItem(it);
                RG_DEBUG << "Control item erased";
                break;
            }
        }
    }

    update();
}

void PropertyControlRuler::viewSegmentDeleted(const ViewSegment *)
{
    m_viewSegment = 0;
    m_segment = 0;
}

void
PropertyControlRuler::endMarkerTimeChanged(const Segment *s, bool)
{
    timeT endMarkerTime = s->getEndMarkerTime();

    RG_DEBUG << "PropertyControlRuler::endMarkerTimeChanged() " << endMarkerTime << endl;

//    clearSelectedItems();
//    clear();
//    init();
}

void
PropertyControlRuler::mousePressEvent(QMouseEvent *e)
{
    RG_DEBUG << "PropertyControlRuler::mousePressEvent\n";

//    if (!m_propertyLineShowing) {
        if (e->button() == Qt::MidButton)
//            m_lastEventPos = inverseMapPoint(e->pos());
            m_lastEventPos = e->pos();

        ControlRuler::mousePressEvent(e); // send super

//        return ;
//    }

    // cancel control line mode
    //if (e->button() == Qt::RightButton) {
        //m_propertyLineShowing = false;
        //m_propertyLine->hide();

        //this->setCursor(Qt::arrowCursor);
        //return ;
    //}

    //if (e->button() == Qt::LeftButton) {
        //QPoint p = inverseMapPoint(e->pos());

        //m_propertyLine->show();
        //m_propertyLineX = p.x();
        //m_propertyLineY = p.y();
        //m_propertyLine->setPoints(m_propertyLineX, m_propertyLineY, m_propertyLineX, m_propertyLineY);
        //canvas()->update();
    //}
}

void
PropertyControlRuler::mouseReleaseEvent(QMouseEvent *e)
{
    ControlRuler::mouseReleaseEvent(e); // send super
}

void
PropertyControlRuler::mouseMoveEvent(QMouseEvent *e)
{
    RG_DEBUG << "PropertyControlRuler::mouseMoveEvent\n";

//    if (!m_propertyLineShowing) {
        // Don't send super if we're using the middle button
        //
        if (e->button() == Qt::MidButton) {
//            m_lastEventPos = inverseMapPoint(e->pos());
            m_lastEventPos = e->pos();
            return ;
        }

        ControlRuler::mouseMoveEvent(e); // send super
        //return ;
    //}

    //QPoint p = inverseMapPoint(e->pos());

    //m_propertyLine->setPoints(m_propertyLineX, m_propertyLineY, p.x(), p.y());
    //canvas()->update();
}

void PropertyControlRuler::contextMenuEvent(QContextMenuEvent* e)
{
    RG_DEBUG << "PropertyControlRuler::contextMenuEvent\n";

    // check if we actually have some control items
//    Q3CanvasItemList list = canvas()->allItems();
    bool haveItems = false;

//    Q3CanvasItemList::Iterator it = list.begin();
    for (ControlItemMap::iterator it = m_controlItemMap.begin(); it != m_controlItemMap.end(); ++it) {
        if (dynamic_cast<ControlItem*>(it->second)) {
            haveItems = true;
            break;
        }
    }

    RG_DEBUG << "PropertyControlRuler::contextMenuEvent : haveItems = "
    << haveItems << endl;

    emit stateChange("have_note_events_in_segment", haveItems);

    ControlRuler::contextMenuEvent(e);
}

//void
//PropertyControlRuler::drawPropertyLine(timeT startTime,
                                       //timeT endTime,
                                       //int startValue,
                                       //int endValue)
//{
    //if (startTime > endTime) {
        //std::swap(startTime, endTime);
        //std::swap(startValue, endValue);
    //}

    //RG_DEBUG << "PropertyControlRuler::drawPropertyLine - set velocity from "
    //<< startTime
    //<< " to " << endTime << endl;

    //// Add the "true" to catch Events overlapping this line
    ////
    //EventSelection selection(*m_segment, startTime, endTime, true);
    //PropertyPattern pattern = DecrescendoPattern;

    //bool haveNotes = selection.contains(Note::EventType);

    //if (haveNotes) {

        //SelectionPropertyCommand *command =
            //new SelectionPropertyCommand(&selection,
                                         //BaseProperties::VELOCITY,
                                         //pattern,
                                         //startValue,
                                         //endValue);

        //CommandHistory::getInstance()->addCommand(command);

    //} else {

        //RG_DEBUG << "PropertyControlRuler::drawPropertyLine - no notes in selection\n";

    //}
//}

void
PropertyControlRuler::selectAllProperties()
{
//    RG_DEBUG << "PropertyControlRuler::selectAllProperties" << endl;
//
//    /*
//    for(Segment::iterator i = m_segment.begin();
//                    i != m_segment.end(); ++i)
//        if (!m_eventSelection->contains(*i)) m_eventSelection->addEvent(*i);
//    */
//
//    clearSelectedItems();
//
////    Q3CanvasItemList l = canvas()->allItems();
////    for (Q3CanvasItemList::Iterator it = l.begin(); it != l.end(); ++it) {
//    for (ControlItemList::iterator it = m_controlItemList.begin(); it != m_controlItemList.end(); ++it) {
//        if (ControlItem *item = dynamic_cast<ControlItem*>(*it)) {
////            m_selectedItems << item;
//            m_selectedItems.push_back(item);
//            (*it)->setSelected(true);
////            ElementAdapter* adapter = item->getElementAdapter();
////            m_eventSelection->addEvent(adapter->getEvent());
//            m_eventSelection->addEvent(item->getEvent());
//        }
//    }
//
//    /*
//    m_eventSelection->addFromSelection(&selection);
//    for (Q3CanvasItemList::Iterator it=m_selectedItems.begin(); it!=m_selectedItems.end(); ++it) {
//        if (ControlItem *item = dynamic_cast<ControlItem*>(*it)) {
//
//            ElementAdapter* adapter = item->getElementAdapter();
//            m_eventSelection->addEvent(adapter->getEvent());
//            item->handleMouseButtonRelease(e);
//        }
//    }
//    */
//
//    emit stateChange("have_controller_item_selected", true);
}

}
