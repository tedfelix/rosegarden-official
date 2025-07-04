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

#include "MidiTypes.h"

#include "Event.h"


namespace Rosegarden
{


//////////////////////////////////////////////////////////////////////
// PitchBend
//////////////////////////////////////////////////////////////////////

const std::string PitchBend::EventType = "pitchbend";

const PropertyName PitchBend::MSB("msb");
const PropertyName PitchBend::LSB("lsb");

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

const char *Controller::EventType{"controller"};

const PropertyName Controller::NUMBER{"number"};
const PropertyName Controller::VALUE{"value"};

Event *
Controller::makeEvent(timeT absoluteTime, MidiByte number, MidiByte value)
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<Int>(NUMBER, number);
    e->set<Int>(VALUE, value);

    return e;
}

const QString &Controller::getName(const MidiByte number)
{
    // Going with a std::vector for raw speed.
    static std::vector<QString> controllerNames(128);
    static bool initialized{false};
    if (!initialized) {
        // ??? Should these be translated?
        controllerNames[0] = "Bank Select MSB";
        controllerNames[1] = "Mod Wheel MSB";
        controllerNames[2] = "Breath Controller MSB";
        controllerNames[4] = "Foot Pedal MSB";
        controllerNames[5] = "Portamento Time MSB";
        controllerNames[6] = "Data Entry MSB";
        controllerNames[7] = "Volume";
        controllerNames[8] = "Stereo Balance MSB";
        controllerNames[10] = "Pan";
        controllerNames[11] = "Expression";
        controllerNames[12] = "Effect 1 MSB";
        controllerNames[13] = "Effect 2 MSB";
        controllerNames[32] = "Bank Select LSB";
        controllerNames[33] = "Mod Wheel LSB";
        controllerNames[34] = "Breath Controller LSB";
        controllerNames[36] = "Foot Pedal LSB";
        controllerNames[37] = "Portamento Time LSB";
        controllerNames[38] = "Data Entry LSB";
        controllerNames[39] = "Volume LSB";
        controllerNames[40] = "Stereo Balance LSB";
        controllerNames[42] = "Pan LSB";
        controllerNames[43] = "Expression LSB";
        controllerNames[44] = "Effect 1 LSB";
        controllerNames[45] = "Effect 2 LSB";
        controllerNames[64] = "Sustain";
        controllerNames[65] = "Portamento On/Off";
        controllerNames[66] = "Sostenuto";
        controllerNames[67] = "Soft Pedal";
        controllerNames[68] = "Legato";
        controllerNames[69] = "Hold Pedal 2";
        controllerNames[91] = "Reverb";
        controllerNames[92] = "Tremolo";
        controllerNames[93] = "Chorus";
        controllerNames[94] = "Detuning";
        controllerNames[95] = "Phaser";
        controllerNames[96] = "Data +";
        controllerNames[97] = "Data -";
        controllerNames[98] = "NRPN LSB";
        controllerNames[99] = "NRPN MSB";
        controllerNames[100] = "RPN LSB";
        controllerNames[101] = "RPN MSB";
        controllerNames[120] = "Channel Mute";
        controllerNames[121] = "Reset All Controllers";
        controllerNames[122] = "Local On/Off";
        controllerNames[123] = "All MIDI Notes Off";
        controllerNames[124] = "Omni Off";
        controllerNames[125] = "Omni On";
        controllerNames[126] = "Mono On/Off";
        controllerNames[127] = "Poly On/Off";

        initialized = true;
    }

    return controllerNames[number];
}

//////////////////////////////////////////////////////////////////////
// RPN
//////////////////////////////////////////////////////////////////////

const char *RPN::EventType{"rpn"};

const PropertyName RPN::NUMBER{"number"};
const PropertyName RPN::VALUE{"value"};

Event *
RPN::makeEvent(timeT absoluteTime, int number, int value)
{
    Event *e = new Event(
            EventType,
            absoluteTime,
            0,  // duration
            EventSubOrdering);
    e->set<Int>(NUMBER, number);
    e->set<Int>(VALUE, value);

    return e;
}


//////////////////////////////////////////////////////////////////////
// NRPN
//////////////////////////////////////////////////////////////////////

const char *NRPN::EventType{"nrpn"};

const PropertyName NRPN::NUMBER{"number"};
const PropertyName NRPN::VALUE{"value"};

Event *
NRPN::makeEvent(timeT absoluteTime, int number, int value)
{
    Event *e = new Event(
            EventType,
            absoluteTime,
            0,  // duration
            EventSubOrdering);
    e->set<Int>(NUMBER, number);
    e->set<Int>(VALUE, value);

    return e;
}


//////////////////////////////////////////////////////////////////////
// Key Pressure
//////////////////////////////////////////////////////////////////////

const std::string KeyPressure::EventType = "keypressure";

const PropertyName KeyPressure::PITCH("pitch");
const PropertyName KeyPressure::PRESSURE("pressure");

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

const PropertyName ChannelPressure::PRESSURE("pressure");

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

const PropertyName ProgramChange::PROGRAM("program");

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

const PropertyName SystemExclusive::DATABLOCK("datablock");

Event *
SystemExclusive::makeEvent(timeT absoluteTime, const std::string &rawData)
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<String>(DATABLOCK, toHex(rawData));

    return e;
}

std::string
SystemExclusive::toHex(std::string rawData)
{
    static const char hexchars[] = "0123456789ABCDEF";
    std::string h;
    // For each character in rawData
    for (size_t i = 0; i < rawData.size(); ++i) {
        if (i > 0)
            h += ' ';
        unsigned char b = (unsigned char)rawData[i];
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
