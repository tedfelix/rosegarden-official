/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[PropertyControlRuler]"

#include <QMouseEvent>
#include "PropertyControlRuler.h"

#include "ControlRuler.h"
#include "ControlTool.h"
#include "ControlToolBox.h"
#include "ControlItem.h"
#include "PropertyControlItem.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
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
#include "gui/widgets/TextFloat.h"

#include <QColor>
#include <QPoint>
#include <QString>
#include <QWidget>
#include <QPainter>


namespace Rosegarden
{

PropertyControlRuler::PropertyControlRuler(PropertyName propertyName,
                                           ViewSegment *segment,
                                           RulerScale *rulerScale,
                                           QWidget *parent,
                                           const char */* name */) :
    ControlRuler(segment, rulerScale,
                 parent),
    m_propertyName(propertyName)
{

    setMenuName("property_ruler_menu");
    setViewSegment(segment);
}

void PropertyControlRuler::update()
{
//    ControlItemList tmplist;
//    for (ControlItemList::iterator it = m_visibleItems.begin(); it != m_visibleItems.end(); ++it) {
//        tmplist.push_back(*it);
//    }
    
//    for (ControlItemList::iterator it = tmplist.begin(); it != tmplist.end(); ++it) {
//        if (!(*it)->isSelected()) {
//            (*it)->update();
//            RG_DEBUG << "update(): ControlItem updated: " << hex << (long)(*it);
//        }
//    }

//    for (ControlItemList::iterator it = m_selectedItems.begin(); it != m_selectedItems.end(); ++it) {
//        (*it)->update();        
//        RG_DEBUG << "update(): ControlItem updated: " << hex << (long)(*it);
//    }

    ControlRuler::update();
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

    // Selected items that are currently visible so that they can be drawn
    // last - can't use m_selectedItems as this covers all selected, visible
    // or not
    ControlItemVector selectedVector;

    for (ControlItemList::iterator it = m_visibleItems.begin();
         it != m_visibleItems.end();
         ++it) {
        if (!(*it)->isSelected()) {
            brush.setColor((*it)->getColour().lighter());
            painter.setBrush(brush);
            painter.setPen(Qt::NoPen);
            painter.drawPolygon(mapItemToWidget(*it));

            painter.setPen(pen);
            painter.drawPolyline(mapItemToWidget(*it));
        } else {
            selectedVector.push_back(*it);
        }
    }

    for (ControlItemVector::iterator it = selectedVector.begin();
         it != selectedVector.end();
         ++it)
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
PropertyControlRuler::setViewSegment(ViewSegment *segment)
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
    return strtoqstr(getPropertyName());
}

void PropertyControlRuler::addControlItem2(ViewElement *el)
{
//    double x0 = el->getLayoutX();
//    double x1 = el->getWidth() + x0;
//    long val = 0;
//    el->event()->get<Rosegarden::Int>(getPropertyName(), val);
//    double y = (double) val / MIDI_CONTROL_MAX_VALUE;

//    size_t s = sizeof(MatrixElement);
//    RG_DEBUG << "addControlItem2(): sizeof(MatrixElement): " << s;

    QSharedPointer<PropertyControlItem> controlItem(
            new PropertyControlItem(this, getPropertyName(), el, QPolygonF()));
    controlItem->update();

    ControlRuler::addControlItem(controlItem);

//    m_controlItemList.push_back(controlItem);
}

//void PropertyControlRuler::addControlItem(Event *event)
//{
//    if (event->getType()!="note")
//        return;
//
//    RG_DEBUG << "addControlItem(): Event Type: " << event->getType();
//    RG_DEBUG << "  Event absolute time: " << event->getAbsoluteTime();
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
    if (!viewElementList) return;
    
    for (ViewElementList::iterator it = viewElementList->begin(); it != viewElementList->end(); ++it) {
//        if (MatrixElement *el = dynamic_cast<MatrixElement*>(*it)) {
        if ((*it)->event()->isa(Note::EventType))
            addControlItem2(*it);
//        }
    }
    

    RG_DEBUG << "init() - Segment size: " << m_segment->size();

//    for (Segment::iterator it = m_viewSegment->begin();
//        it != m_viewSegment->end(); it++) {
//        addControlItem(*it);
//    }

    update();
}

void PropertyControlRuler::updateSelection(std::vector <ViewElement*> *elementList)
{
    // Use base class fcn to set each item as not selected, clear the m_selectedItems lsit
    //  and create a new m_eventSelection member
    clearSelectedItems();

    // For now, simply clear the selected items list and build it afresh
    QSharedPointer<PropertyControlItem> item;

//    for (ControlItemList::iterator it = m_selectedItems.begin(); it != m_selectedItems.end(); ++it) {
//        item = dynamic_cast <PropertyControlItem *> (*it);
//        if (item) item->setSelected(false);
//    }
//
//    m_selectedItems.clear();
    for (std::vector<ViewElement*>::iterator elit = elementList->begin();
         elit != elementList->end();
         ++elit) {
        for (ControlItemMap::iterator it = m_controlItemMap.begin();
             it != m_controlItemMap.end();
             ++it) {
            item = qSharedPointerDynamicCast<PropertyControlItem>(it->second);

            if (item) {
                if (item->getElement() == (*elit)) {
                    break;
                } else {
                    item = nullptr;
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

void PropertyControlRuler::updateSelectedItems()
{
    for (ControlItemList::iterator it = m_selectedItems.begin(); it != m_selectedItems.end(); ++it) {
        (*it)->update();
    }
    
    update();
}

//void PropertyControlRuler::slotHoveredOverNoteChanged(int evPitch, bool haveEvent, timeT evTime)
//{
//    RG_DEBUG << "slotHoveredOverNoteChanged()";
//    for (ControlItemList::iterator it = m_selectedItems.begin(); it != m_selectedItems.end(); ++it) {
//        (*it)->update();
//    }
//    update();
//}

void PropertyControlRuler::slotSetTool(const QString &/* matrixtoolname */)
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
    if (el->event()->isa(Note::EventRestType))
        return ;

    RG_DEBUG << "elementAdded()";

//    addControlItem(el->event());
//    if (MatrixElement *mel = dynamic_cast<MatrixElement*>(el)) {
    addControlItem2(el);
//    }

    update();
//    double x = m_rulerScale->getXForTime(el->getViewAbsoluteTime());

//    new ControlItem(this, new ViewElementAdapter(el, getPropertyName()), int(x + m_viewSegmentOffset),
//                    int(m_rulerScale->getXForTime(el->getViewAbsoluteTime() +
//                                                  el->getViewDuration()) - x));
}

void PropertyControlRuler::elementRemoved(const ViewSegment *, ViewElement *el)
{
    if (el->event()->isa(Note::EventRestType))
        return ;

    RG_DEBUG << "elementRemoved()";

    for (ControlItemMap::iterator it = m_controlItemMap.begin();
         it != m_controlItemMap.end();
         ++it) {
        QSharedPointer<PropertyControlItem> item =
                qSharedPointerDynamicCast<PropertyControlItem>(it->second);

        if (item) {
            if (item->getEvent() == el->event()) {
//                m_controlItemList.erase(it);
//                m_selectedItems.remove(item);
//                delete item;
                eraseControlItem(it);
                RG_DEBUG << "elementRemoved(): Control item erased";
                break;
            }
        }
    }

    update();
}

void PropertyControlRuler::viewSegmentDeleted(const ViewSegment *)
{
    m_viewSegment = nullptr;
    m_segment = nullptr;
}

void
PropertyControlRuler::endMarkerTimeChanged(const Segment *s, bool)
{
    timeT endMarkerTime = s->getEndMarkerTime();

    RG_DEBUG << "endMarkerTimeChanged() " << endMarkerTime;

//    clearSelectedItems();
//    clear();
//    init();
}

void
PropertyControlRuler::mousePressEvent(QMouseEvent *e)
{
    RG_DEBUG << "mousePressEvent()";

    if (e->button() == Qt::MidButton)
        m_lastEventPos = e->pos();

    ControlRuler::mousePressEvent(e); // send super

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
    //RG_DEBUG << "mouseMoveEvent()";

    // Don't send super if we're using the middle button
    //
    if (e->button() == Qt::MidButton) {
        m_lastEventPos = e->pos();
        return;
    }

    ControlRuler::mouseMoveEvent(e); // send super

    //QPoint p = inverseMapPoint(e->pos());

    //m_propertyLine->setPoints(m_propertyLineX, m_propertyLineY, p.x(), p.y());
    //canvas()->update();
}

void PropertyControlRuler::contextMenuEvent(QContextMenuEvent* e)
{
    RG_DEBUG << "contextMenuEvent()";

    // check if we actually have some control items
    bool haveItems = false;

    for (ControlItemMap::iterator it = m_controlItemMap.begin();
         it != m_controlItemMap.end();
         ++it) {
        if (it->second) {
            haveItems = true;
            break;
        }
    }

    RG_DEBUG << "contextMenuEvent(): haveItems = " << haveItems;

    ControlRuler::contextMenuEvent(e);
}

void
PropertyControlRuler::selectAllProperties()
{
}

}
