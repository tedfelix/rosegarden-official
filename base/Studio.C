// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

using std::cerr;
using std::endl;


namespace Rosegarden
{

Studio::Studio():
    m_midiThruFilter(0),
    m_midiRecordFilter(0)
{
}

Studio::~Studio()
{
    DeviceListIterator dIt = m_devices.begin();

    for (; dIt != m_devices.end(); ++dIt)
        delete(*dIt);

    m_devices.clear();

    ControlListIterator it = m_controls.begin();

    for (; it != m_controls.end(); ++it)
        delete (*it);

    m_controls.clear();

}

void
Studio::addDevice(const std::string &name,
                  DeviceId id,
                  Device::DeviceType type)
{
    switch(type)
    {
        case Device::Midi:
            m_devices.push_back(new MidiDevice(id, name, MidiDevice::Play));
            break;

        case Device::Audio:
            m_devices.push_back(new AudioDevice(id, name));
            break;

        default:
            std::cerr << "Studio::addDevice() - unrecognised device"
                      << endl;
            break;
    }
}

void
Studio::addDevice(Device *device)
{
    m_devices.push_back(device);
}

void
Studio::removeDevice(DeviceId id)
{
    DeviceListIterator it;
    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
	if ((*it)->getId() == id) {
	    delete *it;
	    m_devices.erase(it);
	    return;
	}
    }
}

InstrumentList
Studio::getAllInstruments()
{
    InstrumentList list, subList;

    DeviceListIterator it;

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
    MidiDevice *midiDevice;

    // Append lists
    //
    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        midiDevice = dynamic_cast<MidiDevice*>(*it);

	std::cerr << "Studio::getPresentationInstruments: checking out device " << (*it)->getId() << std::endl;
	
        if (midiDevice)
	{
	    // skip read-only devices
	    if (midiDevice->getDirection() == MidiDevice::Record)
		continue;
	}
	
        // get sub list
        subList = (*it)->getPresentationInstruments();

        // concatenate
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
            if ((*iit)->getId() == id)
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
        MidiDevice *midiDevice = dynamic_cast<MidiDevice*>(*it);

        if (midiDevice)
	{
          // skip read-only devices
          if (midiDevice->getDirection() == MidiDevice::Record)
              continue;
        }

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



// Clear down the devices  - the devices will clear down their
// own Instruments.
//
void
Studio::clear()
{
    InstrumentList list;
    std::vector<Device*>::iterator it;

    for (it = m_devices.begin(); it != m_devices.end(); it++)
        delete *it;

    m_devices.erase(m_devices.begin(), m_devices.end());
}

std::string
Studio::toXmlString()
{
    std::stringstream studio;

    studio << "<studio thrufilter=\"" << m_midiThruFilter
           << "\" recordfilter=\"" << m_midiRecordFilter
           << "\">" << endl << endl;

    std::vector<Device*>::iterator it;
    InstrumentList list;

    // Get XML version of devices
    //
    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        // Sends Devices to XML
        //
        studio << (*it)->toXmlString() << endl << endl;
    }

    studio << endl << endl;

    ControlListConstIterator cIt;
    for (cIt = m_controls.begin(); cIt != m_controls.end() ; ++cIt)
        studio << (*cIt)->toXmlString();

    studio << endl << endl;

#if (__GNUC__ < 3)
    studio << "</studio>" << endl << std::ends;
#else
    studio << "</studio>" << endl;
#endif

    return studio.str();
}

// Run through the Devices checking for MidiDevices and
// returning the first Metronome we come across
//
const MidiMetronome*
Studio::getMetronome()
{
    std::vector<Device*>::iterator it;
    MidiDevice *midiDevice;

    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        midiDevice = dynamic_cast<MidiDevice*>(*it);

        if (midiDevice && midiDevice->getMetronome())
        {
            return midiDevice->getMetronome();
        }
    }

    return 0;
}

// Scan all MIDI devices for available channels and map
// them to a current program

Instrument*
Studio::assignMidiProgramToInstrument(MidiByte program,
				      int msb, int lsb,
				      bool percussion)
{
    MidiDevice *midiDevice;
    std::vector<Device*>::iterator it;
    Rosegarden::InstrumentList::iterator iit;
    Rosegarden::InstrumentList instList;

    // Instruments that we may return
    //
    Rosegarden::Instrument *newInstrument = 0;
    Rosegarden::Instrument *firstInstrument = 0;

    bool needBank = (msb >= 0 || lsb >= 0);
    if (needBank) {
	if (msb < 0) msb = 0;
	if (lsb < 0) lsb = 0;
    }

    // Pass one - search through all MIDI instruments looking for
    // a match that we can re-use.  i.e. if we have a matching 
    // Program Change then we can use this Instrument.
    //
    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        midiDevice = dynamic_cast<MidiDevice*>(*it);

        if (midiDevice && midiDevice->getDirection() == MidiDevice::Play)
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
		    (!needBank || ((*iit)->sendsBankSelect() &&
				   (*iit)->getMSB() == msb &&
				   (*iit)->getLSB() == lsb &&
				   (*iit)->isPercussion() == percussion)))
                {
                    return (*iit);
                }
                else
                {
/*!!!
                    // Ignore the program change and use the percussion
                    // flag.
                    //
                    if ((*iit)->getMidiChannel() == MIDI_PERCUSSION_CHANNEL
                            && percussion)
                    {
                        return (*iit);
                    }
*/
                    // Otherwise store the first Instrument for
                    // possible use later.
                    //
                    if (newInstrument == 0 &&
                        (*iit)->sendsProgramChange() == false &&
			(*iit)->sendsBankSelect() == false &&
			(*iit)->isPercussion() == percussion)
                        newInstrument = *iit;
                }
            }
        }
    }

    
    // Okay, if we've got this far and we have a new Instrument to use
    // then use it.
    //
    if (newInstrument != 0)
    {
        newInstrument->setSendProgramChange(true);
        newInstrument->setProgramChange(program);

	if (needBank) {
	    newInstrument->setSendBankSelect(true);
	    newInstrument->setPercussion(percussion);
	    newInstrument->setMSB(msb);
	    newInstrument->setLSB(lsb);
	}
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
    AudioDevice *audioDevice;
    std::vector<Device*>::iterator it;
    Rosegarden::InstrumentList::iterator iit;
    Rosegarden::InstrumentList instList;
    int channel = 0;

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
                if ((*iit)->getId() >= MidiInstrumentBase)
                {
                    (*iit)->setSendBankSelect(false);
                    (*iit)->setSendProgramChange(false);
                    (*iit)->setMidiChannel(channel);
                    channel = ( channel + 1 ) % 16;

                    (*iit)->setSendPan(false);
                    (*iit)->setSendVolume(false);
                    (*iit)->setPan(MidiMidValue);
                    (*iit)->setVolume(100);

                }
            }
        }
        else
        {
            audioDevice = dynamic_cast<AudioDevice*>(*it);

            if (audioDevice)
            {
                instList = (*it)->getPresentationInstruments();

                for (iit = instList.begin(); iit != instList.end(); iit++)
                    (*iit)->emptyPlugins();
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
    cerr << "Studio[" << this << "]::getDevice(" << id << ")... ";

    std::vector<Device*>::iterator it;

    for (it = m_devices.begin(); it != m_devices.end(); it++) {

	if (it != m_devices.begin()) cerr << ", ";

	cerr << (*it)->getId();
        if ((*it)->getId() == id) {
	    cerr << ". Found" << endl;
	    return (*it);
	}
    }

    cerr << ". Not found" << endl;

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
                if ((*iit)->getId() == id)
                {
                    if ((*iit)->sendsProgramChange())
                    {
			return (*iit)->getProgramName();
//!!!                        return midiDevice->
//                            getProgramName((*iit)->getPercussion(),
//					   (*iit)->getMSB(),
//                                           (*iit)->getLSB(),
//                                           (*iit)->getProgramChange());
                    }
                    else
                    {
                        return midiDevice->getName() + " " + (*iit)->getName();
                    }
                }
            }
        }
    }

    return std::string("");
}

InstrumentId
Studio::getAudioPreviewInstrument()
{
    AudioDevice *audioDevice;
    std::vector<Device*>::iterator it;

    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        audioDevice = dynamic_cast<AudioDevice*>(*it);

        // Just the first one will do - we can make this more
        // subtle if we need to later.
        //
        if (audioDevice)
            return audioDevice->getPreviewInstrument();
    }

    // system instrument -  won't accept audio
    return 0;
}

bool
Studio::haveMidiDevices() const
{
    Rosegarden::DeviceListConstIterator it = m_devices.begin();
    for (; it != m_devices.end(); it++)
    {
        if ((*it)->getType() == Device::Midi) return true;
    }
    return false;
}
    


// Copy a studio from another example into this one - regenerating
// copies of everything (deep copying) as we go.
//
void
Studio::copy(const Studio &studio)
{
    clear();

    m_midiThruFilter = studio.getMIDIThruFilter();
    m_midiRecordFilter = studio.getMIDIRecordFilter();


    DeviceListConstIterator it = studio.begin();
    for (; it != studio.end(); it++)
    {
        switch ((*it)->getType())
        {
            case Device::Midi:
                {
                    MidiDevice *dev = dynamic_cast<MidiDevice*>(*it);
                    m_devices.push_back(new MidiDevice(*dev));
                }
                break;
            case Device::Audio:
                {
                    AudioDevice *dev = dynamic_cast<AudioDevice*>(*it);
                    m_devices.push_back(new AudioDevice(*dev));
                }
                break;

            default:
                //do nothing
                break;
        }
    }
}


void
Studio::addControlParameter(ControlParameter *con)
{
    m_controls.push_back(con);
}

void
Studio::addControlParameter(ControlParameter *con, int id)
{
    ControlList controls;

    // if we're out of range just add the control
    if (((unsigned int)id) >= m_controls.size())
    {
        m_controls.push_back(con);
        return;
    }

    // add new controller in at a position
    for (unsigned int i = 0; i < m_controls.size(); ++i)
    {
        if (((unsigned int)id) == i) controls.push_back(con);
        controls.push_back(m_controls[i]);
    }

    // clear and copy back
    m_controls.clear();

    for (unsigned int i = 0; i != controls.size(); ++i)
        m_controls.push_back(controls[i]);
}


bool
Studio::removeControlParameter(int id)
{
    ControlListIterator it = m_controls.begin();
    int i = 0;

    for (; it != m_controls.end(); ++it)
    {
        if (id == i)
        {
            m_controls.erase(it);
            return true;
        }
        i++;
    }

    return false;
}

bool
Studio::modifyControlParameter(ControlParameter *con, int id)
{
    if (id < 0 || ((unsigned int)id) > m_controls.size()) return false;

    delete m_controls[id];
    m_controls[id] = con;

    return true;
}

// Check to see if passed ControlParameter is unique.  Either the
// type must be unique or in the case of Controller::EventType the
// ControllerValue must be unique.
//
// Controllers (Control type)
//
//
bool 
Studio::isUniqueControlParameter(ControlParameter *con) const
{
    ControlListConstIterator it = m_controls.begin();

    for (; it != m_controls.end(); ++it)
    {
        if ((*it)->getType() == con->getType())
        {
            if ((*it)->getType() == Rosegarden::Controller::EventType &&
                (*it)->getControllerValue() != con->getControllerValue())
                continue;

            return false;
        }

    }

    return true;
}

ControlParameter*
Studio::getControlParameter(int id)
{
    if (id >=0  && ((unsigned int)id) < m_controls.size())
        return m_controls[id];

    return 0;
}



}


