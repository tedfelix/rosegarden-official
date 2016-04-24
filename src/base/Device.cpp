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

#include "Device.h"
#include "base/Controllable.h"
#include "base/MidiDevice.h"
#include "base/SoftSynthDevice.h"
#include "misc/Debug.h"

namespace Rosegarden
{

const DeviceId Device::NO_DEVICE = 10000;
const DeviceId Device::ALL_DEVICES = 10001;
const DeviceId Device::CONTROL_DEVICE = 10002;

Device::~Device()
{
    SEQUENCER_DEBUG << "~Device";
    InstrumentList::iterator it = m_instruments.begin();
    for (; it != m_instruments.end(); ++it) {
        (*it)->sendWholeDeviceDestroyed();
        delete (*it);
    }
        
}

// Return a Controllable if we are a subtype that also inherits from
// Controllable, otherwise return NULL
Controllable *
Device::getControllable(void)
{
    Controllable *c = dynamic_cast<MidiDevice *>(this);
    if (!c) {
        c = dynamic_cast<SoftSynthDevice *>(this);
    }
    // Even if it's zero, return it now.
    return c;
}

// Base case: Device itself doesn't know AllocateChannels so gives NULL.
// @author Tom Breton (Tehom)
AllocateChannels *
Device::getAllocator(void)
{ return 0; }

}
