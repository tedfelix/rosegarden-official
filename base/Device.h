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

#include "XmlExportable.h"
#include <string>
#include <vector>

#ifndef _DEVICE_H_
#define _DEVICE_H_

// A Device can query underlying hardware/sound APIs to
// generate a list of Instruments.
//

namespace Rosegarden
{

class Instrument;
typedef std::vector<Instrument *> InstrumentList;

class Device : public XmlExportable
{
public:
    typedef enum 
    {
        Midi,
        Audio
    } DeviceType;

    Device(const std::string &name, DeviceType type):
        m_name(name), m_type(type) { }

    virtual ~Device() {;}

    DeviceType getType() const { return m_type; }
    std::string getName() const { return m_name; }

    // Accessing instrument lists - Devices should only
    // show the world what they want it to see
    //
    virtual void createInstruments() = 0;
    virtual void addInstrument(Instrument*) = 0;

    // Two functions - one to return all Instruments on a
    // Device - one to return all Instruments that a user
    // is allowed to select (Presentation Instruments).
    //
    virtual InstrumentList& getAllInstruments() = 0;
    virtual InstrumentList& getPresentationInstruments() = 0;

protected:
    InstrumentList     m_instruments;
    std::string        m_name;
    DeviceType         m_type;

};

}

#endif // _DEVICE_H_
