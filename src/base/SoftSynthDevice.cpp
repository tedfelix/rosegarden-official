/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "SoftSynthDevice.h"
#include "Instrument.h"
#include "base/MidiTypes.h"

#include <cstdio>
#include <cstdlib>

#include <sstream>
#include <QString>


namespace Rosegarden
{

ControlList
SoftSynthDevice::m_controlList;

SoftSynthDevice::SoftSynthDevice() :
    Device(0, "Default Soft Synth Device", Device::SoftSynth),
    m_metronome(0)
{
    createInstruments();
    checkControlList();
}

SoftSynthDevice::SoftSynthDevice(DeviceId id, const std::string &name) :
    Device(id, name, Device::SoftSynth),
    m_metronome(0)
{
    createInstruments();
    checkControlList();
}


SoftSynthDevice::SoftSynthDevice(const SoftSynthDevice &dev) :
    Device(dev.getId(), dev.getName(), dev.getType()),
    Controllable(),
    m_metronome(0)
{
    // Copy the instruments
    //
    InstrumentList insList = dev.getAllInstruments();
    InstrumentList::iterator iIt = insList.begin();
    for (; iIt != insList.end(); ++iIt)
        m_instruments.push_back(new Instrument(**iIt));
    if (dev.m_metronome) m_metronome = new MidiMetronome(*dev.m_metronome);
}

SoftSynthDevice::~SoftSynthDevice()
{
    delete m_metronome;
}

void
SoftSynthDevice::createInstruments()
{
    for (uint i = 0; i < SoftSynthInstrumentCount; ++i) {
	Instrument *instrument = new Instrument
	    (SoftSynthInstrumentBase + i, Instrument::SoftSynth, "", i, this);
	addInstrument(instrument);
    }
    renameInstruments();
}

void
SoftSynthDevice::renameInstruments()
{
    for (uint i = 0; i < SoftSynthInstrumentCount; ++i) {
        m_instruments[i]->setName
            (QString("%1 #%2").arg(getName().c_str()).arg(i+1).toUtf8().data());
    }
}
    

void
SoftSynthDevice::checkControlList()
{
    // Much as MidiDevice::generateDefaultControllers

    static std::string controls[][9] = {
        { "Pan", Rosegarden::Controller::EventType, "<none>", "0", "127", "64", "10", "2", "0" },
        { "Chorus", Rosegarden::Controller::EventType, "<none>", "0", "127", "0", "93", "3", "1" },
        { "Volume", Rosegarden::Controller::EventType, "<none>", "0", "127", "100", "7", "1", "2" },
        { "Reverb", Rosegarden::Controller::EventType, "<none>", "0", "127", "0", "91", "3", "3" },
        { "Sustain", Rosegarden::Controller::EventType, "<none>", "0", "127", "0", "64", "4", "-1" },
        { "Expression", Rosegarden::Controller::EventType, "<none>", "0", "127", "100", "11", "2", "-1" },
        { "Modulation", Rosegarden::Controller::EventType, "<none>", "0", "127", "0", "1", "4", "-1" },
        { "PitchBend", Rosegarden::PitchBend::EventType, "<none>", "0", "16383", "8192", "1", "4", "-1" }
    };

    if (m_controlList.empty()) {
	
	for (size_t i = 0; i < sizeof(controls) / sizeof(controls[0]); ++i) {

	    Rosegarden::ControlParameter con(controls[i][0],
					     controls[i][1],
					     controls[i][2],
					     atoi(controls[i][3].c_str()),
					     atoi(controls[i][4].c_str()),
					     atoi(controls[i][5].c_str()),
					     Rosegarden::MidiByte(atoi(controls[i][6].c_str())),
					     atoi(controls[i][7].c_str()),
					     atoi(controls[i][8].c_str()));
	    m_controlList.push_back(con);
	}
    }
}

const ControlParameter *
SoftSynthDevice::getControlParameter(int index) const
{
    if (index >= 0 && ((unsigned int)index) < ((unsigned int)m_controlList.size()))
        return &m_controlList[index];
    return 0;
}

const ControlParameter *
SoftSynthDevice::getControlParameter(const std::string &type,
				     Rosegarden::MidiByte controllerValue) const
{
    ControlList::iterator it = m_controlList.begin();

    for (; it != m_controlList.end(); ++it)
    {
        if (it->getType() == type)
        {
            // Return matched on type for most events
            //
            if (type != Rosegarden::Controller::EventType) 
                return &*it;
            
            // Also match controller value for Controller events
            //
            if (it->getControllerValue() == controllerValue)
                return  &*it;
        }
    }

    return 0;
}

void
SoftSynthDevice::setMetronome(const MidiMetronome &metronome)
{
    delete m_metronome;
    m_metronome = new MidiMetronome(metronome);
}

std::string
SoftSynthDevice::toXmlString() const
{
    std::stringstream ssiDevice;
    InstrumentList::const_iterator iit;

    ssiDevice << "    <device id=\""  << m_id
                << "\" name=\""         << m_name
                << "\" type=\"softsynth\">" << std::endl;

    for (iit = m_instruments.begin(); iit != m_instruments.end(); ++iit)
        ssiDevice << (*iit)->toXmlString();

    if (m_metronome) {

        ssiDevice << "        <metronome "
                   << "instrument=\"" << m_metronome->getInstrument() << "\" "
                   << "barpitch=\"" << (int)m_metronome->getBarPitch() << "\" "
                   << "beatpitch=\"" << (int)m_metronome->getBeatPitch() << "\" "
                   << "subbeatpitch=\"" << (int)m_metronome->getSubBeatPitch() << "\" "
                   << "depth=\"" << (int)m_metronome->getDepth() << "\" "
                   << "barvelocity=\"" << (int)m_metronome->getBarVelocity() << "\" "
                   << "beatvelocity=\"" << (int)m_metronome->getBeatVelocity() << "\" "
                   << "subbeatvelocity=\"" << (int)m_metronome->getSubBeatVelocity() 
                   << "\"/>"
                   << std::endl << std::endl;
    }

    ssiDevice << "    </device>"
                << std::endl;

    return ssiDevice.str();
}


// Add to instrument list
//
void
SoftSynthDevice::addInstrument(Instrument *instrument)
{
    m_instruments.push_back(instrument);
}


}
