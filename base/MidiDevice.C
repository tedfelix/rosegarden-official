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
    m_metronome(0),
    m_direction(WriteOnly),
    m_librarian(std::pair<std::string, std::string>("<none>", "<none>"))
{
    createInstruments();
}

MidiDevice::MidiDevice(DeviceId id,
                       const std::string &name,
                       DeviceDirection dir):
    Device(id, name, Device::Midi),
    m_programList(new ProgramList()),
    m_bankList(new BankList()),
    m_metronome(0),
    m_direction(dir),
    m_librarian(std::pair<std::string, std::string>("<none>", "<none>"))
{
    createInstruments();
}

MidiDevice::MidiDevice(DeviceId id,
                       const std::string &name,
                       const std::string &label,
                       DeviceDirection dir):
    Device(id, name, label, Device::Midi),
    m_programList(new ProgramList()),
    m_bankList(new BankList()),
    m_metronome(0),
    m_direction(dir),
    m_librarian(std::pair<std::string, std::string>("<none>", "<none>"))
{
    createInstruments();
}

MidiDevice::MidiDevice(const MidiDevice &dev):
    Device(dev.getId(), dev.getName(), dev.getUserLabel(), dev.getType()),
    m_programList(new ProgramList()),
    m_bankList(new BankList()),
    m_metronome(0),
    m_direction(dev.getDirection()),
    m_librarian(dev.getLibrarian())
{
    // Create and assign a metronome if required
    //
    if (dev.getMetronome())
    {
        m_metronome = new MidiMetronome();
        m_metronome->pitch = dev.getMetronome()->pitch;
        m_metronome->instrument = dev.getMetronome()->instrument;
        m_metronome->msb = dev.getMetronome()->msb;
        m_metronome->lsb = dev.getMetronome()->lsb;
        m_metronome->program = dev.getMetronome()->program;
    }

    std::vector<MidiProgram> programs = dev.getPrograms();
    std::vector<MidiBank> banks = dev.getBanks();

    // Construct a new program list
    //
    std::vector<MidiProgram>::iterator it = programs.begin();
    for (; it != programs.end(); it++)
    {
        MidiProgram *prog = new MidiProgram();
        prog->name = it->name;
        prog->msb = it->msb;
        prog->lsb = it->lsb;
        prog->program = it->program;

        m_programList->push_back(prog);
    }


    // Construct a new bank list
    //
    std::vector<MidiBank>::iterator bIt = banks.begin();
    for (; bIt != banks.end(); bIt++)
    {
        MidiBank *bank = new MidiBank();
        bank->name = bIt->name;
        bank->msb = bIt->msb;
        bank->lsb = bIt->lsb;

        m_bankList->push_back(bank);
    }

    // Copy the instruments
    //
    InstrumentList insList = dev.getAllInstruments();
    InstrumentList::iterator iIt = insList.begin();
    for (; iIt != insList.end(); iIt++)
    {
        Instrument *newInst = new Instrument(**iIt);
        newInst->setDevice(this);
        m_instruments.push_back(newInst);
    }

    // generate presentation instruments
    generatePresentationList();
}

MidiDevice &
MidiDevice::operator=(const MidiDevice &dev)
{
    if (&dev == this) return *this;

    m_id = dev.getId();
    m_name = dev.getName();
    m_type = dev.getType();
    m_librarian = dev.getLibrarian();

    m_programList->clear();
    m_bankList->clear();
    m_direction = dev.getDirection();

    // clear down instruments list
    m_instruments.clear();
    m_presentationInstrumentList.clear();

    // Device
    m_label = dev.getUserLabel();

    // Create and assign a metronome if required
    //
    if (dev.getMetronome())
    {
        if (dev.getMetronome() == 0)
            m_metronome = new MidiMetronome();

        m_metronome->pitch = dev.getMetronome()->pitch;
        m_metronome->instrument = dev.getMetronome()->instrument;
        m_metronome->msb = dev.getMetronome()->msb;
        m_metronome->lsb = dev.getMetronome()->lsb;
        m_metronome->program = dev.getMetronome()->program;
    }
    else
    {
        if (m_metronome) delete m_metronome;
    }


    std::vector<MidiProgram> programs = dev.getPrograms();
    std::vector<MidiBank> banks = dev.getBanks();

    // Construct a new program list
    //
    std::vector<MidiProgram>::iterator it = programs.begin();
    for (; it != programs.end(); it++)
    {
        MidiProgram *prog = new MidiProgram();
        prog->name = it->name;
        prog->msb = it->msb;
        prog->lsb = it->lsb;
        prog->program = it->program;

        m_programList->push_back(prog);
    }


    // Construct a new bank list
    //
    std::vector<MidiBank>::iterator bIt = banks.begin();
    for (; bIt != banks.end(); bIt++)
    {
        MidiBank *bank = new MidiBank();
        bank->name = bIt->name;
        bank->msb = bIt->msb;
        bank->lsb = bIt->lsb;

        m_bankList->push_back(bank);
    }

    // Copy the instruments
    //
    InstrumentList insList = dev.getAllInstruments();
    InstrumentList::iterator iIt = insList.begin();
    for (; iIt != insList.end(); iIt++)
    {
        Instrument *newInst = new Instrument(**iIt);
        newInst->setDevice(this);
        m_instruments.push_back(newInst);
    }

    // generate presentation instruments
    generatePresentationList();

    return (*this);
}



MidiDevice::~MidiDevice()
{
    delete m_programList;
    delete m_bankList;
    if (m_metronome) delete m_metronome;
}

void
MidiDevice::createInstruments()
{
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
MidiDevice::removeMetronome()
{
    if (m_metronome)
    {
        delete m_metronome;
        m_metronome = 0;
    }

}

void
MidiDevice::setMetronome(InstrumentId instrument,
                         MidiByte msb, MidiByte lsb, MidiByte program,
                         MidiByte pitch,
                         const std::string &name)
{

    if (m_metronome == 0)
    {
        m_metronome = new MidiMetronome();
    }

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

MidiProgram*
MidiDevice::getProgram(MidiByte msb, MidiByte lsb, int index)
{
    ProgramList::iterator it;
    int count = 0;

    for (it = m_programList->begin(); it != m_programList->end(); it++)
    {
        if ((*it)->msb == msb && (*it)->lsb == lsb)
        {
            if (count == index)
                return (*it);
            count++;
        }
    }

    return 0;
}

std::string
MidiDevice::toXmlString()
{
    std::stringstream midiDevice;

    midiDevice << "    <device id=\""  << m_id 
               << "\" name=\""         << m_name 
               << "\" type=\"midi\">" << std::endl << std::endl;

    midiDevice << "        <librarian name=\"" << encode(m_librarian.first)
               << "\" email=\"" << encode(m_librarian.second) << std::endl
               << "\"/>" << std::endl;

    if (m_metronome)
    {
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
    }

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
InstrumentList
MidiDevice::getAllInstruments() const
{
    return m_instruments;
}

// Omitting special system Instruments
//
InstrumentList
MidiDevice::getPresentationInstruments() const
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


std::vector<MidiBank>
MidiDevice::getBanks() const
{
    std::vector<MidiBank> bank;
    std::vector<MidiBank*>::iterator it;

    for (it = m_bankList->begin(); it != m_bankList->end(); it++)
        bank.push_back(**it);

    return bank;
}

std::vector<MidiProgram>
MidiDevice::getPrograms() const
{
    std::vector<MidiProgram> program;
    std::vector<MidiProgram*>::iterator it;

    for (it = m_programList->begin(); it != m_programList->end(); it++)
        program.push_back(**it);

    return program;
}

void
MidiDevice::replaceBankList(const std::vector<Rosegarden::MidiBank> &bank)
{
    clearBankList();

    std::vector<Rosegarden::MidiBank>::const_iterator it = bank.begin();
    for (; it != bank.end(); it++)
    {
        addBank(new MidiBank(*it));
    }

}

void
MidiDevice::replaceProgramList(const std::vector<Rosegarden::MidiProgram> &program)
{
    clearProgramList();

    std::vector<Rosegarden::MidiProgram>::const_iterator it = program.begin();
    for (; it != program.end(); it++)
    {
        addProgram(new MidiProgram(*it));
    }
}

// Merge the new bank list in without duplication
//
void
MidiDevice::mergeBankList(const std::vector<Rosegarden::MidiBank> &bank)
{
    std::vector<Rosegarden::MidiBank>::const_iterator it;
    BankList::iterator oIt;
    bool clash = false;
    
    for (it = bank.begin(); it != bank.end(); it++)
    {
        for (oIt = m_bankList->begin(); oIt != m_bankList->end(); oIt++)
        {
            if (it->msb == (*oIt)->msb && it->lsb == (*oIt)->lsb)
            {
                clash = true;
                break;
            }
        }

        if (clash == false)
            addBank(new MidiBank(*it));
        else
            clash = false;
    }

}

void
MidiDevice::mergeProgramList(const std::vector<Rosegarden::MidiProgram> &program)
{
    std::vector<Rosegarden::MidiProgram>::const_iterator it;
    ProgramList::iterator oIt;
    bool clash = false;

    for (it = program.begin(); it != program.end(); it++)
    {
        for (oIt = m_programList->begin(); oIt != m_programList->end(); oIt++)
        {
            if (it->msb == (*oIt)->msb && it->lsb == (*oIt)->lsb &&
                it->program == (*oIt)->program)
            {
                clash = true;
                break;
            }
        }

        if (clash == false)
            addProgram(new MidiProgram(*it));
        else
            clash = false;
    }
}


}


