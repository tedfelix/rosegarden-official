// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
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
#include "AudioLevel.h"

#if (__GNUC__ < 3)
#include <strstream>
#define stringstream strstream
#else
#include <sstream>
#endif


namespace Rosegarden
{

const unsigned int PluginContainer::PLUGIN_COUNT = 5;

PluginContainer::PluginContainer(bool havePlugins)
{
    if (havePlugins) {
        // Add a number of plugin place holders (unassigned)
        for (unsigned int i = 0; i < PLUGIN_COUNT; i++)
            addPlugin(new AudioPluginInstance(i));
    }
}

PluginContainer::~PluginContainer()
{
    clearPlugins();
}

void
PluginContainer::addPlugin(AudioPluginInstance *instance)
{
    m_audioPlugins.push_back(instance);
}

bool
PluginContainer::removePlugin(unsigned int position)
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
PluginContainer::clearPlugins()
{
    PluginInstanceIterator it = m_audioPlugins.begin();
    for (; it != m_audioPlugins.end(); it++)
        delete (*it);

    m_audioPlugins.erase(m_audioPlugins.begin(), m_audioPlugins.end());
}

void 
PluginContainer::emptyPlugins()
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
PluginContainer::getPlugin(unsigned int position)
{
    PluginInstanceIterator it = m_audioPlugins.begin();
    for (; it != m_audioPlugins.end(); it++)
    {
        if ((*it)->getPosition() == position)
            return *it;
    }

    return 0;
}


const unsigned int Instrument::SYNTH_PLUGIN_POSITION = 999;


Instrument::Instrument(InstrumentId id,
		       InstrumentType it,
                       const std::string &name,
                       Device *device):
    PluginContainer(it == Audio || it == SoftSynth),
    m_id(id),
    m_name(name),
    m_type(it),
    m_channel(0),
    m_transpose(MidiMidValue),
    m_pan(MidiMidValue),
    m_volume(100),
    m_level(0.0),
    m_recordLevel(0.0),
    m_device(device),
    m_sendBankSelect(false),
    m_sendProgramChange(false),
    m_sendPan(false),
    m_sendVolume(false),
    m_mappedId(0),
    m_audioInput(1000),
    m_audioInputChannel(0),
    m_audioOutput(0)
{
    if (it == Audio || it == SoftSynth)
    {
        // In an audio instrument we use the m_channel attribute to
        // hold the number of audio channels this Instrument uses -
        // not the MIDI channel number.  Default is 2 (stereo).
        //
        m_channel = 2;

        m_pan = 100; // audio pan ranges from -100 to 100 but
                     // we store within an unsigned char as 
                     // 0 to 200. 
    }

    if (it == SoftSynth) {
	addPlugin(new AudioPluginInstance(SYNTH_PLUGIN_POSITION));
    }
}

Instrument::Instrument(InstrumentId id,
                       InstrumentType it,
                       const std::string &name,
                       MidiByte channel,
                       Device *device):
    PluginContainer(it == Audio || it == SoftSynth),
    m_id(id),
    m_name(name),
    m_type(it),
    m_channel(channel),
    m_transpose(MidiMidValue),
    m_pan(MidiMidValue),
    m_volume(100),
    m_level(0.0),
    m_recordLevel(0.0),
    m_device(device),
    m_sendBankSelect(false),
    m_sendProgramChange(false),
    m_sendPan(false),
    m_sendVolume(false),
    m_mappedId(0),
    m_audioInput(1000),
    m_audioInputChannel(0),
    m_audioOutput(0)
{
    // Add a number of plugin place holders (unassigned)
    //
    if (it == Audio || it == SoftSynth)
    {
        // In an audio instrument we use the m_channel attribute to
        // hold the number of audio channels this Instrument uses -
        // not the MIDI channel number.  Default is 2 (stereo).
        //
        m_channel = 2;

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

    if (it == SoftSynth) {
	addPlugin(new AudioPluginInstance(SYNTH_PLUGIN_POSITION));
    }
}

Instrument::Instrument(const Instrument &ins):
    XmlExportable(),
    PluginContainer(ins.getType() == Audio || ins.getType() == SoftSynth),
    m_id(ins.getId()),
    m_name(ins.getName()),
    m_type(ins.getType()),
    m_channel(ins.getMidiChannel()),
    m_program(ins.getProgram()),
    m_transpose(ins.getMidiTranspose()),
    m_pan(ins.getPan()),
    m_volume(ins.getVolume()),
    m_level(ins.getLevel()),
    m_recordLevel(ins.getRecordLevel()),
    m_device(ins.getDevice()),
    m_sendBankSelect(ins.sendsBankSelect()),
    m_sendProgramChange(ins.sendsProgramChange()),
    m_sendPan(ins.sendsPan()),
    m_sendVolume(ins.sendsVolume()),
    m_mappedId(ins.getMappedId()),
    m_audioInput(ins.m_audioInput),
    m_audioInputChannel(ins.m_audioInputChannel),
    m_audioOutput(ins.m_audioOutput)
{
    if (ins.getType() == Audio || ins.getType() == SoftSynth)
    {
        // In an audio instrument we use the m_channel attribute to
        // hold the number of audio channels this Instrument uses -
        // not the MIDI channel number.  Default is 2 (stereo).
        //
        m_channel = 2;
    }

    if (ins.getType() == SoftSynth) {
	addPlugin(new AudioPluginInstance(SYNTH_PLUGIN_POSITION));
    }
}

Instrument &
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
    m_level = ins.getLevel();
    m_recordLevel = ins.getRecordLevel();
    m_device = ins.getDevice();
    m_sendBankSelect = ins.sendsBankSelect();
    m_sendProgramChange = ins.sendsProgramChange();
    m_sendPan = ins.sendsPan();
    m_sendVolume = ins.sendsVolume();
    m_mappedId = ins.getMappedId();
    m_audioInput = ins.m_audioInput;
    m_audioInputChannel = ins.m_audioInputChannel;
    m_audioOutput = ins.m_audioOutput;

    return *this;
}


Instrument::~Instrument()
{
}

std::string
Instrument::getPresentationName() const
{
    if (m_type == Audio || m_type == SoftSynth || !m_device) {
	return m_name;
    } else {
	return m_device->getName() + " " + m_name;
    }
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

void
Instrument::setAudioInputToBuss(BussId buss, int channel)
{
    m_audioInput = buss;
    m_audioInputChannel = channel;
}

void
Instrument::setAudioInputToRecord(int recordIn, int channel)
{
    m_audioInput = recordIn + 1000;
    m_audioInputChannel = channel;
}

int
Instrument::getAudioInput(bool &isBuss, int &channel) const
{
    channel = m_audioInputChannel;

    if (m_audioInput >= 1000) {
	isBuss = false;
	return m_audioInput - 1000;
    } else {
	isBuss = true;
	return m_audioInput;
    }
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

        for (StaticControllerConstIterator it = m_staticControllers.begin();
             it != m_staticControllers.end(); ++it)
        {
            instrument << "            <controlchange type=\"" << int(it->first)
                       << "\" value=\"" << int(it->second) << "\"/>" << std::endl;
        }

    }
    else // Audio or SoftSynth
    {

	if (m_type == Audio) {
	    instrument << "audio\">" << std::endl;
	} else {
	    instrument << "softsynth\">" << std::endl;
	}

        instrument << "            <pan value=\""
                   << (int)m_pan << "\"/>" << std::endl;

        instrument << "            <level value=\""
                   << m_level << "\"/>" << std::endl;

        instrument << "            <recordLevel value=\""
                   << m_recordLevel << "\"/>" << std::endl;

	bool aibuss;
	int channel;
	int ai = getAudioInput(aibuss, channel);

        instrument << "            <audioInput value=\""
                   << ai << "\" type=\""
		   << (aibuss ? "buss" : "record")
		   << "\" channel=\"" << channel
		   << "\"/>" << std::endl;

        instrument << "            <audioOutput value=\""
                   << m_audioOutput << "\"/>" << std::endl;

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
Instrument::setControllerValue(MidiByte controller, MidiByte value)
{
    for (StaticControllerIterator it = m_staticControllers.begin();
         it != m_staticControllers.end(); ++it)
    {
        if (it->first == controller)
        {
            it->second = value;
            return;
        }
    }

    m_staticControllers.push_back(std::pair<MidiByte, MidiByte>(controller, value));

}

MidiByte
Instrument::getControllerValue(MidiByte controller) const
{
    for (StaticControllerConstIterator it = m_staticControllers.begin();
         it != m_staticControllers.end(); ++it)
    {

        if (it->first  == controller)
            return it->second;
    }

    throw std::string("<no controller of that value>");
}


Buss::Buss(BussId id) :
    PluginContainer(true),
    m_id(id),
    m_level(0.0),
    m_pan(100),
    m_mappedId(0)
{
}

Buss::~Buss()
{
}

std::string
Buss::toXmlString()
{
    std::stringstream buss;

    buss << "    <buss id=\"" << m_id << "\">" << std::endl;
    buss << "       <pan value=\"" << (int)m_pan << "\"/>" << std::endl;
    buss << "       <level value=\"" << m_level << "\"/>" << std::endl;

    PluginInstanceIterator it = m_audioPlugins.begin();
    for (; it != m_audioPlugins.end(); it++) {
	buss << (*it)->toXmlString();
    }

    buss << "    </buss>" << std::endl;

#if (__GNUC__ < 3)
    buss << std::ends;
#endif

    return buss.str();
}

std::string
Buss::getName() const
{
    char buffer[20];
    sprintf(buffer, "Submaster %d", m_id);
    return buffer;
}

RecordIn::RecordIn() :
    m_mappedId(0)
{
}

RecordIn::~RecordIn()
{
}

std::string
RecordIn::toXmlString()
{
    // We don't actually save these, as they have nothing persistent
    // in them.  The studio just remembers how many there should be.
    return "";
}


}

