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

#include "MidiDevice.h"
#include "Instrument.h"

#include <cstdio>

namespace Rosegarden
{

MidiDevice::MidiDevice():
    Device("Default Midi Device", Device::Midi),
    m_bankMSB(0), m_bankLSB(0), m_bankSelect(false)
{
    createInstruments();
}

MidiDevice::MidiDevice(const std::string &name):
    Device(name, Device::Midi),
    m_bankMSB(0), m_bankLSB(0), m_bankSelect(false)
{
    createInstruments();
}

MidiDevice::~MidiDevice()
{
}

void
MidiDevice::createInstruments()
{
    char instNum[100];

    for (InstrumentId i = 0; i < 16; i++)
    {
        sprintf(instNum, "%d", i);
        std::string name = m_name.c_str()+
                           std::string(" #") +std::string(instNum);

        m_instruments.push_back(
            new Instrument(i + MidiInstrumentBase,    // id
                           Instrument::Midi,          // type
                           name,                      // name
                           (MidiByte)i));             // channel
    }

}

void
MidiDevice::setBankSelect(bool value)
{
    m_bankSelect = value;
    populateProgramList();
}


void
MidiDevice::setBankSelectMSB(MidiByte msb)
{
    m_bankMSB = msb;
    populateProgramList();
}

void
MidiDevice::setBankSelectLSB(MidiByte lsb)
{
    m_bankLSB = lsb;
    populateProgramList();
}


void
MidiDevice::clearProgramList()
{
    std::vector<MidiProgram*>::iterator it;

    for (it = m_programList.begin(); it != m_programList.end(); it++)
        delete (*it);

    m_programList.erase(m_programList.begin(), m_programList.end());


}


// Set labels according to bank select messages
//
void
MidiDevice::populateProgramList()
{
    if(m_bankSelect)
    {
        // Eventually calculate according to MSB and LSB -
        // for the moment populate with 128 default values

        clearProgramList();

        std::vector<MidiProgram*>::iterator it;
        int i = 0;

        for (it = m_programList.begin(); it != m_programList.end(); it++)
        {
            m_programList.push_back(new MidiProgram((MidiByte)i++, ""));
        }
        
    }
    else // no bank select - populate with General MIDI
    {
    }

}



}


