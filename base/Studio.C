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

#include <iostream>

#include "Studio.h"
#include "MidiDevice.h"
#include "AudioDevice.h"
#include "Instrument.h"

#if (__GNUC__ < 3)
#include <strstream>
#define stringstream strstream
#else
#include <sstream>
#endif


namespace Rosegarden
{

Studio::Studio()
{
}

Studio::~Studio()
{
}


void
Studio::addDevice(const std::string &name,
                  DeviceId id,
                  Device::DeviceType type)
{
    switch(type)
    {
        case Device::Midi:
            m_devices.push_back(new MidiDevice(id, name));
            break;

        case Device::Audio:
            m_devices.push_back(new AudioDevice(id, name));
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
Studio::getAllInstruments()
{
    InstrumentList list, subList;

    std::vector<Device*>::iterator it;

    // Append lists
    //
    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        // get sub list
        subList = (*it)->getAllInstruments();

        // concetenate
        list.insert(list.end(), subList.begin(), subList.end());
    }

    return list;

}

InstrumentList
Studio::getPresentationInstruments()
{
    InstrumentList list, subList;

    std::vector<Device*>::iterator it;

    // Append lists
    //
    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        // get sub list
        subList = (*it)->getPresentationInstruments();

        // concetenate
        list.insert(list.end(), subList.begin(), subList.end());
    }

    return list;

}

Instrument*
Studio::getInstrumentById(InstrumentId id)
{
    std::vector<Device*>::iterator it;
    InstrumentList list;
    InstrumentList::iterator iit;

    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        list = (*it)->getAllInstruments();

        for (iit = list.begin(); iit != list.end(); iit++)
            if ((*iit)->getID() == id)
                return (*iit);
    }

    return 0;

}

// From a user selection (from a "Presentation" list) return
// the matching Instrument
//
Instrument*
Studio::getInstrumentFromList(int index)
{
    std::vector<Device*>::iterator it;
    InstrumentList list;
    InstrumentList::iterator iit;
    int count = 0;

    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        list = (*it)->getPresentationInstruments();

        for (iit = list.begin(); iit != list.end(); iit++)
        {
            if (count == index)
                return (*iit);

            count++;
        }
    }

    return 0;

}



// Clear down the devices and the Instruments
void
Studio::clear()
{
    std::vector<Device*>::iterator it;

    InstrumentList list;
    InstrumentList::iterator iit;

    // Append lists
    //
    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        // get sub list
        list = (*it)->getAllInstruments();

        for (iit = list.begin(); iit != list.end(); iit++)
            delete(*iit);

        list.erase(list.begin(), list.end());

        delete *it;
    }

    m_devices.erase(m_devices.begin(), m_devices.end());
}

std::string
Studio::toXmlString()
{
    std::stringstream studio;

    studio << "<studio>" << std::endl << std::endl;

    std::vector<Device*>::iterator it;
    InstrumentList list;

    // Get XML version of devices
    //
    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        // Sends Devices to XML
        //
        studio << (*it)->toXmlString() << std::endl << std::endl;
    }

    studio << "</studio>" << std::endl << std::ends;

    return studio.str();
}

// Run through the Devices checking for MidiDevices and
// returning the first Metronome we come across
//
MidiMetronome*
Studio::getMetronome()
{
    std::vector<Device*>::iterator it;
    MidiDevice *midiDevice;

    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        midiDevice = dynamic_cast<MidiDevice*>(*it);

        if (midiDevice)
        {
            return midiDevice->getMetronome();
        }
    }

    return 0;
}

// Scan all MIDI devices for available channels and map
// them to a current program
Instrument*
Studio::assignMidiProgramToInstrument(MidiByte program, bool percussion)
{
    // Also defined in Midi.h but we don't use that - not here
    // in the clean inner sanctum.
    //
    const MidiByte MIDI_PERCUSSION_CHANNEL = 9;

    MidiDevice *midiDevice;
    std::vector<Device*>::iterator it;
    Rosegarden::InstrumentList::iterator iit;
    Rosegarden::InstrumentList instList;

    // Instruments that we may return
    //
    Rosegarden::Instrument *newInstrument = 0;
    Rosegarden::Instrument *firstInstrument = 0;

    // Pass one - search through all MIDI instruments looking for
    // a match that we can re-use.  i.e. if we have a matching 
    // Program Change then we can use this Instrument.
    //
    //
    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        midiDevice = dynamic_cast<MidiDevice*>(*it);

        if (midiDevice)
        {
            instList = (*it)->getPresentationInstruments();

            for (iit = instList.begin(); iit != instList.end(); iit++)
            {
                if (firstInstrument == 0)
                    firstInstrument = *iit;

                // If we find an Instrument sending the right program already.
                //
                if ((*iit)->sendsProgramChange() &&
                    (*iit)->getProgramChange() == program &&
                    percussion == false)
                {
                    return (*iit);
                }
                else
                {
                    // Ignore the program change and use the percussion
                    // flag.
                    //
                    if ((*iit)->getMidiChannel() == MIDI_PERCUSSION_CHANNEL
                            && percussion)
                    {
                        return (*iit);
                    }

                    // Otherwise store the first Instrument for
                    // possible use later.
                    //
                    if (newInstrument == 0 &&
                        (*iit)->sendsProgramChange() == false &&
                        (*iit)->getMidiChannel() != MIDI_PERCUSSION_CHANNEL)
                        newInstrument = *iit;
                }
            }
        }
    }

    
    // Okay, if we've got this far and we have a newInstrument to use
    // then use it.
    //
    if (newInstrument != 0)
    {
        newInstrument->setSendProgramChange(true);
        newInstrument->setProgramChange(program);
    }
    else // Otherwise we just reuse the first Instrument we found
        newInstrument = firstInstrument;


    return newInstrument;
}

// Just make all of these Instruments available for automatic
// assignment in the assignMidiProgramToInstrument() method
// by invalidating the ProgramChange flag.
//
// This method sounds much more dramatic than it actually is - 
// it could probably do with a rename.
//
//
void
Studio::unassignAllInstruments()
{
    MidiDevice *midiDevice;
    std::vector<Device*>::iterator it;
    Rosegarden::InstrumentList::iterator iit;
    Rosegarden::InstrumentList instList;

    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        midiDevice = dynamic_cast<MidiDevice*>(*it);

        if (midiDevice)
        {
            instList = (*it)->getPresentationInstruments();

            for (iit = instList.begin(); iit != instList.end(); iit++)
            {
                // Only for true MIDI Instruments - not System ones
                //
                if ((*iit)->getID() >= MidiInstrumentBase)
                    (*iit)->setSendProgramChange(false);
            }
        }
    }
}

void
Studio::clearMidiBanksAndPrograms()
{
    MidiDevice *midiDevice;
    std::vector<Device*>::iterator it;

    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        midiDevice = dynamic_cast<MidiDevice*>(*it);

        if (midiDevice)
        {
            midiDevice->clearProgramList();
            midiDevice->clearBankList();
        }
    }
}

Device*
Studio::getDevice(DeviceId id)
{
    std::vector<Device*>::iterator it;

    for (it = m_devices.begin(); it != m_devices.end(); it++)
        if ((*it)->getId() == id)
            return (*it);

    return 0;
}

std::string
Studio::getSegmentName(InstrumentId id)
{
    MidiDevice *midiDevice;
    std::vector<Device*>::iterator it;
    Rosegarden::InstrumentList::iterator iit;
    Rosegarden::InstrumentList instList;

    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        midiDevice = dynamic_cast<MidiDevice*>(*it);

        if (midiDevice)
        {
            instList = (*it)->getAllInstruments();

            for (iit = instList.begin(); iit != instList.end(); iit++)
            {
                if ((*iit)->getID() == id)
                {
                    if ((*iit)->sendsProgramChange())
                    {
                        return midiDevice->
                            getProgramName((*iit)->getMSB(),
                                           (*iit)->getLSB(),
                                           (*iit)->getProgramChange());
                    }
                    else
                    {
                        return (*iit)->getName();
                    }
                }
            }
        }
    }

    return std::string("");
}

}


