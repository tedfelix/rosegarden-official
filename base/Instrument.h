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

#ifndef _INSTRUMENT_H_
#define _INSTRUMENT_H_

#include "XmlExportable.h"
#include <string>

// An Instrument connects a Track (which itself contains
// a list of Segments) to a device that can play that
// Track.
//
//

namespace Rosegarden
{

typedef unsigned int InstrumentId;
typedef unsigned char MidiByte;

// Instrument number groups
//
const InstrumentId SystemInstrumentBase = 0;
const InstrumentId AudioInstrumentBase  = 1000;
const InstrumentId MidiInstrumentBase   = 2000;

const MidiByte MidiMaxValue = 127;
const MidiByte MidiMidValue = 64;
const MidiByte MidiMinValue = 0;

class Instrument : public XmlExportable
{
public:
    enum InstrumentType { Midi, Audio };

    Instrument(InstrumentId id,
               InstrumentType it,
               const std::string &name);

    Instrument(InstrumentId id,
               InstrumentType it,
               const std::string &name,
               MidiByte channel);

    ~Instrument();

    std::string getName() { return m_name; }
    InstrumentType getInstrumentType() { return m_type; }
    InstrumentId getID() const { return m_id; }

    MidiByte getMidiChannel() const { return m_midiChannel; }
    MidiByte getMidiTranspose() const { return m_midiTranspose; }
    InstrumentType getType() const { return m_type; }

    void setID(int id) { m_id = id; }
    void setName(const std::string &name) { m_name = name; }
    void setType(InstrumentType type) { m_type = type; }

    void setMidiChannel(MidiByte mC) { m_midiChannel = mC; }
    void setMidiTranspose(MidiByte mT) { m_midiTranspose = mT; }

    // Implementation of virtual function
    //
    virtual std::string toXmlString();

private:
    InstrumentId m_id;
    std::string m_name;
    InstrumentType m_type;
    
    MidiByte m_midiChannel;
    MidiByte m_midiTranspose;
    MidiByte m_programChange;
    MidiByte m_pan;
    MidiByte m_volume;

};

}

#endif // _INSTRUMENT_H_
