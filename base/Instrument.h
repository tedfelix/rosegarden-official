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

#ifndef _INSTRUMENT_H_
#define _INSTRUMENT_H_

#include <string>
#include <vector>

#include "XmlExportable.h"
#include "MidiProgram.h"

// An Instrument connects a Track (which itself contains
// a list of Segments) to a device that can play that
// Track.  
//
// An Instrument is either MIDI or Audio (or whatever else
// we decide to implement).
//
//

namespace Rosegarden
{

// plugins
class AudioPluginInstance;
typedef std::vector<AudioPluginInstance*>::iterator PluginInstanceIterator;

typedef std::vector<std::pair<MidiByte, MidiByte> > StaticControllers;
typedef std::vector<std::pair<MidiByte, MidiByte> >::iterator StaticControllerIterator;
typedef std::vector<std::pair<MidiByte, MidiByte> >::const_iterator StaticControllerConstIterator;


// Instrument number groups
//
const InstrumentId SystemInstrumentBase = 0;
const InstrumentId AudioInstrumentBase  = 1000;
const InstrumentId MidiInstrumentBase   = 2000;

const MidiByte MidiMaxValue = 127;
const MidiByte MidiMidValue = 64;
const MidiByte MidiMinValue = 0;

// Predeclare Device
//
class Device;

class Instrument : public XmlExportable
{
public:
    enum InstrumentType { Midi, Audio };

    Instrument(InstrumentId id,
               InstrumentType it,
               const std::string &name,
               Device *device);

    Instrument(InstrumentId id,
               InstrumentType it,
               const std::string &name,
               MidiByte channel,
               Device *device);


    // Copy constructor and assignment
    //
    Instrument(const Instrument &);
    Instrument operator=(const Instrument &);

    ~Instrument();

    std::string getName() const { return m_name; }
    std::string getPresentationName() const;

    void setId(int id) { m_id = id; }
    InstrumentId getId() const { return m_id; }

    void setName(const std::string &name) { m_name = name; }
    InstrumentType getType() const { return m_type; }

    void setType(InstrumentType type) { m_type = type; }
    InstrumentType getInstrumentType() { return m_type; }


    // ---------------- MIDI Controllers -----------------
    //
    void setMidiChannel(MidiByte mC) { m_channel = mC; }
    MidiByte getMidiChannel() const { return m_channel; }

    void setMidiTranspose(MidiByte mT) { m_transpose = mT; }
    MidiByte getMidiTranspose() const { return m_transpose; }

    void setPan(MidiByte pan) { m_pan = pan; }
    MidiByte getPan() const { return m_pan; }

    void setVolume(MidiByte volume) { m_volume = volume; }
    MidiByte getVolume() const { return m_volume; }

    //!!! hmm, not sure about this -- previously we stored lsb, msb,
    //program literally as ints (or MidiBytes) and that may have been
    //preferable, as we generally need to refer to them individually
    //(for example, we use the program number but not the lsb/msb if
    //the sendBankSelect switch is off).

    void setProgram(const MidiProgram &program) { m_program = program; }
    const MidiProgram &getProgram() const { return m_program; }

    void setSendBankSelect(bool value) { m_sendBankSelect = value; }
    bool sendsBankSelect() const { return m_sendBankSelect; }

    void setSendProgramChange(bool value) { m_sendProgramChange = value; }
    bool sendsProgramChange() const { return m_sendProgramChange; }

    void setSendPan(bool value) { m_sendPan = value; }
    bool sendsPan() const { return m_sendPan; }

    void setSendVolume(bool value) { m_sendVolume = value; }
    bool sendsVolume() const { return m_sendVolume; } 

    void setControllerValue(MidiByte controller, MidiByte value);
    MidiByte getControllerValue(MidiByte controller) const;

    // Convenience functions (strictly redundant with get/setProgram):
    // 
    void setProgramChange(MidiByte program);
    MidiByte getProgramChange() const;

    void setMSB(MidiByte msb);
    MidiByte getMSB() const;

    void setLSB(MidiByte msb);
    MidiByte getLSB() const;

    void setPercussion(bool percussion);
    bool isPercussion() const;

    // --------------- Audio Controllers -----------------
    //
    void setRecordLevel(MidiByte level) { m_recordLevel = level; }
    MidiByte getRecordLevel() const { return m_recordLevel; }

    void setAudioChannels(unsigned int ch) { m_channel = MidiByte(ch); }
    unsigned int getAudioChannels() const { return (unsigned int)(m_channel); }

    void setMappedAudioInput(int input) { m_mappedAudioInput = input; }
    int getMappedAudioInput() const { return m_mappedAudioInput; }

    void setMappedAudioOutput(std::pair<int, int> pair) 
        { m_mappedAudioOutput = pair; }
    std::pair<int, int> getMappedAudioOutput() const 
        { return m_mappedAudioOutput; }

    // Implementation of virtual function
    //
    virtual std::string toXmlString();

    // Get and set the parent device
    //
    Device* getDevice() const { return m_device; }
    void setDevice(Device* dev) { m_device = dev; }

    // Return a string describing the current program for
    // this Instrument
    //
    std::string getProgramName() const;

    PluginInstanceIterator beginPlugins() { return m_audioPlugins.begin(); }
    PluginInstanceIterator endPlugins() { return m_audioPlugins.end(); }

    // Plugin management
    //
    void addPlugin(AudioPluginInstance *instance);
    bool removePlugin(unsigned int position);
    void clearPlugins();
    void emptyPlugins(); // empty the plugins but don't clear them down

    // Get a plugin for this instrument
    //
    AudioPluginInstance* getPlugin(int index);

    // MappedId management - should typedef this type once
    // we have the energy to shake this all out.
    //
    int getMappedId() const { return m_mappedId; }
    void setMappedId(int id) { m_mappedId = id; }

    StaticControllers& getStaticControllers() { return m_staticControllers; }

private:
    InstrumentId    m_id;
    std::string     m_name;
    InstrumentType  m_type;
    
    // Standard MIDI controllers and parameters
    //
    MidiByte        m_channel;
    MidiProgram     m_program;
    MidiByte        m_transpose;
    MidiByte        m_pan;  // required by audio

    // Used for Audio volume (0dB == 100)
    //
    MidiByte        m_volume;

    // Record level for Audio recording (0dB == 100)
    //
    MidiByte        m_recordLevel;

    Device         *m_device;

    // Do we send at this intrument or do we leave these
    // things up to the parent device and God?  These are
    // directly relatable to GUI elements
    // 
    bool             m_sendBankSelect;
    bool             m_sendProgramChange;
    bool             m_sendPan;
    bool             m_sendVolume;

    // Where we hold the audio plugins for this instrument
    std::vector<AudioPluginInstance*>     m_audioPlugins;

    // Instruments are directly related to faders for volume
    // control.  Here we can store the remote fader id.
    //
    int              m_mappedId;

    // Which input terminal we're connected to - simple integer based
    // on the number of channels this audio Instrument supports.
    //
    int              m_mappedAudioInput;

    // MappedObjectId/port pair for output connection
    //
    std::pair<int, int>  m_mappedAudioOutput;

    // A static controller map that can be saved/loaded and queried along with this instrument.
    // These values are modified from the IPB - if they appear on the IPB then they are sent
    // at playback start time to the sequencer.
    //
    //
    StaticControllers    m_staticControllers;
};

}

#endif // _INSTRUMENT_H_
