/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[Studio]"
#define RG_NO_DEBUG_PRINT

#include <iostream>

#include "base/Studio.h"
#include "MidiDevice.h"
#include "AudioDevice.h"
#include "SoftSynthDevice.h"
#include "Instrument.h"

#include "base/RecordIn.h"
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
    m_audioInputs(0),
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
    addDevice(QCoreApplication::translate("INSTRUMENT",
                                          "Audio").toUtf8().data(),
              AudioInstrumentBase, AudioInstrumentBase,
              Device::Audio);
    addDevice(QCoreApplication::translate("INSTRUMENT",
                                          "Synth plugin").toUtf8().data(),
              SoftSynthInstrumentBase, SoftSynthInstrumentBase,
              Device::SoftSynth);
}

Studio::~Studio()
{
    RG_DEBUG << "dtor";
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

    if (!m_observers.empty()) {
        RG_WARNING << "dtor: Warning:" << m_observers.size() <<
            "observers still extant";
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
    // inform the observers
    for(ObserverList::const_iterator i = m_observers.begin();
        i != m_observers.end(); ++i) {
        (*i)->deviceAdded(d);
    }

}

void
Studio::removeDevice(DeviceId id)
{
    DeviceListIterator it;
    for (it = m_devices.begin(); it != m_devices.end(); it++) {
        if ((*it)->getId() == id) {
            Device* d = *it;
            m_devices.erase(it);
            // inform the observers
            for(ObserverList::const_iterator i = m_observers.begin();
                i != m_observers.end(); ++i) {
                (*i)->deviceRemoved(d);
            }
            delete(d);
            return;
        }
    }
}

void
Studio::resyncDeviceConnections()
{
    // Sync all the MidiDevice connections to the current connections
    // according to RosegardenSequencer.

    DeviceList *devices = getDevices();

    // For each Device
    for (unsigned i = 0; i < devices->size(); ++i) {
        // Only MidiDevice's have connections.
        MidiDevice *midiDevice = dynamic_cast<MidiDevice *>((*devices)[i]);
        if (!midiDevice)
            continue;

        DeviceId deviceId = midiDevice->getId();
        QString connection =
                RosegardenSequencer::getInstance()->getConnection(deviceId);

        // If we are connected to something, but the user didn't ask for
        // anything, we must have been connected up by
        // AlsaDriver::connectSomething().  In that case, we'd better store
        // the connection as the user selection in case the user actually
        // likes it.
        if (connection != ""  &&  midiDevice->getUserConnection() == "")
            midiDevice->setUserConnection(qstrtostr(connection));
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

Instrument *
Studio::getInstrumentById(InstrumentId id) const
{
    // For each Device
    for (std::vector<Device *>::const_iterator deviceIter = m_devices.begin();
         deviceIter != m_devices.end();
         ++deviceIter)
    {
        InstrumentList list = (*deviceIter)->getAllInstruments();

        for (InstrumentList::const_iterator instrumentIter = list.begin();
             instrumentIter != list.end();
             ++instrumentIter) {
            if ((*instrumentIter)->getId() == id)
                return (*instrumentIter);
        }
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
Studio::getInstrumentFor(const Segment *segment) const
{
    if (!segment) return nullptr;
    if (!segment->getComposition()) return nullptr;
    TrackId tid = segment->getTrack();
    Track *track = segment->getComposition()->getTrackById(tid);
    if (!track) return nullptr;
    return getInstrumentFor(track);
}

Instrument *
Studio::getInstrumentFor(const Track *track) const
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

const MidiMetronome *
Studio::getMetronomeFromDevice(DeviceId id)
{
    // For each Device
    for (std::vector<Device *>::const_iterator deviceIter = m_devices.begin();
         deviceIter != m_devices.end();
         ++deviceIter) {

        //RG_DEBUG << "getMetronomeFromDevice(): Having a look at device " << (*it)->getId();

        // No ID match?  Try the next.
        if ((*deviceIter)->getId() != id)
            continue;

        MidiDevice *midiDevice = dynamic_cast<MidiDevice *>(*deviceIter);

        // If it's a MidiDevice and it has a metronome, return it.
        if (midiDevice  &&
            midiDevice->getMetronome()) {
            //RG_DEBUG << "getMetronomeFromDevice(" << id << "): device is a MIDI device";
            return midiDevice->getMetronome();
        }

        SoftSynthDevice *ssDevice = dynamic_cast<SoftSynthDevice *>(*deviceIter);

        // If it's a SoftSynthDevice and it has a metronome, return it.
        if (ssDevice  &&
            ssDevice->getMetronome()) {
            //RG_DEBUG << "getMetronomeFromDevice(" << id << "): device is a soft synth device";
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
        MidiDevice* midiDevice = dynamic_cast<MidiDevice*>(*it);

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
    AudioDevice *audioDevice;
    std::vector<Device*>::iterator it;
    Rosegarden::InstrumentList::iterator iit;
    Rosegarden::InstrumentList instList;
    int channel = 0;

    for (it = m_devices.begin(); it != m_devices.end(); ++it)
    {
        MidiDevice* midiDevice = dynamic_cast<MidiDevice*>(*it);

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
                    (*iit)->setNaturalMidiChannel(channel);
                    channel = ( channel + 1 ) % 16;
                    (*iit)->setFixedChannel();
                    // ??? This is a "reset" of the instrument.  It doesn't
                    //     seem to make sense that we should send out the
                    //     default values.
                    //(*iit)->sendChannelSetup();

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
    std::vector<Device*>::iterator it;

    for (it = m_devices.begin(); it != m_devices.end(); ++it)
    {
        MidiDevice* midiDevice = dynamic_cast<MidiDevice*>(*it);

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
Studio::getAudioDevice() const
{
    // For each Device
    for (Device *device : m_devices) {
        // Audio?  Return it.
        if (device->getType() == Device::Audio)
            return device;
    }

    return nullptr;
}

Device *
Studio::getSoftSynthDevice() const
{
    // For each Device
    for (Device *device : m_devices) {
        // SoftSynth?  Return it.
        if (device->getType() == Device::SoftSynth)
            return device;
    }

    return nullptr;
}

std::string
Studio::getSegmentName(InstrumentId id)
{
    std::vector<Device*>::iterator it;
    Rosegarden::InstrumentList::iterator iit;
    Rosegarden::InstrumentList instList;

    for (it = m_devices.begin(); it != m_devices.end(); ++it)
    {
        MidiDevice* midiDevice = dynamic_cast<MidiDevice*>(*it);

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
    std::vector<Device*>::iterator it;

    for (it = m_devices.begin(); it != m_devices.end(); ++it)
    {
        AudioDevice* audioDevice = dynamic_cast<AudioDevice*>(*it);

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

Device *
Studio::getFirstMIDIOutDevice() const
{
    // For each Device...
    for (Device *device : m_devices) {
        if (!device)
            continue;

        // MIDI Devices only.
        if (device->getType() != Device::Midi)
            continue;
        // Output only.
        if (!device->isOutput())
            continue;

        return device;
    }

    // Not found.
    return nullptr;
}

InstrumentId
Studio::getFirstMIDIInstrument() const
{
    const Device *device = getFirstMIDIOutDevice();
    if (!device)
        return SoftSynthInstrumentBase;

    InstrumentList instruments = device->getPresentationInstruments();

    if (!instruments.empty()) {
        Instrument *instrument = instruments[0];
        if (instrument)
            return instrument->getId();
    }

    return SoftSynthInstrumentBase;
}

InstrumentId
Studio::getAvailableMIDIInstrument(const Composition *composition) const
{
    const Device *device = getFirstMIDIOutDevice();
    if (!device)
        return SoftSynthInstrumentBase;

    // Find an Instrument we can use.
    const InstrumentId foundInstrumentID =
            device->getAvailableInstrument(composition);

    if (foundInstrumentID != NoInstrument)
        return foundInstrumentID;

    return SoftSynthInstrumentBase;
}

void Studio::addObserver(StudioObserver *obs)
{
    RG_DEBUG << "addObserver" << this << obs;
    m_observers.push_back(obs);
}

void Studio::removeObserver(StudioObserver *obs)
{
    RG_DEBUG << "removeObserver" << this << obs;
    m_observers.remove(obs);
}


}
