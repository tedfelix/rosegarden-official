// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#include "SoftSynthDevice.h"
#include "Instrument.h"

#include <cstdio>


#if (__GNUC__ < 3)
#include <strstream>
#define stringstream strstream
#else
#include <sstream>
#endif


namespace Rosegarden
{

SoftSynthDevice::SoftSynthDevice() :
    Device(0, "Default Soft Synth Device", Device::SoftSynth)
{
}

SoftSynthDevice::SoftSynthDevice(DeviceId id, const std::string &name) :
    Device(id, name, Device::SoftSynth)
{
}


SoftSynthDevice::SoftSynthDevice(const SoftSynthDevice &dev) :
    Device(dev.getId(), dev.getName(), dev.getType())
{
    // Copy the instruments
    //
    InstrumentList insList = dev.getAllInstruments();
    InstrumentList::iterator iIt = insList.begin();
    for (; iIt != insList.end(); iIt++)
        m_instruments.push_back(new Instrument(**iIt));
}

SoftSynthDevice::~SoftSynthDevice()
{
}


std::string
SoftSynthDevice::toXmlString()
{
    std::stringstream ssiDevice;
    InstrumentList::iterator iit;

    ssiDevice << "    <device id=\""  << m_id
                << "\" name=\""         << m_name
                << "\" type=\"softsynth\">" << std::endl;

    for (iit = m_instruments.begin(); iit != m_instruments.end(); ++iit)
        ssiDevice << (*iit)->toXmlString();

    ssiDevice << "    </device>"
#if (__GNUC__ < 3)
                << std::endl << std::ends;
#else
                << std::endl;
#endif

   return ssiDevice.str();
}


// Add to instrument list
//
void
SoftSynthDevice::addInstrument(Instrument *instrument)
{
    m_instruments.push_back(instrument);
}

}


