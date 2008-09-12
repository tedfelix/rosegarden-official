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


#include <QMouseEvent>
#include <Q3Canvas>
#include <Q3CanvasItemList>
#include <Q3CanvasLine>
#include "PropertyControlRuler.h"

#include "ControlRuler.h"
#include "ControlItem.h"
#include "ViewElementAdapter.h"
#include "misc/Debug.h"
#include "base/BaseProperties.h"
#include "base/NotationTypes.h"
#include "base/PropertyName.h"
#include "base/RulerScale.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "base/Staff.h"
#include "base/ViewElement.h"
#include "commands/edit/SelectionPropertyCommand.h"
#include "gui/general/EditViewBase.h"
#include "gui/widgets/TextFloat.h"
#include "gui/general/LinedStaff.h"
#include <Q3Canvas>
#include <QColor>
#include <QPoint>
#include <QString>
#include <QWidget>


namespace Rosegarden
{

PropertyControlRuler::PropertyControlRuler(PropertyName propertyName,
        Staff* staff,
        RulerScale* rulerScale,
        EditViewBase* parentView,
        Q3Canvas* c, QWidget* parent,
        const char* name, WFlags f) :
        ControlRuler(&(staff->getSegment()), rulerScale,
                     parentView, c, parent, name, f),
        m_propertyName(propertyName),
        m_staff(staff),
        m_propertyLine(new Q3CanvasLine(canvas())),
        m_propertyLineShowing(false),
        m_propertyLineX(0),
        m_propertyLineY(0)
{
    m_staff->addObserver(this);
    m_propertyLine->setZ(1000); // bring to front

    setMenuName("property_ruler_menu");
    drawBackground();
    init();
}

void
PropertyControlRuler::setStaff(Staff *staff)
{
    RG_DEBUG << "PropertyControlRuler::setStaff(" << staff << ")" << endl;

    m_staff->removeObserver(this);
    m_segment->removeObserver(this);
    m_staff = staff;
    m_segment = &m_staff->getSegment();
    m_staff->addObserver(this);
    m_segment->addObserver(this);

    //!!! need to delete the control items here

    drawBackground();
    init();
}

void
PropertyControlRuler::drawBackground()
{
    // Draw some minimum and maximum controller value guide lines
    //
    Q3CanvasLine *topLine = new Q3CanvasLine(canvas());
    Q3CanvasLine *topQLine = new Q3CanvasLine(canvas());
    Q3CanvasLine *midLine = new Q3CanvasLine(canvas());
    Q3CanvasLine *botQLine = new Q3CanvasLine(canvas());
    Q3CanvasLine *bottomLine = new Q3CanvasLine(canvas());
    //m_controlLine->setPoints(m_controlLineX, m_controlLineY, m_controlLineX, m_controlLineY);
    int cHeight = canvas()->height();
    int cWidth = canvas()->width();

    topLine->setPen(QColor(127, 127, 127));
    topLine->setPoints(0, 0, cWidth, 0);
    topLine->setZ( -10);
    topLine->show();

    topQLine->setPen(QColor(192, 192, 192));
    topQLine->setPoints(0, cHeight / 4, cWidth, cHeight / 4);
    topQLine->setZ( -10);
    topQLine->show();

    midLine->setPen(QColor(127, 127, 127));
    midLine->setPoints(0, cHeight / 2, cWidth, cHeight / 2);
    midLine->setZ( -10);
    midLine->show();

    botQLine->setPen(QColor(192, 192, 192));
    botQLine->setPoints(0, 3*cHeight / 4, cWidth, 3*cHeight / 4);
    botQLine->setZ( -10);
    botQLine->show();

    bottomLine->setPen(QColor(127, 127, 127));
    bottomLine->setPoints(0, cHeight - 1, cWidth, cHeight - 1);
    bottomLine->setZ( -10);
    bottomLine->show();
}

PropertyControlRuler::~PropertyControlRuler()
{
    if (m_staff) {
        m_staff->removeObserver(this);
    }
}

QString PropertyControlRuler::getName()
{
    return getPropertyName().c_str();
}

void PropertyControlRuler::init()
{
    ViewElementList* viewElementList = m_staff->getViewElementList();

    LinedStaff* lStaff = dynamic_cast<LinedStaff*>(m_staff);

    if (lStaff)
        m_staffOffset = lStaff->getX();

    for (ViewElementList::iterator i = viewElementList->begin();
            i != viewElementList->end(); ++i) {

        if ((*i)->event()->isa(Note::EventRestType))
            continue;

        double x = m_rulerScale->getXForTime((*i)->getViewAbsoluteTime());
        new ControlItem(this, new ViewElementAdapter(*i, getPropertyName()), int(x + m_staffOffset),
                        int(m_rulerScale->getXForTime((*i)->getViewAbsoluteTime() +
                                                      (*i)->getViewDuration()) - x));

    }
}

void PropertyControlRuler::elementAdded(const Staff *, ViewElement *el)
{
    RG_DEBUG << "PropertyControlRuler::elementAdded()\n";

    if (el->event()->isa(Note::EventRestType))
        return ;

    double x = m_rulerScale->getXForTime(el->getViewAbsoluteTime());

    new ControlItem(this, new ViewElementAdapter(el, getPropertyName()), int(x + m_staffOffset),
                    int(m_rulerScale->getXForTime(el->getViewAbsoluteTime() +
                                                  el->getViewDuration()) - x));
}

void PropertyControlRuler::elementRemoved(const Staff *, ViewElement *el)
{
    RG_DEBUG << "PropertyControlRuler::elementRemoved(\n";

    clearSelectedItems();

    Q3CanvasItemList allItems = canvas()->allItems();

    for (Q3CanvasItemList::Iterator it = allItems.begin(); it != allItems.end(); ++it) {
        if (ControlItem *item = dynamic_cast<ControlItem*>(*it)) {
            ViewElementAdapter * adapter = dynamic_cast<ViewElementAdapter*>(item->getElementAdapter());
            if (adapter->getViewElement() == el) {
                delete item;
                break;
            }
        }
    }
}

void PropertyControlRuler::staffDeleted(const Staff *)
{
    m_staff = 0;
}

void
PropertyControlRuler::endMarkerTimeChanged(const Segment *s, bool)
{
    timeT endMarkerTime = s->getEndMarkerTime();

    RG_DEBUG << "PropertyControlRuler::endMarkerTimeChanged() " << endMarkerTime << endl;

    clearSelectedItems();

    clear();
    init();
}

void PropertyControlRuler::computeStaffOffset()
{
    LinedStaff* lStaff = dynamic_cast<LinedStaff*>(m_staff);
    if (lStaff)
        m_staffOffset = lStaff->getX();
}

void PropertyControlRuler::startPropertyLine()
{
    RG_DEBUG << "PropertyControlRuler::startPropertyLine\n";
    m_propertyLineShowing = true;
    this->setCursor(Qt::pointingHandCursor);
}

void
PropertyControlRuler::contentsMousePressEvent(QMouseEvent *e)
{
    RG_DEBUG << "PropertyControlRuler::contentsMousePressEvent\n";

    if (!m_propertyLineShowing) {
        if (e->button() == MidButton)
            m_lastEventPos = inverseMapPoint(e->pos());

        ControlRuler::contentsMousePressEvent(e); // send super

        return ;
    }

    // cancel control line mode
    if (e->button() == RightButton) {
        m_propertyLineShowing = false;
        m_propertyLine->hide();

        this->setCursor(Qt::arrowCursor);
        return ;
    }

    if (e->button() == LeftButton) {
        QPoint p = inverseMapPoint(e->pos());

        m_propertyLine->show();
        m_propertyLineX = p.x();
        m_propertyLineY = p.y();
        m_propertyLine->setPoints(m_propertyLineX, m_propertyLineY, m_propertyLineX, m_propertyLineY);
        canvas()->update();
    }
}

void
PropertyControlRuler::contentsMouseReleaseEvent(QMouseEvent *e)
{
    RG_DEBUG << "PropertyControlRuler::contentsMouseReleaseEvent\n";

    /*
    if (m_propertyLineShowing)
    {
        this->setCursor(Qt::arrowCursor);
        m_propertyLineShowing = false;
        canvas()->update();
    }
    */

    if (!m_propertyLineShowing) {
        /*
        if (e->button() == MidButton)
            insertControllerEvent();
            */

        ControlRuler::contentsMouseReleaseEvent(e); // send super
        return ;
    } else {
        QPoint p = inverseMapPoint(e->pos());

        timeT startTime = m_rulerScale->getTimeForX(m_propertyLineX);
        timeT endTime = m_rulerScale->getTimeForX(p.x());

        long startValue = heightToValue(m_propertyLineY - canvas()->height());
        long endValue = heightToValue(p.y() - canvas()->height());

        RG_DEBUG << "PropertyControlRuler::contentsMouseReleaseEvent - "
        << "starttime = " << startTime
        << ", endtime = " << endTime
        << ", startValue = " << startValue
        << ", endValue = " << endValue
        << endl;

        drawPropertyLine(startTime, endTime, startValue, endValue);

        m_propertyLineShowing = false;
        m_propertyLine->hide();
        this->setCursor(Qt::arrowCursor);
        canvas()->update();
    }
}

void
PropertyControlRuler::contentsMouseMoveEvent(QMouseEvent *e)
{
    RG_DEBUG << "PropertyControlRuler::contentsMouseMoveEvent\n";

    if (!m_propertyLineShowing) {
        // Don't send super if we're using the middle button
        //
        if (e->button() == MidButton) {
            m_lastEventPos = inverseMapPoint(e->pos());
            return ;
        }

        ControlRuler::contentsMouseMoveEvent(e); // send super
        return ;
    }

    QPoint p = inverseMapPoint(e->pos());

    m_propertyLine->setPoints(m_propertyLineX, m_propertyLineY, p.x(), p.y());
    canvas()->update();
}

void PropertyControlRuler::contentsContextMenuEvent(QContextMenuEvent* e)
{
    RG_DEBUG << "PropertyControlRuler::contentsContextMenuEvent\n";

    // check if we actually have some control items
    Q3CanvasItemList list = canvas()->allItems();
    bool haveItems = false;

    Q3CanvasItemList::Iterator it = list.begin();
    for (; it != list.end(); ++it) {
        if (dynamic_cast<ControlItem*>(*it)) {
            haveItems = true;
            break;
        }
    }

    RG_DEBUG << "PropertyControlRuler::contentsContextMenuEvent : haveItems = "
    << haveItems << endl;

    emit stateChange("have_note_events_in_segment", haveItems);

    ControlRuler::contentsContextMenuEvent(e);
}

void
PropertyControlRuler::drawPropertyLine(timeT startTime,
                                       timeT endTime,
                                       int startValue,
                                       int endValue)
{
    if (startTime > endTime) {
        std::swap(startTime, endTime);
        std::swap(startValue, endValue);
    }

    RG_DEBUG << "PropertyControlRuler::drawPropertyLine - set velocity from "
    << startTime
    << " to " << endTime << endl;

    // Add the "true" to catch Events overlapping this line
    //
    EventSelection selection(*m_segment, startTime, endTime, true);
    PropertyPattern pattern = DecrescendoPattern;

    bool haveNotes = selection.contains(Note::EventType);

    if (haveNotes) {

        SelectionPropertyCommand *command =
            new SelectionPropertyCommand(&selection,
                                         BaseProperties::VELOCITY,
                                         pattern,
                                         startValue,
                                         endValue);

        m_parentEditView->addCommandToHistory(command);

    } else {

        RG_DEBUG << "PropertyControlRuler::drawPropertyLine - no notes in selection\n";

    }
}

void
PropertyControlRuler::selectAllProperties()
{
    RG_DEBUG << "PropertyControlRuler::selectAllProperties" << endl;

    /*
    for(Segment::iterator i = m_segment.begin();
                    i != m_segment.end(); ++i)
        if (!m_eventSelection->contains(*i)) m_eventSelection->addEvent(*i);
    */

    clearSelectedItems();

    Q3CanvasItemList l = canvas()->allItems();
    for (Q3CanvasItemList::Iterator it = l.begin(); it != l.end(); ++it) {
        if (ControlItem *item = dynamic_cast<ControlItem*>(*it)) {
            m_selectedItems << item;
            (*it)->setSelected(true);
            ElementAdapter* adapter = item->getElementAdapter();
            m_eventSelection->addEvent(adapter->getEvent());
        }
    }

    /*
    m_eventSelection->addFromSelection(&selection);
    for (Q3CanvasItemList::Iterator it=m_selectedItems.begin(); it!=m_selectedItems.end(); ++it) {
        if (ControlItem *item = dynamic_cast<ControlItem*>(*it)) {

            ElementAdapter* adapter = item->getElementAdapter();
            m_eventSelection->addEvent(adapter->getEvent());
            item->handleMouseButtonRelease(e);
        }
    }
    */

    emit stateChange("have_controller_item_selected", true);
}

}
