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

#ifndef _INSTRUMENT_H_
#define _INSTRUMENT_H_

#include <string>
#include <vector>

#include "XmlExportable.h"

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

typedef unsigned int InstrumentId;
typedef unsigned char MidiByte;

// plugins
class AudioPluginInstance;
typedef std::vector<AudioPluginInstance*>::iterator PluginInstanceIterator;

struct MidiBank
{
    MidiByte msb;
    MidiByte lsb;
    std::string name;
};

struct MidiProgram : public MidiBank
{
    MidiByte program;
};

// A mapped MIDI instrument - a drum track click for example
//
struct MidiMetronome : public MidiProgram
{
    MidiByte pitch;
    InstrumentId instrument;
};


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

    void setId(int id) { m_id = id; }
    InstrumentId getId() const { return m_id; }

    void setName(const std::string &name) { m_name = name; }
    InstrumentType getType() const { return m_type; }

    void setType(InstrumentType type) { m_type = type; }
    InstrumentType getInstrumentType() { return m_type; }

    void setMidiChannel(MidiByte mC) { m_channel = mC; }
    MidiByte getMidiChannel() const { return m_channel; }

    void setAudioChannels(unsigned int ch) { m_channel = MidiByte(ch); }
    unsigned int getAudioChannels() const { return (unsigned int)(m_channel); }

    void setMidiTranspose(MidiByte mT) { m_transpose = mT; }
    MidiByte getMidiTranspose() const { return m_transpose; }

    void setPan(MidiByte pan) { m_pan = pan; }
    MidiByte getPan() const { return m_pan; }

    void setVelocity(MidiByte velocity) { m_velocity = velocity; }
    MidiByte getVelocity() const { return m_velocity; }

    void setProgramChange(MidiByte program) { m_programChange = program; }
    MidiByte getProgramChange() const { return m_programChange; }

    void setSendBankSelect(bool value) { m_sendBankSelect = value; }
    bool sendsBankSelect() const { return m_sendBankSelect; }

    void setSendProgramChange(bool value) { m_sendProgramChange = value; }
    bool sendsProgramChange() const { return m_sendProgramChange; }

    void setSendPan(bool value) { m_sendPan = value; }
    bool sendsPan() const { return m_sendPan; }

    void setSendVelocity(bool value) { m_sendVelocity = value; }
    bool sendsVelocity() const { return m_sendVelocity; } 

    void setMSB(MidiByte msb) { m_msb = msb; }
    MidiByte getMSB() const { return m_msb; }

    void setLSB(MidiByte lsb) { m_lsb = lsb; }
    MidiByte getLSB() const { return m_lsb; }

    void setAttack(MidiByte attack) { m_attack = attack; }
    MidiByte getAttack() const { return m_attack; }
    
    void setRelease(MidiByte release) { m_release = release; }
    MidiByte getRelease() const { return m_release; }

    void setFilter(MidiByte filter) { m_filter = filter; }
    MidiByte getFilter() const { return m_filter; }

    void setResonance(MidiByte res) { m_resonance = res; }
    MidiByte getResonance() const { return m_resonance; }

    void setChorus(MidiByte chorus) { m_chorus = chorus; }
    MidiByte getChorus() const { return m_chorus; }

    void setReverb(MidiByte reverb) { m_reverb = reverb; }
    MidiByte getReverb() const { return m_reverb; }

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

    int getPort() const { return m_port; }
    void setPort(int port) { m_port = port; }

private:
    InstrumentId    m_id;
    std::string     m_name;
    InstrumentType  m_type;
    
    // Standard MIDI controllers and parameters
    //
    MidiByte        m_channel;
    MidiByte        m_programChange;
    MidiByte        m_msb;
    MidiByte        m_lsb;
    MidiByte        m_transpose;
    MidiByte        m_pan;
    MidiByte        m_velocity;

    // More advanced controllers
    //
    MidiByte        m_attack;
    MidiByte        m_release;
    MidiByte        m_filter;
    MidiByte        m_resonance;
    MidiByte        m_chorus;
    MidiByte        m_reverb;

    Device         *m_device;

    // Do we send at this intrument or do we leave these
    // things up to the parent device and God?  These are
    // directly relatable to GUI elements
    // 
    bool             m_sendBankSelect;
    bool             m_sendProgramChange;
    bool             m_sendPan;
    bool             m_sendVelocity;

    // Where we hold the audio plugins for this instrument
    std::vector<AudioPluginInstance*>     m_audioPlugins;

    // Instruments are directly related to faders for volume
    // control.  Here we can store the remote fader id.
    //
    int              m_mappedId;

    // If a MIDI Instrument then we might have a port number.
    // The Device to which this Instrument belongs will give
    // us a client ID on the sequencer side in the ALSA case.
    //
    int              m_port;

};

}

#endif // _INSTRUMENT_H_
