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
                       const std::string &name):
    m_id(id),
    m_name(name),
    m_type(it),
    m_midiChannel(0),
    m_midiTranspose(MidiMidValue),
    m_programChange(0),
    m_pan(MidiMidValue),
    m_volume(0)

{
}

Instrument::Instrument(InstrumentId id,
                       InstrumentType it,
                       const std::string &name,
                       MidiByte channel):
    m_id(id),
    m_name(name),
    m_type(it),
    m_midiChannel(channel),
    m_midiTranspose(MidiMidValue),
    m_programChange(0),
    m_pan(MidiMidValue),
    m_volume(0)

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

    instrument << "<instrument id=\"";
    instrument << m_id << "\" type=\"";

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

    instrument << "\" name=\"" << m_name << "\"";
    instrument << "/>" << std::ends;

    return instrument.str();

}


}

