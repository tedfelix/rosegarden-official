/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "MidiTypes.h"

#include "Event.h"


namespace Rosegarden
{


//////////////////////////////////////////////////////////////////////
// PitchBend
//////////////////////////////////////////////////////////////////////

const std::string PitchBend::EventType = "pitchbend";

const PropertyName PitchBend::MSB = "msb";
const PropertyName PitchBend::LSB = "lsb";

Event *PitchBend::makeEvent(timeT absoluteTime, MidiByte msb, MidiByte lsb)
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<Int>(MSB, msb);
    e->set<Int>(LSB, lsb);

    return e;
}


//////////////////////////////////////////////////////////////////////
// Controller
//////////////////////////////////////////////////////////////////////

const std::string Controller::EventType = "controller";

const PropertyName Controller::NUMBER = "number";
const PropertyName Controller::VALUE  = "value";

Event *
Controller::makeEvent(timeT absoluteTime, MidiByte number, MidiByte value)
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<Int>(NUMBER, number);
    e->set<Int>(VALUE, value);

    return e;
}


//////////////////////////////////////////////////////////////////////
// Key Pressure
//////////////////////////////////////////////////////////////////////

const std::string KeyPressure::EventType = "keypressure";

const PropertyName KeyPressure::PITCH = "pitch";
const PropertyName KeyPressure::PRESSURE = "pressure";

Event *
KeyPressure::makeEvent(timeT absoluteTime, MidiByte pitch, MidiByte pressure)
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<Int>(PITCH, pitch);
    e->set<Int>(PRESSURE, pressure);

    return e;
}


//////////////////////////////////////////////////////////////////////
// Channel Pressure
//////////////////////////////////////////////////////////////////////

const std::string ChannelPressure::EventType = "channelpressure";

const PropertyName ChannelPressure::PRESSURE = "pressure";

Event *
ChannelPressure::makeEvent(timeT absoluteTime, MidiByte pressure)
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<Int>(PRESSURE, pressure);

    return e;
}


//////////////////////////////////////////////////////////////////////
// ProgramChange
//////////////////////////////////////////////////////////////////////

const std::string ProgramChange::EventType = "programchange";

const PropertyName ProgramChange::PROGRAM = "program";

Event *
ProgramChange::makeEvent(timeT absoluteTime, MidiByte program)
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<Int>(PROGRAM, program);

    return e;
}


//////////////////////////////////////////////////////////////////////
// SystemExclusive
//////////////////////////////////////////////////////////////////////

namespace
{

    unsigned char
    hexDigitToRaw(char c)
    {
        if (islower(c))
            c = toupper(c);

        if (isdigit(c))
            return c - '0';

        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;

        throw SystemExclusive::BadEncoding();
    }


}

const std::string SystemExclusive::EventType = "systemexclusive";

const PropertyName SystemExclusive::DATABLOCK = "datablock";

Event *
SystemExclusive::makeEvent(timeT absoluteTime, const std::string &rawData)
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<String>(DATABLOCK, toHex(rawData));

    return e;
}

std::string
SystemExclusive::toHex(std::string r)
{
    static char hexchars[] = "0123456789ABCDEF";
    std::string h;
    // For each character in r
    for (size_t i = 0; i < r.size(); ++i) {
        if (i > 0)
            h += ' ';
        unsigned char b = (unsigned char)r[i];
        h += hexchars[(b / 16) % 16];
        h += hexchars[b % 16];
    }
    return h;
}

std::string
SystemExclusive::toRaw(std::string hexData)
{
    std::string h;

    // remove whitespace
    for (size_t i = 0; i < hexData.size(); ++i) {
        if (!isspace(hexData[i]))
            h += hexData[i];
    }

    std::string rawData;

    for (size_t i = 0; i < h.size()/2; ++i) {
        unsigned char b = hexDigitToRaw(h[2*i]) * 16 + hexDigitToRaw(h[2*i+1]);
        rawData += b;
    }

    return rawData;
}


}
