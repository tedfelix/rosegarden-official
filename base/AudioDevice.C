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

#include <cstdio>


#if (__GNUC__ < 3)
#include <strstream>
#define stringstream strstream
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
    char instNum[100];

    // for the moment just create an arbitrary number of audio tracks
    for (InstrumentId i = 0; i < 8; i++)
    {
        sprintf(instNum, "%d", i);
        std::string name = m_name.c_str() +
                           std::string(" #") +
                           std::string(instNum);

        m_instruments.push_back(
                new Instrument(i + AudioInstrumentBase,     // id
                               Instrument::Audio,           // type
                               name,                        // name
                               dynamic_cast<Device*>(this))); // parent device
    }

}

std::string
AudioDevice::toXmlString()
{
    std::stringstream audioDevice;
    InstrumentList::iterator iit;

    audioDevice << "    <device name=\""  << m_name
                << "\" type=\"audio\">" << std::endl;

    for (iit = m_instruments.begin(); iit != m_instruments.end(); iit++)
        audioDevice << (*iit)->toXmlString();

    audioDevice << "    </device>" << std::endl << std::ends;

   return audioDevice.str();
}


void
AudioDevice::addInstrument(Instrument*)
{
}


}


