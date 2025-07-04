/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MIDITYPES_H
#define RG_MIDITYPES_H

#include "Exception.h"
#include "MidiProgram.h"  // For MidiByte
#include "PropertyName.h"
#include "TimeT.h"

#include <string>

// Event factories for some very MIDI-specific event types
// that fall clearly outside of NotationTypes.

// See NotationTypes.h for more Event types.


namespace Rosegarden 
{


class Event;


//////////////////////////////////////////////////////////////////////

namespace PitchBend
{
    extern const std::string EventType;
    constexpr int EventSubOrdering = -5;

    extern const PropertyName MSB;
    extern const PropertyName LSB;

    /// Returned Event is on heap; caller takes responsibility for ownership.
    Event *makeEvent(timeT absoluteTime, MidiByte msb, MidiByte lsb);
}

//////////////////////////////////////////////////////////////////////

namespace Controller
{
    extern const char *EventType;
    constexpr int EventSubOrdering = -5;

    extern const PropertyName NUMBER;
    extern const PropertyName VALUE;

    /// Returned Event is on heap; caller takes responsibility for ownership.
    Event *makeEvent(timeT absoluteTime, MidiByte number, MidiByte value);

    const QString &getName(MidiByte number);
}

//////////////////////////////////////////////////////////////////////
// Registered Parameter Numbers
namespace RPN
{
    extern const char *EventType;
    constexpr int EventSubOrdering = -5;

    extern const PropertyName NUMBER;
    extern const PropertyName VALUE;

    /// Returned Event is on heap; caller takes responsibility for ownership.
    Event *makeEvent(timeT absoluteTime, int number, int value);
}

//////////////////////////////////////////////////////////////////////
// Non-Registered Parameter Numbers
namespace NRPN
{
    extern const char *EventType;
    constexpr int EventSubOrdering = -5;

    extern const PropertyName NUMBER;
    extern const PropertyName VALUE;

    /// Returned Event is on heap; caller takes responsibility for ownership.
    Event *makeEvent(timeT absoluteTime, int number, int value);
}

//////////////////////////////////////////////////////////////////////

namespace KeyPressure
{
    extern const std::string EventType;
    constexpr int EventSubOrdering = -5;

    extern const PropertyName PITCH;
    extern const PropertyName PRESSURE;

    /// Returned Event is on heap; caller takes responsibility for ownership.
    Event *makeEvent(timeT absoluteTime, MidiByte pitch, MidiByte pressure);
}

//////////////////////////////////////////////////////////////////////

namespace ChannelPressure
{
    extern const std::string EventType;
    constexpr int EventSubOrdering = -5;

    extern const PropertyName PRESSURE;

    /// Returned Event is on heap; caller takes responsibility for ownership.
    Event *makeEvent(timeT absoluteTime, MidiByte pressure);
};

//////////////////////////////////////////////////////////////////////

namespace ProgramChange
{
    extern const std::string EventType;
    constexpr int EventSubOrdering = -5;

    extern const PropertyName PROGRAM;

    /// Returned Event is on heap; caller takes responsibility for ownership.
    Event *makeEvent(timeT absoluteTime, MidiByte program);
}

//////////////////////////////////////////////////////////////////////

namespace SystemExclusive
{
    extern const std::string EventType;
    constexpr int EventSubOrdering = -5;

    struct BadEncoding : public Exception {
        BadEncoding() : Exception("Bad SysEx encoding") { }
    };

    extern const PropertyName DATABLOCK;

    /// Returned Event is on heap; caller takes responsibility for ownership.
    Event *makeEvent(timeT absoluteTime, const std::string &rawData);

    // ??? rename: rawToHex()
    std::string toHex(std::string rawData);
    // ??? rename: hexToRaw()
    std::string toRaw(std::string hexData);
}


}


#endif
