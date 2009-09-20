/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "AudioDevice.h"
#include "Instrument.h"

#include <cstdio>

#include <sstream>


namespace Rosegarden
{

AudioDevice::AudioDevice():Device(0, "Default Audio Device", Device::Audio)
{
}

AudioDevice::AudioDevice(DeviceId id, const std::string &name):
    Device(id, name, Device::Audio)
{
}


AudioDevice::AudioDevice(const AudioDevice &dev):
    Device(dev.getId(), dev.getName(), dev.getType())
{
    // Copy the instruments
    //
    InstrumentList insList = dev.getAllInstruments();
    InstrumentList::iterator iIt = insList.begin();
    for (; iIt != insList.end(); iIt++)
        m_instruments.push_back(new Instrument(**iIt));
}

AudioDevice::~AudioDevice()
{
}


std::string
AudioDevice::toXmlString()
{
    std::stringstream audioDevice;
    InstrumentList::iterator iit;

    audioDevice << "    <device id=\""  << m_id
                << "\" name=\""         << m_name
                << "\" type=\"audio\">" << std::endl;

    for (iit = m_instruments.begin(); iit != m_instruments.end(); ++iit)
        audioDevice << (*iit)->toXmlString();

    audioDevice << "    </device>"
                << std::endl;

   return audioDevice.str();
}


// Add to instrument list
//
void
AudioDevice::addInstrument(Instrument *instrument)
{
    m_instruments.push_back(instrument);
}

// For the moment just use the first audio Instrument
//
InstrumentId
AudioDevice::getPreviewInstrument()
{
    return AudioInstrumentBase;
}

}


