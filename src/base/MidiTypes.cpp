/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */


/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "base/MidiTypes.h"

namespace Rosegarden
{

static MidiByte getByte(const Event &e, const PropertyName &name) {
    long value = -1;
    try {
	value = e.get<Int>(name);
    } catch (...) { }
    if (value < 0 || value > 255) throw MIDIValueOutOfRange(name.getName());
    return MidiByte(value);
}

//////////////////////////////////////////////////////////////////////
// PitchBend
//////////////////////////////////////////////////////////////////////

const std::string PitchBend::EventType = "pitchbend";
const int PitchBend::EventSubOrdering = -5;

const PropertyName PitchBend::MSB = "msb";
const PropertyName PitchBend::LSB = "lsb";

PitchBend::PitchBend(Rosegarden::MidiByte msb,
                     Rosegarden::MidiByte lsb) :
    m_msb(msb),
    m_lsb(lsb)
{
}

PitchBend::PitchBend(const Event &e)
{
    if (e.getType() != EventType) {
        throw Event::BadType("PitchBend model event", EventType, e.getType());
    }
    m_msb = getByte(e, MSB);
    m_lsb = getByte(e, LSB);
}

PitchBend::~PitchBend()
{
}

Event*
PitchBend::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<Int>(MSB, (long)m_msb);
    e->set<Int>(LSB, (long)m_lsb);
    return e;
}


//////////////////////////////////////////////////////////////////////
// Controller
//////////////////////////////////////////////////////////////////////

const std::string Controller::EventType = "controller";
const int Controller::EventSubOrdering = -5;

const PropertyName Controller::NUMBER = "number";
const PropertyName Controller::VALUE  = "value";

Controller::Controller(Rosegarden::MidiByte number,
                       Rosegarden::MidiByte value):
    m_number(number),
    m_value(value)
{
}

Controller::Controller(const Event &e)
{
    if (e.getType() != EventType) {
        throw Event::BadType("Controller model event", EventType, e.getType());
    }
    m_number = getByte(e, NUMBER);
    m_value = getByte(e, VALUE);
}

Controller::~Controller()
{
}

Event*
Controller::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<Int>(NUMBER, (long)m_number);
    e->set<Int>(VALUE, (long)m_value);
    return e;
}


//////////////////////////////////////////////////////////////////////
// Key Pressure
//////////////////////////////////////////////////////////////////////

const std::string KeyPressure::EventType = "keypressure";
const int KeyPressure::EventSubOrdering = -5;

const PropertyName KeyPressure::PITCH = "pitch";
const PropertyName KeyPressure::PRESSURE = "pressure";

KeyPressure::KeyPressure(Rosegarden::MidiByte pitch,
                         Rosegarden::MidiByte pressure):
    m_pitch(pitch),
    m_pressure(pressure)
{
}

KeyPressure::KeyPressure(const Event &e)
{
    if (e.getType() != EventType) {
        throw Event::BadType("KeyPressure model event", EventType, e.getType());
    }
    m_pitch = getByte(e, PITCH);
    m_pressure = getByte(e, PRESSURE);
}

KeyPressure::~KeyPressure()
{
}

Event*
KeyPressure::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<Int>(PITCH, (long)m_pitch);
    e->set<Int>(PRESSURE, (long)m_pressure);
    return e;
}


//////////////////////////////////////////////////////////////////////
// Channel Pressure
//////////////////////////////////////////////////////////////////////

const std::string ChannelPressure::EventType = "channelpressure";
const int ChannelPressure::EventSubOrdering = -5;

const PropertyName ChannelPressure::PRESSURE = "pressure";

ChannelPressure::ChannelPressure(Rosegarden::MidiByte pressure):
    m_pressure(pressure)
{
}

ChannelPressure::ChannelPressure(const Event &e)
{
    if (e.getType() != EventType) {
        throw Event::BadType("ChannelPressure model event", EventType, e.getType());
    }
    m_pressure = getByte(e, PRESSURE);
}

ChannelPressure::~ChannelPressure()
{
}

Event*
ChannelPressure::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<Int>(PRESSURE, (long)m_pressure);
    return e;
}


//////////////////////////////////////////////////////////////////////
// ProgramChange
//////////////////////////////////////////////////////////////////////

const std::string ProgramChange::EventType = "programchange";
const int ProgramChange::EventSubOrdering = -5;

const PropertyName ProgramChange::PROGRAM = "program";

ProgramChange::ProgramChange(Rosegarden::MidiByte program):
    m_program(program)
{
}

ProgramChange::ProgramChange(const Event &e)
{
    if (e.getType() != EventType) {
        throw Event::BadType("ProgramChange model event", EventType, e.getType());
    }
    m_program = getByte(e, PROGRAM);
}

ProgramChange::~ProgramChange()
{
}

Event*
ProgramChange::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<Int>(PROGRAM, (long)m_program);
    return e;
}


//////////////////////////////////////////////////////////////////////
// SystemExclusive
//////////////////////////////////////////////////////////////////////

const std::string SystemExclusive::EventType = "systemexclusive";
const int SystemExclusive::EventSubOrdering = -5;

const PropertyName SystemExclusive::DATABLOCK = "datablock";

SystemExclusive::SystemExclusive(std::string rawData) :
    m_rawData(rawData)
{
}

SystemExclusive::SystemExclusive(const Event &e)
{
    if (e.getType() != EventType) {
        throw Event::BadType("SystemExclusive model event", EventType, e.getType());
    }
    std::string datablock;
    e.get<String>(DATABLOCK, datablock);
    m_rawData = toRaw(datablock);
}

SystemExclusive::~SystemExclusive()
{
}

Event*
SystemExclusive::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    std::string hex(toHex(m_rawData));
    e->set<String>(DATABLOCK, hex);
    return e;
}

std::string
SystemExclusive::toHex(std::string r)
{
    static char hexchars[] = "0123456789ABCDEF";
    std::string h;
    for (size_t i = 0; i < r.size(); ++i) {
	if (i > 0) h += ' ';
	unsigned char b = (unsigned char)r[i];
	h += hexchars[(b / 16) % 16];
	h += hexchars[b % 16];
    }
    return h;
}

std::string
SystemExclusive::toRaw(std::string rh)
{
    std::string r;
    std::string h;

    // remove whitespace
    for (size_t i = 0; i < rh.size(); ++i) {
	if (!isspace(rh[i])) h += rh[i];
    }

    for (size_t i = 0; i < h.size()/2; ++i) {
	unsigned char b = toRawNibble(h[2*i]) * 16 + toRawNibble(h[2*i+1]);
	r += b;
    }

    return r;
}

unsigned char
SystemExclusive::toRawNibble(char c)
{
    if (islower(c)) c = toupper(c);
    if (isdigit(c)) return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    throw BadEncoding();
}

bool
SystemExclusive::isHex(std::string rh)
{
    // arf
    try {
	std::string r = toRaw(rh);
    } catch (BadEncoding) {
	return false;
    }
    return true;
}
	

}

