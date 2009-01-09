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

#include "ControllerEventAdapter.h"
#include "base/MidiTypes.h"
#include "misc/Debug.h"

namespace Rosegarden {

bool ControllerEventAdapter::getValue(long& val)
{
    if (m_event->getType() == Rosegarden::Controller::EventType)
    {
        return m_event->get<Rosegarden::Int>(Rosegarden::Controller::VALUE, val);
    }
    else if (m_event->getType() == Rosegarden::PitchBend::EventType)
    {
        long msb = 0, lsb = 0;
        m_event->get<Rosegarden::Int>(Rosegarden::PitchBend::MSB, msb);
        m_event->get<Rosegarden::Int>(Rosegarden::PitchBend::LSB, lsb);
        
        long value = msb;
        value <<= 7;
        value |= lsb;

        //RG_DEBUG << "PitchBend Get Value = " << value << endl;

        val = value;
        return true;
    }

    return false;
}

void ControllerEventAdapter::setValue(long val)
{
    if (m_event->getType() == Rosegarden::Controller::EventType)
    {
        m_event->set<Rosegarden::Int>(Rosegarden::Controller::VALUE, val);
    }
    else if (m_event->getType() == Rosegarden::PitchBend::EventType)
    {
        RG_DEBUG << "PitchBend Set Value = " << val << endl;

        int lsb = val & 0x7f;
        int msb = (val >> 7) & 0x7f;
        m_event->set<Rosegarden::Int>(Rosegarden::PitchBend::MSB, msb);
        m_event->set<Rosegarden::Int>(Rosegarden::PitchBend::LSB, lsb);
    }
}

timeT ControllerEventAdapter::getTime()
{
    return m_event->getAbsoluteTime();
}

timeT ControllerEventAdapter::getDuration()
{
    return m_event->getDuration();
}

}
