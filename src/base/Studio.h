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

#ifndef RG_STUDIO_H
#define RG_STUDIO_H

#include "XmlExportable.h"
#include "Instrument.h"
#include "Device.h"
#include "MidiProgram.h"
#include "MidiMetronome.h"
#include "ControlParameter.h"

#include <QCoreApplication>

#include <string>
#include <vector>

namespace Rosegarden
{


class Composition;
class RecordIn;
class MidiDevice;
class Segment;
class Track;
class StudioObserver;

typedef std::vector<Instrument *> InstrumentList;
typedef std::vector<Device*> DeviceList;
typedef std::vector<Buss *> BussList;
typedef std::vector<RecordIn *> RecordInList;
typedef std::vector<Device*>::iterator DeviceListIterator;
typedef std::vector<Device*>::const_iterator DeviceListConstIterator;


/// Holds Device objects.
/**
 * The Studio is where Midi and Audio devices live.  We can query
 * them for a list of Instruments, connect them together or to
 * effects units (eventually) and generally do real studio-type
 * stuff to them.
 *
 * RosegardenDocument has an instance of Studio.  A reference can be obtained
 * using RosegardenDocument::getStudio().
 */
class Studio : public XmlExportable
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::Studio)

public:
    Studio();
    ~Studio() override;

private:
    Studio(const Studio &);
    Studio& operator=(const Studio &);

public:
    void addDevice(const std::string &name,
                   DeviceId id,
                   InstrumentId baseInstrumentId,
                   Device::DeviceType type);

    void removeDevice(DeviceId id);

    void resyncDeviceConnections();

    DeviceId getSpareDeviceId(InstrumentId &baseInstrumentId);

    // Return the combined instrument list from all devices
    //
    InstrumentList getAllInstruments();
    InstrumentList getPresentationInstruments() const;

    // Return an Instrument
    Instrument* getInstrumentById(InstrumentId id) const;
    Instrument* getInstrumentFromList(int index);

    // Convenience functions
    Instrument *getInstrumentFor(const Segment *) const;
    Instrument *getInstrumentFor(const Track *) const;

    // Return a Buss
    BussList getBusses();
    Buss *getBussById(BussId id);
    void addBuss(Buss *buss);
    //void removeBuss(BussId id);
    void setBussCount(unsigned newBussCount);

    // Return an Instrument or a Buss
    PluginContainer *getContainerById(InstrumentId id);

    RecordInList getRecordIns() { return m_recordIns; }
    RecordIn *getRecordIn(int number);
    void addRecordIn(RecordIn *ri) { m_recordIns.push_back(ri); }
    void setRecordInCount(unsigned newRecordInCount);

    // A clever method to best guess MIDI file program mappings
    // to available MIDI channels across all MidiDevices.
    //
    // Set the percussion flag if it's a percussion channel (mapped
    // channel) we're after.
    //
    Instrument* assignMidiProgramToInstrument(MidiByte program,
                                              bool percussion) {
        return assignMidiProgramToInstrument(program, -1, -1, percussion);
    }

    // Same again, but with bank select
    //
    Instrument* assignMidiProgramToInstrument(MidiByte program,
                                              int msb, int lsb,
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

    void clearBusses();
    void clearRecordIns();

    // Get a MIDI metronome from a given device
    //
    const MidiMetronome* getMetronomeFromDevice(DeviceId id);

    // Return the device list
    //
    DeviceList *getDevices()  { return &m_devices; }
    const DeviceList *getDevices() const  { return &m_devices; }

    /// Get an available Instrument on the first MIDI Device.
    /**
     * If none are available, go with the first MIDI Instrument on the first
     * Device.  If there are no MIDI Devices, go with the first SoftSynth
     * Instrument.
     *
     * composition can be specified when working with a new Composition
     * that isn't "current" yet (e.g. during import).  Specify nullptr to
     * use the current Composition.
     */
    InstrumentId getAvailableMIDIInstrument(
            const Composition *composition = nullptr) const;
    InstrumentId getFirstMIDIInstrument() const;


    // Const iterators
    //
    DeviceListConstIterator begin() const { return m_devices.begin(); }
    DeviceListConstIterator end() const { return m_devices.end(); }

    // Get a device by ID
    //
    Device *getDevice(DeviceId id) const;

    // Get device of audio type (there is only one)
    //
    Device *getAudioDevice() const;

    // Get device of soft synth type (there is only one)
    //
    Device *getSoftSynthDevice() const;

    bool haveMidiDevices() const;

    // Export as XML string
    //
    std::string toXmlString() const override;

    // Export a subset of devices as XML string.  If devices is empty,
    // exports all devices just as the above method does.
    //
    virtual std::string toXmlString(const std::vector<DeviceId> &devices) const;

    // Get an audio preview Instrument
    //
    InstrumentId getAudioPreviewInstrument();

    // MIDI filtering into and thru Rosegarden
    //
    void setMIDIThruFilter(MidiFilter filter) { m_midiThruFilter = filter; }
    MidiFilter getMIDIThruFilter() const { return m_midiThruFilter; }

    void setMIDIRecordFilter(MidiFilter filter) { m_midiRecordFilter = filter; }
    MidiFilter getMIDIRecordFilter() const { return m_midiRecordFilter; }

    /// For the AudioMixerWindow2.
    bool amwShowAudioFaders;
    bool amwShowSynthFaders;
    bool amwShowAudioSubmasters;
    bool amwShowUnassignedFaders;

    DeviceId getMetronomeDevice() const { return m_metronomeDevice; }
    void setMetronomeDevice(DeviceId device) { m_metronomeDevice = device; }

    // observer management
    void addObserver(StudioObserver *obs);
    void removeObserver(StudioObserver *obs);

private:

    DeviceList        m_devices;
    /// Returns nullptr if there are no MIDI out devices.
    Device *getFirstMIDIOutDevice() const;

    BussList          m_busses;
    RecordInList      m_recordIns;

    int               m_audioInputs; // stereo pairs

    MidiFilter        m_midiThruFilter;
    MidiFilter        m_midiRecordFilter;

    DeviceId          m_metronomeDevice;
    typedef std::list<StudioObserver *> ObserverList;
    ObserverList m_observers;

};

class StudioObserver
{
 public:
    virtual ~StudioObserver() {}

    // called after device has been created
    virtual void deviceAdded(Device*) {}
    // called after device has been removed but before it is deleted
    virtual void deviceRemoved(Device*) {}
};

}

#endif // RG_STUDIO_H
