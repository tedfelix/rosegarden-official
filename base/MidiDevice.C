// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.2
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

#if (__GNUC__ < 3)
#include <strstream>
#define stringstream strstream
#else
#include <sstream>
#endif


namespace Rosegarden
{

MidiDevice::MidiDevice():
    Device(0, "Default Midi Device", Device::Midi),
    m_programList(new ProgramList()),
    m_bankList(new BankList()),
    m_metronome(new MidiMetronome())
{
    createInstruments();
}

MidiDevice::MidiDevice(DeviceId id, const std::string &name):
    Device(id, name, Device::Midi),
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
    // initialise metronome
    m_metronome->instrument = 0;
    m_metronome->msb = 0;
    m_metronome->lsb = 0;
    m_metronome->program = 0;
    m_metronome->pitch = 0;

    // Create metronome instrument
    //
    m_instruments.push_back(
        new Instrument(SystemInstrumentBase + 1,      // Metronome ID
                       Instrument::Midi,              // type
                       std::string("Metronome"),      // name
                       (MidiByte)9,                   // channel
                       dynamic_cast<Device*>(this))); // parent device 


    generatePresentationList();
}

void
MidiDevice::generatePresentationList()
{
    // Fill the presentation list for the instruments
    //
    m_presentationInstrumentList.clear();

    InstrumentList::iterator it;
    for (it = m_instruments.begin(); it != m_instruments.end(); it++)
    {
        if ((*it)->getId() >= MidiInstrumentBase)
            m_presentationInstrumentList.push_back(*it);
    }
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
MidiDevice::setMetronome(InstrumentId instrument,
                         MidiByte msb, MidiByte lsb, MidiByte program,
                         MidiByte pitch,
                         const std::string &name)
{

    m_metronome->pitch = pitch;
    m_metronome->program = program;
    m_metronome->msb = msb;
    m_metronome->lsb = lsb;
    m_metronome->name = name;
    m_metronome->instrument = instrument;
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
    std::stringstream midiDevice;

    midiDevice << "    <device id=\""  << m_id 
               << "\" type=\"midi\">" << std::endl << std::endl;

    // Write out the metronome - watch the MidiBytes
    // when using the stringstream
    //
    midiDevice << "        <metronome "
               << "instrument=\"" << m_metronome->instrument << "\" "
               << "msb=\"" << (int)m_metronome->msb << "\" "
               << "lsb=\"" << (int)m_metronome->lsb << "\" " 
               << "program=\"" << (int)m_metronome->program << "\" "
               << "pitch=\"" << (int)m_metronome->pitch << "\"/>"
               << std::endl << std::endl;

    // and now bank information
    //
    std::vector<MidiBank*>::iterator it;
    InstrumentList::iterator iit;
    ProgramList::iterator pt;

    for (it = m_bankList->begin(); it != m_bankList->end(); it++)
    {
        midiDevice << "        <bank "
                   << "name=\"" << encode((*it)->name) << "\" "
                   << "msb=\"" << (int)(*it)->msb << "\" "
                   << "lsb=\"" << (int)(*it)->lsb << "\">"
                   << std::endl;

        // Slightly inefficient way of doing this until
        // we've sorted out these program changes and
        // bank relationships
        //
        for (pt = m_programList->begin(); pt != m_programList->end(); pt++)
        {
            // if bank on program matches current
            if ((*it)->msb == (*pt)->msb && (*it)->lsb == (*pt)->lsb)
            {
                midiDevice << "            <program "
                           << "id=\"" << (int)(*pt)->program << "\" "
                           << "name=\"" << encode((*pt)->name) << "\"/>" << std::endl;
            }
        }

        midiDevice << "        </bank>" << std::endl << std::endl;
    }

    // Add instruments
    //
    for (iit = m_instruments.begin(); iit != m_instruments.end(); iit++)
        midiDevice << (*iit)->toXmlString();

#if (__GNUC__ < 3)
    midiDevice << "    </device>" << std::endl << std::ends;
#else
    midiDevice << "    </device>" << std::endl;
#endif

    return midiDevice.str();
}

// Only copy across non System instruments
//
InstrumentList&
MidiDevice::getAllInstruments()
{
    return m_instruments;
}

// Omitting special system Instruments
//
InstrumentList&
MidiDevice::getPresentationInstruments()
{
    return m_presentationInstrumentList;
}

void
MidiDevice::addInstrument(Instrument *instrument)
{
    m_instruments.push_back(instrument);
    generatePresentationList();
}

std::string
MidiDevice::getProgramName(MidiByte msb, MidiByte lsb, MidiByte program)
{
    ProgramList::iterator it;

    for (it = m_programList->begin(); it != m_programList->end(); it++)
    {
        // If the bank matches
        if (msb == (*it)->msb && lsb == (*it)->lsb &&
            program == (*it)->program)
            return (*it)->name;
    }

    return std::string("");
}


// Clear down banks
//
void
MidiDevice::clearBankList()
{
    std::vector<MidiBank*>::iterator it;

    for (it = m_bankList->begin(); it != m_bankList->end(); it++)
        delete(*it);

    m_bankList->erase(m_bankList->begin(), m_bankList->end());
}


}


