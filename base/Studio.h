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

#include <string>

#include "XmlExportable.h"
#include "Instrument.h"
#include "Device.h"

// The Studio is where Midi and Audio devices live.  We can query
// them for a list of Instruments, connect them together or to
// effects units (eventually) and generally do real studio-type
// stuff to them.
//
//


#ifndef _STUDIO_H_
#define _STUDIO_H_

namespace Rosegarden
{

typedef std::vector<Instrument *> InstrumentList;
typedef std::vector<Device*> DeviceList;
typedef std::vector<Device*>::iterator DeviceListIterator;


class Studio : public XmlExportable
{

public:
    Studio();
    ~Studio();

    void addDevice(const std::string &name,
                   DeviceId id,
                   Device::DeviceType type);
    void addDevice(Device *device);

    // Return the combined instrument list from all devices
    // for all and presentation Instrument (Presentation is
    // just a subset of All).
    //
    InstrumentList getAllInstruments();
    InstrumentList getPresentationInstruments();

    // Return an Instrument
    Instrument* getInstrumentById(InstrumentId id);
    Instrument* getInstrumentFromList(int index);

    // A clever method to best guess MIDI file program mappings
    // to available MIDI channels across all MidiDevices.
    //
    // Set the percussion flag if it's a percussion channel (mapped
    // channel) we're after.
    //
    Instrument* assignMidiProgramToInstrument(MidiByte program,
                                              bool percussion);

    // Clear down all the ProgramChange flags in all MIDI Instruments
    //
    void unassignAllInstruments();

    // Clear down
    void clear();

    // Get the first metronome defined in the first device -
    // we only want one Metronome per Studio
    //
    MidiMetronome* getMetronome();

    // Return the device list
    //
    DeviceList* getDevices() { return &m_devices; }

    // Get a device by ID
    //
    Device* getDevice(DeviceId id);

    // Export as XML string
    //
    virtual std::string toXmlString();

private:

    DeviceList m_devices;

};

}

#endif // _STUDIO_H_
