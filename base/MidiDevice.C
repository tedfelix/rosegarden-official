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

#include "MidiDevice.h"
#include "Instrument.h"

#include <cstdio>
#include <iostream>
#include <set>

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
    m_direction(Play),
    m_variationType(NoVariations),
    m_librarian(std::pair<std::string, std::string>("<none>", "<none>"))
{
    std::cerr<< "MidiDevice ctor without data for device " << getId() << std::endl;
    generatePresentationList();
}

MidiDevice::MidiDevice(DeviceId id,
                       const std::string &name,
                       DeviceDirection dir):
    Device(id, name, Device::Midi),
    m_programList(new ProgramList()),
    m_bankList(new BankList()),
    m_metronome(0),
    m_direction(dir),
    m_variationType(NoVariations),
    m_librarian(std::pair<std::string, std::string>("<none>", "<none>"))
{
    std::cerr<< "MidiDevice ctor with data for device " << getId() << std::endl;

    generatePresentationList();
}

MidiDevice::MidiDevice(const MidiDevice &dev):
    Device(dev.getId(), dev.getName(), dev.getType()),
    m_programList(new ProgramList()),
    m_bankList(new BankList()),
    m_metronome(0),
    m_direction(dev.getDirection()),
    m_variationType(dev.getVariationType()),
    m_librarian(dev.getLibrarian())
{
    std::cerr<< "MidiDevice copy ctor for device " << getId() << std::endl;

    // Create and assign a metronome if required
    //
    if (dev.getMetronome())
    {
        m_metronome = new MidiMetronome(*dev.getMetronome());
    }

    std::vector<MidiProgram> programs = dev.getPrograms();
    std::vector<MidiBank> banks = dev.getBanks();

    // Construct a new program list
    //
    std::vector<MidiProgram>::iterator it = programs.begin();
    for (; it != programs.end(); it++)
    {
        MidiProgram *prog = new MidiProgram(*it);
        m_programList->push_back(prog);
    }


    // Construct a new bank list
    //
    std::vector<MidiBank>::iterator bIt = banks.begin();
    for (; bIt != banks.end(); bIt++)
    {
        MidiBank *bank = new MidiBank(*bIt);
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

    std::cerr<< "MidiDevice assignment operator for device " << getId() << " from " << dev.getId() << std::endl;

    m_id = dev.getId();
    m_name = dev.getName();
    m_type = dev.getType();
    m_librarian = dev.getLibrarian();

    m_programList->clear();
    m_bankList->clear();
    m_direction = dev.getDirection();
    m_variationType = dev.getVariationType();

    // clear down instruments list
    m_instruments.clear();
    m_presentationInstrumentList.clear();

    // Create and assign a metronome if required
    //
    if (dev.getMetronome())
    {
	if (m_metronome) delete m_metronome;
	m_metronome = new MidiMetronome(*dev.getMetronome());
    }
    else
    {
	delete m_metronome;
	m_metronome = 0;
    }

    std::vector<MidiProgram> programs = dev.getPrograms();
    std::vector<MidiBank> banks = dev.getBanks();

    // Construct a new program list
    //
    std::vector<MidiProgram>::iterator it = programs.begin();
    for (; it != programs.end(); it++)
    {
        MidiProgram *prog = new MidiProgram(*it);
//!!!	prog->percussion = it->percussion;
//        prog->name = it->name;
//        prog->msb = it->msb;
//        prog->lsb = it->lsb;
//        prog->program = it->program;

        m_programList->push_back(prog);
    }


    // Construct a new bank list
    //
    std::vector<MidiBank>::iterator bIt = banks.begin();
    for (; bIt != banks.end(); bIt++)
    {
        MidiBank *bank = new MidiBank(*bIt);
//!!!	bank->percussion = bIt->percussion;
//        bank->name = bIt->name;
//        bank->msb = bIt->msb;
//        bank->lsb = bIt->lsb;

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
    std::cerr<< "MidiDevice dtor for device " << getId() << std::endl;

    delete m_programList;
    delete m_bankList;
    if (m_metronome) delete m_metronome;
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
        if ((*it)->getId() >= MidiInstrumentBase) {
            m_presentationInstrumentList.push_back(*it);
	}
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
			 const MidiMetronome &metronome)
{
    if (!m_metronome) {
	m_metronome = new MidiMetronome(metronome);
    } else {
	*m_metronome = metronome;
    }
}


// Create a Program list
//
StringList
MidiDevice::getProgramList(const MidiBank &bank)
{
    StringList list;
    ProgramList::iterator it;

    for (it = m_programList->begin(); it != m_programList->end(); it++)
    {
        // If the bank matches
        if ((*it)->getBank() == bank)
            list.push_back((*it)->getName());
    }

    return list;
}

// We display banks by name, not by number
//
StringList
MidiDevice::getBankList()
{
    StringList list;
    BankList::iterator it;

    for (it = m_bankList->begin(); it != m_bankList->end(); it++)
        list.push_back((*it)->getName());

    return list;
}

const MidiBank*
MidiDevice::getBankByIndex(int index) const
{
    return (*m_bankList)[index];
}

const MidiBank*
MidiDevice::getBankByMsbLsb(bool percussion, MidiByte msb, MidiByte lsb) const
{
    BankList::iterator it;

    // Bank comparator ignores names

    for (it = m_bankList->begin(); it != m_bankList->end(); it++) 
	if (**it == MidiBank(percussion, msb, lsb)) return *it;

    return 0;
}

const BankList
MidiDevice::getBanks(bool percussion) const
{
    BankList banks;

    for (BankList::const_iterator it = m_bankList->begin();
	 it != m_bankList->end(); ++it) {
	if ((*it)->isPercussion() == percussion) banks.push_back(*it);
    }

    return banks;
}

const BankList
MidiDevice::getBanksByMSB(bool percussion, MidiByte msb) const
{
    BankList banks;

    for (BankList::const_iterator it = m_bankList->begin();
	 it != m_bankList->end(); ++it) {
	if ((*it)->isPercussion() == percussion && (*it)->getMSB() == msb)
	    banks.push_back(*it);
    }

    return banks;
}

const BankList
MidiDevice::getBanksByLSB(bool percussion, MidiByte lsb) const
{
    BankList banks;

    for (BankList::const_iterator it = m_bankList->begin();
	 it != m_bankList->end(); ++it) {
	if ((*it)->isPercussion() == percussion && (*it)->getLSB() == lsb)
	    banks.push_back(*it);
    }

    return banks;
}

MidiByteList
MidiDevice::getDistinctMSBs(bool percussion, int lsb) const
{
    std::set<MidiByte> msbs;

    for (BankList::const_iterator it = m_bankList->begin();
	 it != m_bankList->end(); ++it) {
	if ((*it)->isPercussion() == percussion &&
	    (lsb == -1 || (*it)->getLSB() == lsb)) msbs.insert((*it)->getMSB());
    }

    MidiByteList v;
    for (std::set<MidiByte>::iterator i = msbs.begin(); i != msbs.end(); ++i) {
	v.push_back(*i);
    }

    return v;
}

MidiByteList
MidiDevice::getDistinctLSBs(bool percussion, int msb) const
{
    std::set<MidiByte> lsbs;

    for (BankList::const_iterator it = m_bankList->begin();
	 it != m_bankList->end(); ++it) {
	if ((*it)->isPercussion() == percussion &&
	    (msb == -1 || (*it)->getMSB() == msb)) lsbs.insert((*it)->getLSB());
    }

    MidiByteList v;
    for (std::set<MidiByte>::iterator i = lsbs.begin(); i != lsbs.end(); ++i) {
	v.push_back(*i);
    }

    return v;
}

const ProgramList
MidiDevice::getPrograms(const MidiBank &bank) const
{
    ProgramList programs;

    for (ProgramList::const_iterator it = m_programList->begin();
	 it != m_programList->end(); ++it) {
	if ((*it)->getBank() == bank) programs.push_back(*it);
    }

    return programs;
}

std::string
MidiDevice::getBankName(const MidiBank &bank) const
{
    for (BankList::const_iterator it = m_bankList->begin();
	 it != m_bankList->end(); ++it) {
	if (**it == bank) return (*it)->getName();
    }
    return "";
}

const MidiProgram*
MidiDevice::getProgramByIndex(int index) const
{
    return (*m_programList)[index];
}

const MidiProgram*
MidiDevice::getProgram(const MidiBank &bank, int index) const
{
    ProgramList::iterator it;
    int count = 0;

    for (it = m_programList->begin(); it != m_programList->end(); it++)
    {
        if ((*it)->getBank() == bank)
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
	       << "\" direction=\""    << (m_direction == Play ?
					   "play" : "record")
	       << "\" variation=\""    << (m_variationType == VariationFromLSB ?
					   "LSB" :
					   m_variationType == VariationFromMSB ?
					   "MSB" : "")
	       << "\" connection=\""   << encode(m_connection)
               << "\" type=\"midi\">"  << std::endl << std::endl;

    midiDevice << "        <librarian name=\"" << encode(m_librarian.first)
               << "\" email=\"" << encode(m_librarian.second)
               << "\"/>" << std::endl;

    if (m_metronome)
    {
        // Write out the metronome - watch the MidiBytes
        // when using the stringstream
        //
        midiDevice << "        <metronome "
                   << "instrument=\"" << m_metronome->getInstrument() << "\" "
                   << "msb=\"" << (int)m_metronome->getProgram().getBank().getMSB() << "\" "
                   << "lsb=\"" << (int)m_metronome->getProgram().getBank().getLSB() << "\" " 
                   << "program=\"" << (int)m_metronome->getProgram().getProgram() << "\" "
                   << "pitch=\"" << (int)m_metronome->getPitch() << "\"/>"
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
                   << "name=\"" << encode((*it)->getName()) << "\" "
	           << "percussion=\"" << ((*it)->isPercussion() ? "true" : "false") << "\" "
                   << "msb=\"" << (int)(*it)->getMSB() << "\" "
                   << "lsb=\"" << (int)(*it)->getLSB() << "\">"
                   << std::endl;

        // Slightly inefficient way of doing this until
        // we've sorted out these program changes and
        // bank relationships
        //
        for (pt = m_programList->begin(); pt != m_programList->end(); pt++)
        {
	    if ((*pt)->getBank() == **it)
            {
                midiDevice << "            <program "
                           << "id=\"" << (int)(*pt)->getProgram() << "\" "
                           << "name=\"" << encode((*pt)->getName()) << "\"/>" << std::endl;
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
MidiDevice::getProgramName(const MidiProgram &program) const
{
    ProgramList::iterator it;

    for (it = m_programList->begin(); it != m_programList->end(); it++)
    {
	if (**it == program) return (*it)->getName();
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
	    if (*it == **oIt)
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
	    if (*it == **oIt)
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


