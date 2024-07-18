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

#include "Device.h"

#include "base/Controllable.h"
#include "base/MidiDevice.h"
#include "base/SoftSynthDevice.h"
#include "misc/Debug.h"
#include "document/RosegardenDocument.h"
#include "base/Composition.h"


namespace Rosegarden
{


const DeviceId Device::NO_DEVICE = 10000;
const DeviceId Device::ALL_DEVICES = 10001;
// "external controller" port.
const DeviceId Device::EXTERNAL_CONTROLLER = 10002;


Device::~Device()
{
    //SEQUENCER_DEBUG << "~Device";
    InstrumentList::iterator it = m_instruments.begin();
    // For each Instrument
    for (; it != m_instruments.end(); ++it) {
        (*it)->sendWholeDeviceDestroyed();
        delete (*it);
    }

    if (!m_observers.empty()) {
        RG_WARNING << "dtor: Warning:" << m_observers.size() <<
            "observers still extant";
    }
}

// Return a Controllable if we are a subtype that also inherits from
// Controllable, otherwise return nullptr
const Controllable *
Device::getControllable() const
{
    const Controllable *c = dynamic_cast<const MidiDevice *>(this);
    if (!c) {
        c = dynamic_cast<const SoftSynthDevice *>(this);
    }
    // Even if it's zero, return it now.
    return c;
}

// Base case: Device itself doesn't know AllocateChannels so gives nullptr.
// @author Tom Breton (Tehom)
AllocateChannels *
Device::getAllocator() const
{ return nullptr; }

void
Device::sendChannelSetups() const
{
    // For each Instrument, send channel setup
    for (InstrumentList::const_iterator it = m_instruments.begin();
         it != m_instruments.end();
         ++it) {
        (*it)->sendChannelSetup();
    }
}

InstrumentId
Device::getAvailableInstrument(const Composition *composition) const
{
    InstrumentList instruments = getPresentationInstruments();
    if (instruments.empty())
        return NoInstrument;

    if (!composition)
        composition = &RosegardenDocument::currentDocument->getComposition();

    // Assume not found.
    InstrumentId firstInstrumentID{NoInstrument};

    // For each instrument on the device
    for (const Instrument *instrument : instruments) {
        if (!instrument)
            continue;

        const InstrumentId instrumentID = instrument->getId();

        // If we've not found the first one yet, save it in case we don't
        // find anything available.
        if (firstInstrumentID == NoInstrument)
            firstInstrumentID = instrumentID;

        // If this instrumentID is not in use, return it.
        if (!composition->hasTrack(instrumentID))
            return instrumentID;
    }

    // Return the first instrumentID for this device.
    return firstInstrumentID;
}

void Device::addObserver(DeviceObserver *obs)
{
    //RG_DEBUG << "addObserver" << this << obs;
    m_observers.push_back(obs);
}

void Device::removeObserver(DeviceObserver *obs)
{
    //RG_DEBUG << "removeObserver" << this << obs;
    m_observers.remove(obs);
}

void Device::notifyDeviceModified()
{
    for(ObserverList::iterator i = m_observers.begin();
        i != m_observers.end(); ++i) {
        (*i)->deviceModified(this);
    }

}

}
