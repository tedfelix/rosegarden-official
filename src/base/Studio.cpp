/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[Studio]"

#include <iostream>

#include "base/Studio.h"
#include "MidiDevice.h"
#include "AudioDevice.h"
#include "SoftSynthDevice.h"
#include "Instrument.h"

#include "base/Segment.h"
#include "misc/Strings.h"
#include "Track.h"
#include "Composition.h"
#include "sequencer/RosegardenSequencer.h"

#include <sstream>
#include <string>

#include <QString>


using std::endl;


namespace Rosegarden
{

Studio::Studio() :
    amwShowAudioFaders(true),
    amwShowSynthFaders(true),
    amwShowAudioSubmasters(true),
    amwShowUnassignedFaders(false),
    m_midiThruFilter(0),
    m_midiRecordFilter(0),
    m_metronomeDevice(0)
{
    // We _always_ have a buss with id zero, for the master out
    m_busses.push_back(new Buss(0));

    // And we always create one audio record in
    m_recordIns.push_back(new RecordIn());

    // And we always have one audio and one soft-synth device, whose
    // IDs match the base instrument numbers (for no good reason
    // except easy identifiability)
    addDevice(QObject::tr("Audio").toUtf8().data(),
              AudioInstrumentBase, AudioInstrumentBase,
              Device::Audio);
    addDevice(QObject::tr("Synth plugin").toUtf8().data(),
              SoftSynthInstrumentBase, SoftSynthInstrumentBase,
              Device::SoftSynth);
}

Studio::~Studio()
{
    DeviceListIterator dIt = m_devices.begin();

    for (; dIt != m_devices.end(); ++dIt)
        delete(*dIt);

    m_devices.clear();

    for (size_t i = 0; i < m_busses.size(); ++i) {
        delete m_busses[i];
    }

    for (size_t i = 0; i < m_recordIns.size(); ++i) {
        delete m_recordIns[i];
    }
}

void
Studio::addDevice(const std::string &name,
                  DeviceId id,
                  InstrumentId baseInstrumentId,
                  Device::DeviceType type)
{
    Device *d = nullptr;

    switch (type) {

        case Device::Midi:
            d = new MidiDevice(id, baseInstrumentId, name, MidiDevice::Play);
            break;

        case Device::Audio:
            d = new AudioDevice(id, name);
            break;

        case Device::SoftSynth:
            d = new SoftSynthDevice(id, name);
            break;

        default:
            RG_WARNING << "addDevice(): WARNING: unrecognised device type " << type;
            return;
    }

    m_devices.push_back(d);
}

void
Studio::removeDevice(DeviceId id)
{
    DeviceListIterator it;
    for (it = m_devices.begin(); it != m_devices.end(); it++) {
        if ((*it)->getId() == id) {
            delete *it;
            m_devices.erase(it);
            return;
        }
    }
}

void
Studio::
resyncDeviceConnections()
{
    // Sync all the device connections
    DeviceList *devices = getDevices();
    for (uint i = 0; i < devices->size(); ++i) {
        DeviceId id = (*devices)[i]->getId();
        QString connection = RosegardenSequencer::getInstance()->getConnection(id);
        (*devices)[i]->setConnection(qstrtostr(connection));
    }
}


DeviceId
Studio::getSpareDeviceId(InstrumentId &baseInstrumentId)
{
    InstrumentId highestMidiInstrumentId = MidiInstrumentBase;
    bool foundInstrument = false;

    std::set<DeviceId> ids;
    DeviceListIterator it;
    for (it = m_devices.begin(); it != m_devices.end(); it++) {
        ids.insert((*it)->getId());
        if ((*it)->getType() == Device::Midi) {
            InstrumentList il = (*it)->getAllInstruments();
            for (size_t i = 0; i < il.size(); ++i) {
                if (il[i]->getId() > highestMidiInstrumentId) {
                    highestMidiInstrumentId = il[i]->getId();
                    foundInstrument = true;
                }
            }
        }
    }

    if (!foundInstrument) {
        baseInstrumentId = MidiInstrumentBase;
    } else {
        baseInstrumentId = ((highestMidiInstrumentId / 128) + 1) * 128;
    }

    DeviceId id = 0;
    while (ids.find(id) != ids.end()) ++id;
    return id;
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
Studio::getPresentationInstruments() const
{
    InstrumentList list;

    // For each device...
    for (DeviceList::const_iterator it = m_devices.begin();
         it != m_devices.end();
         ++it) {
        const MidiDevice *midiDevice = dynamic_cast<MidiDevice *>(*it);

        if (midiDevice) {
            // skip read-only devices
            if (midiDevice->getDirection() == MidiDevice::Record)
                continue;
        }

        // get sub list
        InstrumentList subList = (*it)->getPresentationInstruments();

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

    for (it = m_devices.begin(); it != m_devices.end(); ++it)
    {
        list = (*it)->getAllInstruments();

        for (iit = list.begin(); iit != list.end(); ++iit)
            if ((*iit)->getId() == id)
                return (*iit);
    }

    return nullptr;

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

    for (it = m_devices.begin(); it != m_devices.end(); ++it)
    {
        MidiDevice *midiDevice = dynamic_cast<MidiDevice*>(*it);

        if (midiDevice)
        {
          // skip read-only devices
          if (midiDevice->getDirection() == MidiDevice::Record)
              continue;
        }

        list = (*it)->getPresentationInstruments();

        for (iit = list.begin(); iit != list.end(); ++iit)
        {
            if (count == index)
                return (*iit);

            count++;
        }
    }

    return nullptr;

}

Instrument *
Studio::getInstrumentFor(Segment *segment)
{
    if (!segment) return nullptr;
    if (!segment->getComposition()) return nullptr;
    TrackId tid = segment->getTrack();
    Track *track = segment->getComposition()->getTrackById(tid);
    if (!track) return nullptr;
    return getInstrumentFor(track);
}

Instrument *
Studio::getInstrumentFor(Track *track)
{
    if (!track) return nullptr;
    InstrumentId iid = track->getInstrument();
    return getInstrumentById(iid);
}

BussList
Studio::getBusses()
{
    return m_busses;
}

Buss *
Studio::getBussById(BussId id)
{
    for (BussList::iterator i = m_busses.begin(); i != m_busses.end(); ++i) {
        if ((*i)->getId() == id) return *i;
    }
    return nullptr;
}

void
Studio::addBuss(Buss *buss)
{
    if (buss->getId() != m_busses.size()) {
        RG_WARNING << "addBuss() Precondition: Incoming buss has wrong ID.";
    }

    m_busses.push_back(buss);
}

#if 0
void
Studio::removeBuss(BussId id)
{
    for (BussList::iterator i = m_busses.begin(); i != m_busses.end(); ++i) {
        if ((*i)->getId() == id) {
            delete *i;
            m_busses.erase(i);
            return;
        }
    }
}
#endif

void
Studio::setBussCount(unsigned newBussCount)
{
    // We have to have at least one for the master.
    if (newBussCount < 1)
        return;
    // Reasonable limit.  Adjust if needed.
    if (newBussCount > 16)
        return;
    // No change?  Bail.
    if (newBussCount == m_busses.size())
        return;

    // If we need to remove busses
    if (newBussCount < m_busses.size()) {
        int removeCount = m_busses.size() - newBussCount;

        // For each one that needs removing.
        for (int i = 0; i < removeCount; ++i) {
            // Delete the last buss.
            delete m_busses.back();
            // Remove it from the list.
            m_busses.pop_back();
        }
    } else {  // We need to add busses
        int addCount = newBussCount - m_busses.size();

        for (int i = 0; i < addCount; ++i) {
            unsigned bussId = m_busses.size();
            m_busses.push_back(new Buss(bussId));
        }
    }

#if 0
    Q_ASSERT_X(m_busses.size() == newBussCount,
               "Studio::setBussCount()",
               "Postcondition: Buss count is not as expected.");

    for (BussId bussId = 0; bussId < m_busses.size(); ++bussId) {
        Q_ASSERT_X(m_busses[bussId]->getId() == bussId,
                   "Studio::setBussCount()",
                   "Postcondition: Buss has wrong ID.");
    }
#endif
}

PluginContainer *
Studio::getContainerById(InstrumentId id)
{
    PluginContainer *pc = getInstrumentById(id);
    if (pc) return pc;
    else return getBussById(id);
}

RecordIn *
Studio::getRecordIn(int number)
{
    if (number >= 0  &&  number < int(m_recordIns.size()))
        return m_recordIns[number];
    else
        return nullptr;
}

void
Studio::setRecordInCount(unsigned newRecordInCount)
{
    // Can't have zero.
    if (newRecordInCount < 1)
        return;
    if (newRecordInCount > 32)
        return;
    // No change?  Bail.
    if (newRecordInCount == m_recordIns.size())
        return;

    // If we need to add some RecordIns.
    if (newRecordInCount > m_recordIns.size()) {

        unsigned addCount = newRecordInCount - m_recordIns.size();

        for (unsigned i = 0; i < addCount; ++i) {
            m_recordIns.push_back(new RecordIn());
        }

    } else {  // We need to remove some.

        unsigned removeCount = m_recordIns.size() - newRecordInCount;

        // For each one that needs removing.
        for (unsigned i = 0; i < removeCount; ++i) {
            // Delete the last RecordIn.
            delete m_recordIns.back();
            // Remove it from the list.
            m_recordIns.pop_back();
        }
    }

    // The mapped IDs get set by RosegardenDocument::initialiseStudio().
}

// Clear down the devices  - the devices will clear down their
// own Instruments.
//
void
Studio::clear()
{
    InstrumentList list;
    std::vector<Device*>::iterator it;

    for (it = m_devices.begin(); it != m_devices.end(); ++it)
        delete *it;

    m_devices.erase(m_devices.begin(), m_devices.end());
}

std::string
Studio::toXmlString() const
{
    return toXmlString(std::vector<DeviceId>());
}

std::string
Studio::toXmlString(const std::vector<DeviceId> &devices) const
{
    // See RoseXmlHandler for the read side of this.

    std::stringstream studio;

    studio << "<studio thrufilter=\"" << m_midiThruFilter
           << "\" recordfilter=\"" << m_midiRecordFilter
           << "\" audioinputpairs=\"" << m_recordIns.size()
           << "\" metronomedevice=\"" << m_metronomeDevice
           << "\" amwshowaudiofaders=\"" << amwShowAudioFaders
           << "\" amwshowsynthfaders=\"" << amwShowSynthFaders
           << "\" amwshowaudiosubmasters=\"" << amwShowAudioSubmasters
           << "\" amwshowunassignedfaders=\"" << amwShowUnassignedFaders
           << "\">" << endl << endl;

    studio << endl;

    // Get XML version of devices
    //
    if (devices.empty()) { // export all devices and busses

        for (DeviceListConstIterator it = m_devices.begin();
             it != m_devices.end(); it++) {
            studio << (*it)->toXmlString() << endl << endl;
        }

        for (BussList::const_iterator it = m_busses.begin();
             it != m_busses.end(); ++it) {
            studio << (*it)->toXmlString() << endl << endl;
        }

    } else {
        for (std::vector<DeviceId>::const_iterator di(devices.begin());
             di != devices.end(); ++di) {
            Device *d = getDevice(*di);
            if (!d) {
                RG_WARNING << "toXmlString(): WARNING: Unknown device id " << (*di);
            } else {
                studio << d->toXmlString() << endl << endl;
            }
        }
    }

    studio << endl << endl;

    studio << "</studio>" << endl;

    return studio.str();
}

// Run through the Devices checking for MidiDevices and
// returning the first Metronome we come across
//
const MidiMetronome*
Studio::getMetronomeFromDevice(DeviceId id)
{
    std::vector<Device*>::iterator it;

    for (it = m_devices.begin(); it != m_devices.end(); ++it) {

        RG_DEBUG << "getMetronomeFromDevice(): Having a look at device " << (*it)->getId();

        MidiDevice *midiDevice = dynamic_cast<MidiDevice*>(*it);
        if (midiDevice && 
            midiDevice->getId() == id &&
            midiDevice->getMetronome()) {
            RG_DEBUG << "getMetronomeFromDevice(" << id << "): device is a MIDI device";
            return midiDevice->getMetronome();
        }

        SoftSynthDevice *ssDevice = dynamic_cast<SoftSynthDevice *>(*it);
        if (ssDevice && 
            ssDevice->getId() == id &&
            ssDevice->getMetronome()) {
            RG_DEBUG << "getMetronomeFromDevice(" << id << "): device is a soft synth device";
            return ssDevice->getMetronome();
        }
    }

    return nullptr;
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
    Rosegarden::Instrument *newInstrument = nullptr;
    Rosegarden::Instrument *firstInstrument = nullptr;

    bool needBank = (msb >= 0 || lsb >= 0);
    if (needBank) {
        if (msb < 0) msb = 0;
        if (lsb < 0) lsb = 0;
    }

    // Pass one - search through all MIDI instruments looking for
    // a match that we can re-use.  i.e. if we have a matching 
    // Program Change then we can use this Instrument.
    //
    for (it = m_devices.begin(); it != m_devices.end(); ++it)
    {
        midiDevice = dynamic_cast<MidiDevice*>(*it);

        if (midiDevice && midiDevice->getDirection() == MidiDevice::Play)
        {
            instList = (*it)->getPresentationInstruments();

            for (iit = instList.begin(); iit != instList.end(); ++iit)
            {
                if (firstInstrument == nullptr)
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
                    // Ignore the program change and use the percussion
                    // flag.
                    //
                    if ((*iit)->isPercussion() && percussion)
                    {
                        return (*iit);
                    }

                    // Otherwise store the first Instrument for
                    // possible use later.
                    //
                    if (newInstrument == nullptr &&
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
    if (newInstrument != nullptr)
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

    for (it = m_devices.begin(); it != m_devices.end(); ++it)
    {
        midiDevice = dynamic_cast<MidiDevice*>(*it);

        if (midiDevice)
        {
            instList = (*it)->getPresentationInstruments();

            for (iit = instList.begin(); iit != instList.end(); ++iit)
            {
                // Only for true MIDI Instruments - not System ones
                //
                if ((*iit)->getId() >= MidiInstrumentBase)
                {
                    (*iit)->setSendBankSelect(false);
                    (*iit)->setSendProgramChange(false);
                    (*iit)->setNaturalChannel(channel);
                    channel = ( channel + 1 ) % 16;
                    (*iit)->setFixedChannel();
                    // ??? This is a "reset" of the instrument.  It doesn't
                    //     seem to make sense that we should send out the
                    //     default values.
                    //(*iit)->sendChannelSetup();

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

                for (iit = instList.begin(); iit != instList.end(); ++iit)
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

    for (it = m_devices.begin(); it != m_devices.end(); ++it)
    {
        midiDevice = dynamic_cast<MidiDevice*>(*it);

        if (midiDevice)
        {
            midiDevice->clearProgramList();
            midiDevice->clearBankList();
        }
    }
}

void
Studio::clearBusses()
{
    for (size_t i = 0; i < m_busses.size(); ++i) {
        delete m_busses[i];
    }
    m_busses.clear();
    m_busses.push_back(new Buss(0));
}

void
Studio::clearRecordIns()
{
    for (size_t i = 0; i < m_recordIns.size(); ++i) {
        delete m_recordIns[i];
    }
    m_recordIns.clear();
    m_recordIns.push_back(new RecordIn());
}

Device *
Studio::getDevice(DeviceId id) const
{
    //RG_DEBUG << "Studio[" << this << "]::getDevice(" << id << ")... ";
    
    std::vector<Device*>::const_iterator it;
    
    for (it = m_devices.begin(); it != m_devices.end(); ++it) {
        
        // possibly fix a following seg.fault :
        if( ! (*it) ){ 
            RG_WARNING << "getDevice(): WARNING: (*it) is nullptr";
            continue;
        }
        
        //RG_DEBUG << (*it)->getId();

        if ((*it)->getId() == id) {
            //RG_DEBUG << "Found";
            return (*it);
        }
    }
    
    //RG_DEBUG << "NOT found";
    
    return nullptr;
}

Device *
Studio::getAudioDevice()
{
    std::vector<Device*>::iterator it;

    for (it = m_devices.begin(); it != m_devices.end(); ++it) {
        if ((*it)->getType() == Device::Audio) return *it;
    }

    return nullptr;
}

Device *
Studio::getSoftSynthDevice()
{
    std::vector<Device*>::iterator it;

    for (it = m_devices.begin(); it != m_devices.end(); ++it) {
        if ((*it)->getType() == Device::SoftSynth) return *it;
    }

    return nullptr;
}

std::string
Studio::getSegmentName(InstrumentId id)
{
    MidiDevice *midiDevice;
    std::vector<Device*>::iterator it;
    Rosegarden::InstrumentList::iterator iit;
    Rosegarden::InstrumentList instList;

    for (it = m_devices.begin(); it != m_devices.end(); ++it)
    {
        midiDevice = dynamic_cast<MidiDevice*>(*it);

        if (midiDevice)
        {
            instList = (*it)->getAllInstruments();

            for (iit = instList.begin(); iit != instList.end(); ++iit)
            {
                if ((*iit)->getId() == id)
                {
                    if ((*iit)->sendsProgramChange())
                    {
                        return (*iit)->getProgramName();
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

    for (it = m_devices.begin(); it != m_devices.end(); ++it)
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
    

}

