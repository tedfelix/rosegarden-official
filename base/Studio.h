// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
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

// We populate MidiFilter with an OR'd Rosegarden::MappedEvent::MappedEventType.
// Look in sound/MappedEvent.h
//
typedef int MidiFilter;

typedef std::vector<Instrument *> InstrumentList;
typedef std::vector<Device*> DeviceList;
typedef std::vector<Device*>::iterator DeviceListIterator;

class MidiDevice;


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

    // Get a suitable name for a Segment belonging to this instrument.
    // Takes into account ProgramChanges.
    //
    std::string getSegmentName(InstrumentId id);

    // Clear down all the ProgramChange flags in all MIDI Instruments
    //
    void unassignAllInstruments();

    // Clear down all MIDI banks and programs on all MidiDevices
    // prior to reloading.  The Instruments and Devices are generated
    // at the Sequencer - the Banks and Programs are loaded from the
    // RG4 file.
    //
    void clearMidiBanksAndPrograms();

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

    // Useful for combo boxes
    //
    MidiDevice *getMidiDevice(int id);
    Device *getDeviceOfType(int id, Device::DeviceType type);

    // Export as XML string
    //
    virtual std::string toXmlString();

    // Get an audio preview Instrument
    //
    InstrumentId getAudioPreviewInstrument();

    // MIDI filtering into and thru Rosegarden
    //
    void setMIDIThruFilter(MidiFilter filter) { m_midiThruFilter = filter; }
    MidiFilter getMIDIThruFilter() const { return m_midiThruFilter; }

    void setMIDIRecordFilter(MidiFilter filter) { m_midiRecordFilter = filter; }
    MidiFilter getMIDIRecordFilter() const { return m_midiRecordFilter; }

private:

    DeviceList m_devices;

    MidiFilter        m_midiThruFilter;
    MidiFilter        m_midiRecordFilter;

};

}

#endif // _STUDIO_H_
