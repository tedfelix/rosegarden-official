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

#ifndef RG_EVENTCONTROLITEM_H
#define RG_EVENTCONTROLITEM_H

#include "gui/rulers/ControlItem.h"

class QPolygon;

namespace Rosegarden {

class ControlRuler;
class ControllerEventAdapter;

class EventControlItem : public ControlItem
{
public:
    EventControlItem(ControlRuler* controlRuler,
                ControllerEventAdapter* eventAdapter,
                QPolygonF polygon);

    ~EventControlItem();
    
    virtual void update();
    virtual void updateFromEvent();

    void setY(float y);
    void updateSegment();
    void reconfigure();
    void reconfigure(float x,float y);

protected:
    void setEvent(Event*);

    //--------------- Data members ---------------------------------
    ControllerEventAdapter *m_eventAdapter;
    QPolygon m_symbol;
};

}

#endif
