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

#include "Studio.h"
#include "MidiDevice.h"
#include "AudioDevice.h"
#include "Instrument.h"


namespace Rosegarden
{

Studio::Studio()
{
}

Studio::~Studio()
{
}


void
Studio::addDevice(const std::string &name, Device::DeviceType type)
{
    switch(type)
    {
        case Device::Midi:
            m_devices.push_back(new MidiDevice(name));
            break;

        case Device::Audio:
            m_devices.push_back(new AudioDevice(name));
            break;

        default:
            std::cerr << "Studio::addDevice() - unrecognised device"
                      << std::endl;
            break;
    }
}

void
Studio::addDevice(Device *device)
{
    m_devices.push_back(device);
}

InstrumentList
Studio::getInstruments()
{
    InstrumentList list, subList;

    std::vector<Device*>::iterator it;

    // Append lists
    //
    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        // get sub list
        subList = (*it)->getInstruments();

        // concetenate
        list.insert(list.end(), subList.begin(), subList.end());
    }

    return list;

}

Instrument*
Studio::getInstrumentByIndex(InstrumentId id)
{
    std::vector<Device*>::iterator it;
    InstrumentList list;
    InstrumentList::iterator iit;

    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        list = (*it)->getInstruments();

        for (iit = list.begin(); iit != list.end(); it++)
            if ((*iit)->getID() == id)
                return (*iit);
    }

    return 0;

}




}


