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

#include "Instrument.h"

#if (__GNUC__ < 3)
#include <strstream>
#define stringstream strstream
#else
#include <sstream>
#endif


namespace Rosegarden
{

Instrument::Instrument(InstrumentId id, InstrumentType it,
                       const std::string &name,
                       Device *device):
    m_id(id),
    m_name(name),
    m_type(it),
    m_channel(0),
    m_programChange(0),
    m_msb(0),
    m_lsb(0),
    m_transpose(MidiMidValue),
    m_pan(MidiMidValue),
    m_velocity(MidiMaxValue),
    m_device(device),
    m_sendBankSelect(false),
    m_sendProgramChange(false),
    m_sendPan(false),
    m_sendVelocity(false)

{
}

Instrument::Instrument(InstrumentId id,
                       InstrumentType it,
                       const std::string &name,
                       MidiByte channel,
                       Device *device):
    m_id(id),
    m_name(name),
    m_type(it),
    m_channel(channel),
    m_programChange(0),
    m_msb(0),
    m_lsb(0),
    m_transpose(MidiMidValue),
    m_pan(MidiMidValue),
    m_velocity(MidiMaxValue),
    m_device(device),
    m_sendBankSelect(false),
    m_sendProgramChange(false),
    m_sendPan(false),
    m_sendVelocity(false)

{
}

Instrument::~Instrument()
{
}

// Implementation of the virtual method to output this class
// as XML
//
//
std::string
Instrument::toXmlString()
{
    std::stringstream instrument;

    // only export if there's anything worth sending
    //
    //
    /*
    if (!m_sendBankSelect &&
        !m_sendProgramChange &&
        !m_sendPan &&
        !m_sendVelocity)
    {
        instrument << std::ends;
        return instrument.str();
    }
    */

    instrument << "        <instrument id=\"" << m_id;
    instrument << "\" channel=\"" << (int)m_channel;
    instrument << "\" type=\"";

    switch(m_type)
    {
        case Midi:
            instrument << "midi";
            break;

        case Audio:
            instrument << "audio";
            break;

        default:
            instrument << "unknown";
            break;
    }

    instrument << "\" name=\"" << m_name << "\">" << std::endl;

    if (m_sendBankSelect)
    {
        instrument << "        <bank msb=\"" << (int)m_msb;
        instrument << "\" lsb=\"" << (int)m_lsb << "\"/>" << std::endl;
    }

    if (m_sendProgramChange)
    {
        instrument << "        <program id=\""
                   << (int)m_programChange << "\"/>"
                   << std::endl;
    }
    
    if (m_sendPan)
    {
        instrument << "        <pan value=\""
                   << (int)m_pan << "\"/>" << std::endl;
    }

    if (m_sendVelocity)
    {
        instrument << "        <velocity value=\""
                   << (int)m_velocity << "\"/>" << std::endl;
    }

    instrument << "        </instrument>" << std::endl
               << std::endl << std::ends;

    return instrument.str();

}


}

