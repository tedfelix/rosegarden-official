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

class Instrument : public XmlExportable
{
public:
    enum InstrumentType { Midi, Audio };

    Instrument();
    Instrument(const InstrumentId &id,
               const InstrumentType &it,
               const std::string &name);
    ~Instrument();

    std::string getName() { return m_name; }
    InstrumentType getInstrumentType() { return m_type; }
    InstrumentId getID() const { return m_id; }

    int getMidiChannel() { return m_midiChannel; }
    int getMidiTranspose() { return m_midiTranspose; }

    void setID(const int &id) { m_id = id; }
    void setName(const std::string &name) { m_name = name; }
    void setType(const InstrumentType &type) { m_type = type; }

    void setMidiChannel(const int &mC) { m_midiChannel = mC; }
    void setMidiTranspose(const int &mT) { m_midiTranspose = mT; }

    // Implementation of virtual function
    //
    virtual std::string toXmlString();

private:

    InstrumentId m_id;
    std::string m_name;
    InstrumentType m_type;
    
    int m_midiChannel;
    int m_midiTranspose;

};

}

#endif // _INSTRUMENT_H_
