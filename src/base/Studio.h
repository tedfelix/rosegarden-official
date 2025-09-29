/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
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

#include <QCoreApplication>

#include <string>
#include <vector>


namespace Rosegarden
{


class Composition;
class RecordIn;
class MidiDevice;
class MidiMetronome;
class Segment;
class Track;
class StudioObserver;


typedef std::vector<Instrument *> InstrumentVector;
typedef std::vector<Buss *> BussVector;
typedef std::vector<RecordIn *> RecordInVector;
typedef std::vector<Device *> DeviceVector;


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

    void addDevice(const std::string &name,
                   DeviceId id,
                   InstrumentId baseInstrumentId,
                   Device::DeviceType type);

    void removeDevice(DeviceId id);

    void resyncDeviceConnections();

    DeviceId getSpareDeviceId(InstrumentId &baseInstrumentId);

    // Return the combined instrument list from all devices
    //
    InstrumentVector getAllInstruments();
    InstrumentVector getPresentationInstruments() const;

    // Return an Instrument
    Instrument* getInstrumentById(InstrumentId id) const;
    Instrument* getInstrumentFromList(int index);

    // Convenience functions
    Instrument *getInstrumentFor(const Segment *) const;
    Instrument *getInstrumentFor(const Track *) const;

    /// Sets the input for an Instrument.
    /**
     * Since the studio needs to have connections that match, this routine
     * simplifies the process by doing all the necessary work.
     *
     * This sets the input in the provided Instrument and updates the Studio
     * connections to match.
     *
     * instrument - The instrument to change.
     * isBuss - Is the new input a buss (submaster) or a record input.
     * newInput - The new buss or record input ID.  E.g. 0 is input 1.
     * newChannel - For record inputs only.  0 for left, 1 for right.  ???
     */
    void setInput(Instrument *instrument, bool isBuss, int newInput, int newChannel) const;
    /// Sets the output for an Instrument.
    /**
     * Since the studio needs to have connections that match, this routine
     * simplifies the process by doing all the necessary work.
     *
     * This sets the output in the provided Instrument and updates the Studio
     * connections to match.
     *
     * instrument - The instrument to change.
     * bussID - The new buss ID.  E.g. 0 is master, 1 is submaster 1...
     */
    void setOutput(Instrument *instrument, int bussID) const;

    // An instrument whose "record in" or submaster is beyond the new
    // record in or submaster count.
    struct InvalidInstrument
    {
        Instrument *instrument{nullptr};
        // Out of range Record In ID or Submaster ID.
        int id{0};
        // For recording, this can either be an input or a submaster.
        bool isInput{false};
    };
    typedef std::vector<InvalidInstrument> InvalidInstrumentVector;

    /// Get a list of the instruments that point to a "record in" beyond newCount.
    InvalidInstrumentVector getRecordInInvalid(int newCount) const;
    /// Sets invalid record ins to "input 1".
    void fixRecordIns(int count);
    /// Get a list of the instruments that point to a submaster beyond newCount.
    InvalidInstrumentVector getSubmasterInvalid(int newCount) const;
    /// Sets invalid submasters to "input 1" for inputs and "master" for outputs..
    void fixSubmasters(int count);

    // Return a Buss
    BussVector getBusses() const;
    Buss *getBussById(BussId id) const;
    void addBuss(Buss *buss);
    //void removeBuss(BussId id);
    void setBussCount(unsigned newBussCount);

    // Return an Instrument or a Buss
    PluginContainer *getContainerById(InstrumentId id);

    RecordInVector getRecordIns() const  { return m_recordIns; }
    RecordIn *getRecordIn(int number) const;
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
    const MidiMetronome* getMetronomeFromDevice(DeviceId id) const;

    // Return the device list
    //
    DeviceVector *getDevices()  { return &m_devices; }
    const DeviceVector *getDevices() const  { return &m_devices; }
    DeviceVector &getDevicesRef()  { return m_devices; }
    const DeviceVector &getDevicesRef() const  { return m_devices; }

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
    DeviceVector::const_iterator begin() const { return m_devices.begin(); }
    DeviceVector::const_iterator end() const { return m_devices.end(); }

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

    Studio(const Studio &) = delete;
    Studio &operator=(const Studio &) = delete;

    DeviceVector        m_devices;
    /// Returns nullptr if there are no MIDI out devices.
    Device *getFirstMIDIOutDevice() const;

    BussVector          m_busses;
    RecordInVector      m_recordIns;

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
