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

#include "Instrument.h"
#include "MidiDevice.h"
#include "AudioPluginInstance.h"

#if (__GNUC__ < 3)
#include <strstream>
#define stringstream strstream
#else
#include <sstream>
#endif


namespace Rosegarden
{

Instrument::Instrument(InstrumentId id, InstrumentType it,
                       const std::string &name,
                       Device *device):
    m_id(id),
    m_name(name),
    m_type(it),
    m_channel(0),
    m_transpose(MidiMidValue),
    m_pan(MidiMidValue),
    m_volume(100),
    m_recordLevel(100),
    m_attack(MidiMinValue),
    m_release(MidiMinValue),
    m_filter(MidiMaxValue),
    m_resonance(MidiMinValue),
    m_chorus(MidiMinValue),
    m_reverb(MidiMinValue),
    m_device(device),
    m_sendBankSelect(false),
    m_sendProgramChange(false),
    m_sendPan(false),
    m_sendVolume(false),
    m_mappedId(0),
    m_mappedAudioInput(0),
    m_mappedAudioOutput(std::pair<int, int>(0, 0))
{
    if (it == Audio)
    {
        // Add a number of plugin place holders (unassigned)
        //
        unsigned int defaultPlugins = 5;
        for (unsigned int i = 0; i < defaultPlugins; i++)
            addPlugin(new AudioPluginInstance(i));

        // In an audio instrument we use the m_channel attribute to
        // hold the number of audio channels this Instrument uses -
        // not the MIDI channel number.  Default is 1 (mono).
        //
        m_channel = 1;

        m_pan = 100; // audio pan ranges from -100 to 100 but
                     // we store within an unsigned char as 
                     // 0 to 200. 
    }

}

Instrument::Instrument(InstrumentId id,
                       InstrumentType it,
                       const std::string &name,
                       MidiByte channel,
                       Device *device):
    m_id(id),
    m_name(name),
    m_type(it),
    m_channel(channel),
    m_transpose(MidiMidValue),
    m_pan(MidiMidValue),
    m_volume(100),
    m_recordLevel(100),
    m_attack(MidiMinValue),
    m_release(MidiMinValue),
    m_filter(MidiMaxValue),
    m_resonance(MidiMinValue),
    m_chorus(MidiMinValue),
    m_reverb(MidiMinValue),
    m_device(device),
    m_sendBankSelect(false),
    m_sendProgramChange(false),
    m_sendPan(false),
    m_sendVolume(false),
    m_mappedId(0),
    m_mappedAudioInput(0),
    m_mappedAudioOutput(std::pair<int, int>(0, 0))
{
    // Add a number of plugin place holders (unassigned)
    //
    if (it == Audio)
    {
        // Add a number of plugin place holders (unassigned)
        //
        unsigned int defaultPlugins = 5;
        for (unsigned int i = 0; i < defaultPlugins; i++)
            addPlugin(new AudioPluginInstance(i));

        // In an audio instrument we use the m_channel attribute to
        // hold the number of audio channels this Instrument uses -
        // not the MIDI channel number.  Default is 1 (mono).
        //
        m_channel = 1;

        m_pan = 100; // audio pan ranges from -100 to 100 but
                     // we store within an unsigned char as 

    } else {

	// Also defined in Midi.h but we don't use that - not here
	// in the clean inner sanctum.
	//
	const MidiByte MIDI_PERCUSSION_CHANNEL = 9;
	const MidiByte MIDI_EXTENDED_PERCUSSION_CHANNEL = 10;

	if (m_channel == MIDI_PERCUSSION_CHANNEL ||
	    m_channel == MIDI_EXTENDED_PERCUSSION_CHANNEL) {
	    setPercussion(true);
	}
    }
}

Instrument::Instrument(const Instrument &ins):
    XmlExportable(),
    m_id(ins.getId()),
    m_name(ins.getName()),
    m_type(ins.getType()),
    m_channel(ins.getMidiChannel()),
    m_program(ins.getProgram()),
    m_transpose(ins.getMidiTranspose()),
    m_pan(ins.getPan()),
    m_volume(ins.getVolume()),
    m_recordLevel(ins.getRecordLevel()),
    m_attack(ins.getAttack()),
    m_release(ins.getRelease()),
    m_filter(ins.getFilter()),
    m_resonance(ins.getResonance()),
    m_chorus(ins.getChorus()),
    m_reverb(ins.getReverb()),
    m_device(ins.getDevice()),
    m_sendBankSelect(ins.sendsBankSelect()),
    m_sendProgramChange(ins.sendsProgramChange()),
    m_sendPan(ins.sendsPan()),
    m_sendVolume(ins.sendsVolume()),
    m_mappedId(ins.getMappedId()),
    m_mappedAudioInput(ins.getMappedAudioInput()),
    m_mappedAudioOutput(ins.getMappedAudioOutput())
{
    // Add a number of plugin place holders (unassigned)
    //
    if (ins.getType() == Audio)
    {
        // Add a number of plugin place holders (unassigned)
        //
        unsigned int defaultPlugins = 5;
        for (unsigned int i = 0; i < defaultPlugins; i++)
            addPlugin(new AudioPluginInstance(i));

        // In an audio instrument we use the m_channel attribute to
        // hold the number of audio channels this Instrument uses -
        // not the MIDI channel number.  Default is 1 (mono).
        //
        m_channel = 1;
    }
}

Instrument
Instrument::operator=(const Instrument &ins)
{
    if (&ins == this) return *this;

    m_id = ins.getId();
    m_name = ins.getName();
    m_type = ins.getType();
    m_channel = ins.getMidiChannel();
    m_program = ins.getProgram();
    m_transpose = ins.getMidiTranspose();
    m_pan = ins.getPan();
    m_volume = ins.getVolume();
    m_recordLevel = ins.getRecordLevel();
    m_attack = ins.getAttack();
    m_release = ins.getRelease();
    m_filter = ins.getFilter();
    m_resonance = ins.getResonance();
    m_chorus = ins.getChorus();
    m_reverb = ins.getReverb();
    m_device = ins.getDevice();
    m_sendBankSelect = ins.sendsBankSelect();
    m_sendProgramChange = ins.sendsProgramChange();
    m_sendPan = ins.sendsPan();
    m_sendVolume = ins.sendsVolume();
    m_mappedId = ins.getMappedId();
    m_mappedAudioInput = ins.getMappedAudioInput();
    m_mappedAudioOutput = ins.getMappedAudioOutput();

    return *this;
}


Instrument::~Instrument()
{
}

void
Instrument::setProgramChange(MidiByte program)
{
    m_program = MidiProgram(m_program.getBank(), program);
}

MidiByte
Instrument::getProgramChange() const
{
    return m_program.getProgram();
}

void
Instrument::setMSB(MidiByte msb)
{
    m_program = MidiProgram(MidiBank(m_program.getBank().isPercussion(),
				     msb,
				     m_program.getBank().getLSB()),
			    m_program.getProgram());
}

MidiByte
Instrument::getMSB() const
{
    return m_program.getBank().getMSB();
}

void
Instrument::setLSB(MidiByte lsb)
{
    m_program = MidiProgram(MidiBank(m_program.getBank().isPercussion(),
				     m_program.getBank().getMSB(),
				     lsb),
			    m_program.getProgram());
}

MidiByte
Instrument::getLSB() const
{
    return m_program.getBank().getLSB();
}

void
Instrument::setPercussion(bool percussion)
{
    m_program = MidiProgram(MidiBank(percussion,
				     m_program.getBank().getMSB(),
				     m_program.getBank().getLSB()),
			    m_program.getProgram());
}

bool
Instrument::isPercussion() const
{
    return m_program.getBank().isPercussion();
}


// Implementation of the virtual method to output this class
// as XML.  We don't send out the name as it's redundant in
// the file - that is driven from the sequencer.
//
//
std::string
Instrument::toXmlString()
{

    std::stringstream instrument;

    // We don't send system Instruments out this way -
    // only user Instruments.
    //
    if (m_id < AudioInstrumentBase)
    {
#if (__GNUC__ < 3)
        instrument << std::ends;
#endif
        return instrument.str();
    } 

    // only export if there's anything worth sending
    //
    //
    /*
    if (!m_sendBankSelect &&
        !m_sendProgramChange &&
        !m_sendPan &&
        !m_sendVolume)
    {
        instrument << std::ends;
        return instrument.str();
    }
    */

    instrument << "        <instrument id=\"" << m_id;
    instrument << "\" channel=\"" << (int)m_channel;
    instrument << "\" type=\"";

    if (m_type == Midi)
    {
        instrument << "midi\">" << std::endl;

        if (m_sendBankSelect)
        {
            instrument << "            <bank percussion=\""
		       << (isPercussion() ? "true" : "false") << "\" msb=\""
		       << (int)getMSB();
            instrument << "\" lsb=\"" << (int)getLSB() << "\"/>" << std::endl;
        }

        if (m_sendProgramChange)
        {
            instrument << "            <program id=\""
                       << (int)getProgramChange() << "\"/>"
                       << std::endl;
        }
    
        instrument << "            <pan value=\""
                   << (int)m_pan << "\"/>" << std::endl;

        instrument << "            <volume value=\""
                   << (int)m_volume << "\"/>" << std::endl;

        // Advanced MIDI controls
        //
        instrument << "            <reverb value=\""
                       << (int)m_reverb << "\"/>" << std::endl;

        instrument << "            <chorus value=\""
                       << (int)m_chorus << "\"/>" << std::endl;

        instrument << "            <filter value=\""
                       << (int)m_filter << "\"/>" << std::endl;

        instrument << "            <resonance value=\""
                       << (int)m_resonance << "\"/>" << std::endl;

        instrument << "            <attack value=\""
                       << (int)m_attack << "\"/>" << std::endl;

        instrument << "            <release value=\""
                       << (int)m_release << "\"/>" << std::endl;

    }
    else // Audio
    {
        instrument << "audio\">" << std::endl;

        instrument << "            <pan value=\""
                   << (int)m_pan << "\"/>" << std::endl;

        instrument << "            <volume value=\""
                   << (int)m_volume << "\"/>" << std::endl;

        instrument << "            <recordLevel value=\""
                   << (int)m_recordLevel << "\"/>" << std::endl;

        instrument << "            <audioInput value=\""
                   << m_mappedAudioInput << "\"/>" << std::endl;

        PluginInstanceIterator it = m_audioPlugins.begin();
        for (; it != m_audioPlugins.end(); it++)
        {
            instrument << (*it)->toXmlString();
        }
    }
        
    instrument << "        </instrument>" << std::endl
#if (__GNUC__ < 3)
               << std::endl << std::ends;
#else
               << std::endl;
#endif

    return instrument.str();

}


// Return a program name given a bank select (and whether
// we send it or not)
//
std::string
Instrument::getProgramName() const
{
    if (m_sendProgramChange == false)
        return std::string("");

    MidiProgram program(m_program);

    if (!m_sendBankSelect)
	program = MidiProgram(MidiBank(isPercussion(), 0, 0), program.getProgram());

    return ((dynamic_cast<MidiDevice*>(m_device))->getProgramName(program));
}

void
Instrument::addPlugin(AudioPluginInstance *instance)
{
    m_audioPlugins.push_back(instance);
}

bool
Instrument::removePlugin(unsigned int position)
{
    PluginInstanceIterator it = m_audioPlugins.begin();

    for (; it != m_audioPlugins.end(); it++)
    {
        if ((*it)->getPosition() == position)
        {
            delete (*it);
            m_audioPlugins.erase(it);
            return true;
        }

    }

    return false;
}

void
Instrument::clearPlugins()
{
    PluginInstanceIterator it = m_audioPlugins.begin();
    for (; it != m_audioPlugins.end(); it++)
        delete (*it);

    m_audioPlugins.erase(m_audioPlugins.begin(), m_audioPlugins.end());
}

void 
Instrument::emptyPlugins()
{
    PluginInstanceIterator it = m_audioPlugins.begin();
    for (; it != m_audioPlugins.end(); it++)
    {
        (*it)->setAssigned(false);
        (*it)->setBypass(false);
        (*it)->clearPorts();
    }
}


// Get an instance for an index
//
AudioPluginInstance*
Instrument::getPlugin(int index)
{
    PluginInstanceIterator it = m_audioPlugins.begin();
    for (; it != m_audioPlugins.end(); it++)
    {
        if ((*it)->getPosition() == ((unsigned int)index))
            return *it;
    }

    return 0;
}


}

