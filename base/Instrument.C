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
    m_programChange(0),
    m_msb(0),
    m_lsb(0),
    m_transpose(MidiMidValue),
    m_pan(MidiMidValue),
    m_velocity(100),
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
    m_sendVelocity(false),
    m_mappedId(0)

{
    if (it == Audio)
    {
        // Add a number of plugin place holders (unassigned)
        //
        unsigned int defaultPlugins = 5;
        for (unsigned int i = 0; i < defaultPlugins; i++)
            addPlugin(new AudioPluginInstance(i));
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
    m_programChange(0),
    m_msb(0),
    m_lsb(0),
    m_transpose(MidiMidValue),
    m_pan(MidiMidValue),
    m_velocity(100),
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
    m_sendVelocity(false),
    m_mappedId(0)

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
    }
}

Instrument::Instrument(const Instrument &ins):
    m_id(ins.getId()),
    m_name(ins.getName()),
    m_type(ins.getType()),
    m_channel(ins.getMidiChannel()),
    m_programChange(ins.getProgramChange()),
    m_msb(ins.getMSB()),
    m_lsb(ins.getLSB()),
    m_transpose(ins.getMidiTranspose()),
    m_pan(ins.getPan()),
    m_velocity(ins.getVelocity()),
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
    m_sendVelocity(ins.sendsVelocity()),
    m_mappedId(ins.getMappedId())
{
}


Instrument::~Instrument()
{
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
        !m_sendVelocity)
    {
        instrument << std::ends;
        return instrument.str();
    }
    */

    instrument << "        <instrument id=\"" << m_id;
    instrument << "\" channel=\"" << (int)m_channel;
    instrument << "\" type=\"";

    switch(m_type)
    {
        case Midi:
            instrument << "midi";
            break;

        case Audio:
            instrument << "audio";
            break;

        default:
            instrument << "unknown";
            break;
    }

    instrument << "\">" << std::endl;

    if (m_sendBankSelect)
    {
        instrument << "            <bank msb=\"" << (int)m_msb;
        instrument << "\" lsb=\"" << (int)m_lsb << "\"/>" << std::endl;
    }

    if (m_sendProgramChange)
    {
        instrument << "            <program id=\""
                   << (int)m_programChange << "\"/>"
                   << std::endl;
    }
    
    instrument << "            <pan value=\""
               << (int)m_pan << "\"/>" << std::endl;

    instrument << "            <volume value=\""
               << (int)m_velocity << "\"/>" << std::endl;

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


    PluginInstanceIterator it = m_audioPlugins.begin();
    for (; it != m_audioPlugins.end(); it++)
    {
        instrument << (*it)->toXmlString();
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
    std::string programName;

    MidiByte msb = 0;
    MidiByte lsb = 0;
    //MidiByte program;

    if (m_sendBankSelect)
    {
        msb = m_msb;
        lsb = m_lsb;
    }

    if (m_sendProgramChange == false)
        return std::string("");

    return ((dynamic_cast<MidiDevice*>(m_device))
              ->getProgramName(m_msb, m_lsb, m_programChange));
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

