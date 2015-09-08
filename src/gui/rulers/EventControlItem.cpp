/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/
#include <QPolygon>

#include "EventControlItem.h"
#include "ControllerEventAdapter.h"
#include "ControllerEventsRuler.h"
#include "ControlRuler.h"
#include "base/BaseProperties.h"
#include "base/Event.h"
#include "base/RulerScale.h"

namespace Rosegarden {

EventControlItem::EventControlItem(ControlRuler *controlRuler,
        ControllerEventAdapter *eventAdapter,
        QPolygonF polygon)
    : ControlItem(controlRuler,eventAdapter->getEvent(),polygon),
    m_eventAdapter(eventAdapter)
{
    m_symbol << QPoint(-5,0) << QPoint(0,-5) << QPoint(5,0) << QPoint(0,5);
}

EventControlItem::~EventControlItem()
{
}

void EventControlItem::update()
{
    updateFromEvent();
}

void EventControlItem::setEvent(Event* event)
{
    m_event = event;
    if (m_eventAdapter) delete m_eventAdapter;

    m_eventAdapter = new ControllerEventAdapter(event);
}

void EventControlItem::updateFromEvent()
{
    long value = 0;
    m_eventAdapter->getValue(value);

    double xscale = m_controlRuler->getXScale();
    reconfigure(m_controlRuler->getRulerScale()->getXForTime(m_eventAdapter->getTime())*xscale,
                m_controlRuler->valueToY(value));
}

void EventControlItem::setY(float y)
{
    if (y > 1.0) y = 1.0;
    if (y < 0) y = 0;

//    if (m_propertyname == BaseProperties::VELOCITY) {
//        m_element->reconfigure(y*MIDI_CONTROL_MAX_VALUE);
//        m_element->setSelected(true);
//        m_colour = DefaultVelocityColour::getInstance()->getColour(y*MIDI_CONTROL_MAX_VALUE);
//    }

    reconfigure(m_xstart,y);
}

void EventControlItem::reconfigure()
{
    reconfigure(m_xstart,m_y);
}

void EventControlItem::reconfigure(float x, float y)
{
    // Need to calculate the symbol each time to keep it the same size as we zoom
    double xscale = m_controlRuler->getXScale();
    double yscale = m_controlRuler->getYScale();

    // Clear the current polygon
    this->clear();
    for (QPolygon::iterator it=m_symbol.begin(); it !=m_symbol.end(); ++it)
    {
        *this << QPointF(x+it->x()*xscale,y+it->y()*yscale);
    }

    m_xend = x;
    m_y = y;
    // If this is a true reconfigure (and not part of an item creation)
    // m_xstart will be positive or zero and we need to move it in the
    // ControlItemMap
    if (m_xstart != -1.0 && m_xstart != x) {
        m_xstart = x;
        m_controlRuler->moveItem(this);
    } else {
        m_xstart = x;
    }
    
    ControlItem::reconfigure();
    
///@TODO ??
    ControlItem::update();

///@TODO ??
    m_controlRuler->update();
}

void EventControlItem::updateSegment()
{
    // This implementation for code simplicity - it is not efficient for control modification but I don't think this matters
    // If this item already has an event, erase it
    ControllerEventsRuler *ruler = static_cast<ControllerEventsRuler *> (m_controlRuler);

    if (m_event) {
        ruler->eraseEvent(m_event);
    }
    // Make a new event
    setEvent(ruler->insertEvent(m_xstart, m_y));

////    m_element->event()->set<Int>(m_propertyname,(int)(m_y*MIDI_CONTROL_MAX_VALUE));
//    long value = m_controlRuler->YToValue(m_y);
//    m_eventAdapter->setValue(value);
//    timeT time = m_controlRuler->getRulerScale()->getTimeForX(m_xstart);
//    ///TODO Update segment properly m_eventAdapter->setTime(time);
}

}
