// -*- c-basic-offset: 4 -*-


/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _MIDI_TYPES_H_
#define _MIDI_TYPES_H_

#include <list>

#include "Event.h"
#include "Instrument.h"

/*
 *
 * Internal representation of some very MIDI specific event types
 * that fall clearly outside of NotationTypes but will of course
 * still be represented in some way.
 *
 */

namespace Rosegarden 
{

class PitchBend
{
public:
    static const std::string EventType;
    static const int EventSubOrdering;

    static const PropertyName MSBPropertyName;
    static const PropertyName LSBPropertyName;

    /// Returned event is on heap; caller takes responsibility for ownership
    Event *getAsEvent(timeT absoluteTime) const;

    PitchBend(Rosegarden::MidiByte msb, Rosegarden::MidiByte lsb);
    ~PitchBend();

private:
    Rosegarden::MidiByte m_msb;
    Rosegarden::MidiByte m_lsb;
};

class Controller
{
public:
    static const std::string EventType;
    static const int EventSubOrdering;

    static const PropertyName Data1PropertyName;
    static const PropertyName Data2PropertyName;

    static const std::string UnspecifiedType;
    static const std::string Modulation;
    static const std::string Pan;

    Controller(const std::string &t,
               Rosegarden::MidiByte data1,
               Rosegarden::MidiByte data2);
    ~Controller();

    /// Returned event is on heap; caller takes responsibility for ownership
    Event *getAsEvent(timeT absoluteTime) const;

private:
    std::string m_type;

    Rosegarden::MidiByte m_data1;
    Rosegarden::MidiByte m_data2;

};


}


#endif
