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
    m_programList(new ProgramList()),
    m_bankList(new BankList()),
    m_metronome(new MidiMetronome())
{
    createInstruments();
}

MidiDevice::MidiDevice(const std::string &name):
    Device(name, Device::Midi),
    m_programList(new ProgramList()),
    m_bankList(new BankList()),
    m_metronome(new MidiMetronome())
{
    createInstruments();
}

MidiDevice::~MidiDevice()
{
    delete m_programList;
    delete m_bankList;
    delete m_metronome;
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
            new Instrument(i + MidiInstrumentBase,        // id
                           Instrument::Midi,              // type
                           name,                          // name
                           (MidiByte)i,                   // channel
                           dynamic_cast<Device*>(this))); // parent device 
    }

    m_instruments.push_back(
        new Instrument(SystemInstrumentBase + 1,      // Metronome ID
                       Instrument::Midi,              // type
                       string("Metronome"),           // name
                       (MidiByte)9,                   // channel
                       dynamic_cast<Device*>(this))); // parent device 

}

void
MidiDevice::clearProgramList()
{
    std::vector<MidiProgram*>::iterator it;

    for (it = m_programList->begin(); it != m_programList->end(); it++)
        delete (*it);

    m_programList->erase(m_programList->begin(), m_programList->end());


}

void
MidiDevice::addProgram(MidiProgram *prog)
{
    m_programList->push_back(prog);
}

void 
MidiDevice::addBank(MidiBank *bank)
{
    m_bankList->push_back(bank);
}

void
MidiDevice::setMetronome(MidiByte msb, MidiByte lsb, MidiByte program,
                         MidiByte pitch, MidiByte channel,
                         const std::string &name)
{

    m_metronome->pitch = pitch;
    m_metronome->program = program;
    m_metronome->msb = msb;
    m_metronome->lsb = lsb;
    m_metronome->name = name;
    m_metronome->instrument = SystemInstrumentBase + 1; // hardcode for the mo
}


// Create a Program list
//
StringList
MidiDevice::getProgramList(MidiByte msb, MidiByte lsb)
{
    StringList list;
    ProgramList::iterator it;

    for (it = m_programList->begin(); it != m_programList->end(); it++)
    {
        // If the bank matches
        if (msb == (*it)->msb && lsb == (*it)->lsb)
            list.push_back((*it)->name);
    }

    return list;
}

// We handle banks by name, not by number
//
StringList
MidiDevice::getBankList()
{
    StringList list;
    std::vector<MidiBank*>::iterator it;

    for (it = m_bankList->begin(); it != m_bankList->end(); it++)
        list.push_back((*it)->name);

    return list;
}

MidiBank*
MidiDevice::getBankByIndex(int index)
{
    return (*m_bankList)[index];
}


MidiProgram*
MidiDevice::getProgramByIndex(int index)
{
    return (*m_programList)[index];
}


std::string
MidiDevice::toXmlString()
{
    std::string xml;

    return xml;
}

}


