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

#include "AudioDevice.h"
#include "Instrument.h"

#if (__GNUC__ < 3)
#include <strstream>
#else
#include <sstream>
#endif


namespace Rosegarden
{

AudioDevice::AudioDevice():Device("Default Audio Device", Device::Audio)
{
   createInstruments();
}

AudioDevice::AudioDevice(const std::string &name):Device(name, Device::Audio)
{
   createInstruments();
}


AudioDevice::~AudioDevice()
{
}

void
AudioDevice::createInstruments()
{

#if (__GNUC__ < 3)
    std::ostrstream instrumentName;
#else
    std::ostringstream instrumentName;
#endif

    // for the moment just create an arbitrary number of audio tracks
    for (InstrumentId i = 0; i < 8; i++)
    {
        instrumentName << m_name.c_str() << " #" << i << std::ends;

        m_instruments.push_back(
                new Instrument(i, Instrument::Audio, instrumentName.str()));
    }

}

}


