/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
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
#include <QString>

namespace Rosegarden
{

AudioDevice::AudioDevice() : 
    Device(0, "Default Audio Device", Device::Audio)
{
    createInstruments();
}

AudioDevice::AudioDevice(DeviceId id, const std::string &name) :
    Device(id, name, Device::Audio)
{
    createInstruments();
}


AudioDevice::AudioDevice(const AudioDevice &dev):
    Device(dev.getId(), dev.getName(), dev.getType())
{
    // Copy the instruments
    //
    InstrumentList insList = dev.getAllInstruments();
    InstrumentList::iterator iIt = insList.begin();
    for (; iIt != insList.end(); ++iIt)
        m_instruments.push_back(new Instrument(**iIt));
}

AudioDevice::~AudioDevice()
{
}
    
void
AudioDevice::createInstruments()
{
    for (uint i = 0; i < AudioInstrumentCount; ++i) {
	Instrument *instrument = new Instrument
	    (AudioInstrumentBase + i, Instrument::Audio, "", i, this);
        addInstrument(instrument);
    }
    renameInstruments();
}

void
AudioDevice::renameInstruments()
{
    for (uint i = 0; i < AudioInstrumentCount; ++i) {
        m_instruments[i]->setName
            (QString("%1 #%2").arg(getName().c_str()).arg(i+1).toUtf8().data());
    }
}

std::string
AudioDevice::toXmlString() const
{
    std::stringstream audioDevice;

    audioDevice << "    <device id=\""  << m_id
                << "\" name=\""         << m_name
                << "\" type=\"audio\">" << std::endl;

    InstrumentList::const_iterator iit;
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
