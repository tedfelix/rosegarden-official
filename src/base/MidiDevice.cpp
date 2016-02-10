/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MidiDevice]"

#include "MidiDevice.h"
#include "sound/Midi.h"
#include "base/AllocateChannels.h"
#include "base/ControlParameter.h"
#include "base/Instrument.h"
#include "base/MidiTypes.h"
#include "misc/Debug.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <set>

#include <sstream>

#include <QString>

namespace Rosegarden
{

#if 0
MidiDevice::MidiDevice():
    Device(0, "Default Midi Device", Device::Midi),
    m_metronome(0),
    m_direction(Play),
    m_variationType(NoVariations),
    m_librarian(std::pair<std::string, std::string>("<none>", "<none>")),
    m_allocator(new AllocateChannels(ChannelSetup::MIDI))
{
    createInstruments(MidiInstrumentBase);
    generatePresentationList();
    generateDefaultControllers();
    deviceToInstrControllerPush();

    // create a default Metronome
    m_metronome = new MidiMetronome(MidiInstrumentBase + 9);
}
#endif

MidiDevice::MidiDevice(DeviceId id,
                       InstrumentId ibase,
                       const std::string &name,
                       DeviceDirection dir):
    Device(id, name, Device::Midi),
    m_metronome(0),
    m_direction(dir),
    m_variationType(NoVariations),
    m_librarian(std::pair<std::string, std::string>("<none>", "<none>")),
    m_allocator(new AllocateChannels(ChannelSetup::MIDI))
{
    createInstruments(ibase);
    generatePresentationList();
    generateDefaultControllers();
    deviceToInstrControllerPush();

    // create a default Metronome
    m_metronome = new MidiMetronome(MidiInstrumentBase + 9);
}

#if 0
MidiDevice::MidiDevice(DeviceId id,
                       InstrumentId ibase,
                       const MidiDevice &dev) :
    Device(id, dev.getName(), Device::Midi),
    m_programList(dev.m_programList),
    m_bankList(dev.m_bankList),
    m_controlList(0),
    m_keyMappingList(dev.m_keyMappingList),
    m_metronome(0),
    m_direction(dev.getDirection()),
    m_variationType(dev.getVariationType()),
    m_librarian(dev.getLibrarian()),
    m_allocator(new AllocateChannels(ChannelSetup::MIDI))
{
    createInstruments(ibase);

    // Populate device and Instrument with Controllers.
    ControlList::const_iterator cIt = dev.m_controlList.begin();
    for(; cIt != dev.m_controlList.end(); ++cIt) {
        addControlParameter(*cIt, true);
    }
    
    
    // Create and assign a metronome if required
    //
    if (dev.getMetronome()) {
        m_metronome = new MidiMetronome(*dev.getMetronome());
    }

    generatePresentationList();
}
#endif

MidiDevice::MidiDevice(const MidiDevice &dev) :
    Device(dev.getId(), dev.getName(), dev.getType()),
    Controllable(),
    m_programList(dev.m_programList),
    m_bankList(dev.m_bankList),
    m_controlList(dev.m_controlList),
    m_keyMappingList(dev.m_keyMappingList),
    m_metronome(0),
    m_direction(dev.getDirection()),
    m_variationType(dev.getVariationType()),
    m_librarian(dev.getLibrarian()),
    m_allocator(new AllocateChannels(ChannelSetup::MIDI))
{
    // Create and assign a metronome if required
    //
    if (dev.getMetronome())
    {
        m_metronome = new MidiMetronome(*dev.getMetronome());
    }

    // Copy the instruments
    //
    InstrumentList insList = dev.getAllInstruments();
    InstrumentList::iterator iIt = insList.begin();
    for (; iIt != insList.end(); ++iIt)
    {
        Instrument *newInst = new Instrument(**iIt);
        newInst->setDevice(this);
        m_instruments.push_back(newInst);
    }

    // generate presentation instruments
    generatePresentationList();
}

#if 0
MidiDevice &
MidiDevice::operator=(const MidiDevice &dev)
{
    if (&dev == this) return *this;

    m_id = dev.getId();
    m_name = dev.getName();
    m_type = dev.getType();
    m_librarian = dev.getLibrarian();

    m_keyMappingList = dev.getKeyMappings(),
    m_programList = dev.getPrograms();
    m_bankList = dev.getBanks();
    m_controlList = dev.getControlParameters();
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

    if (m_allocator) { delete m_allocator; }
    m_allocator = new AllocateChannels(ChannelSetup::MIDI);

    // Copy the instruments
    //
    InstrumentList insList = dev.getAllInstruments();
    InstrumentList::iterator iIt = insList.begin();
    for (; iIt != insList.end(); ++iIt)
    {
        Instrument *newInst = new Instrument(**iIt);
        newInst->setDevice(this);
        m_instruments.push_back(newInst);
    }

    // generate presentation instruments
    generatePresentationList();

    return (*this);
}
#endif

MidiDevice::~MidiDevice()
{
    delete m_metronome;
    delete m_allocator;

    //!!! delete key mappings
}

AllocateChannels *
MidiDevice::getAllocator(void)
{ return m_allocator; }

void
MidiDevice::createInstruments(InstrumentId base)
{
    for (int i = 0; i < 16; ++i) {
        Instrument *instrument = new Instrument
            (base + i, Instrument::Midi, "", i, this);
        instrument->setFixedChannel();
        // ??? Since we don't have a connection yet, this makes
        //     little sense.
        //instrument->sendChannelSetup();
        addInstrument(instrument);
    }
    renameInstruments();
}

void
MidiDevice::renameInstruments()
{
    for (int i = 0; i < 16; ++i) {
        m_instruments[i]->setName
            (QString("%1 #%2%3")
             .arg(getName().c_str())
             .arg(i+1)
             .arg(isPercussionNumber(i) ? "[D]" : "")
             .toUtf8().data());
    }
}

void
MidiDevice::generatePresentationList()
{
    // Fill the presentation list for the instruments
    //
    m_presentationInstrumentList.clear();

    InstrumentList::iterator it;
    for (it = m_instruments.begin(); it != m_instruments.end(); ++it)
    {
        if ((*it)->getId() >= MidiInstrumentBase) {
            m_presentationInstrumentList.push_back(*it);
        }
    }
}

void
MidiDevice::generateDefaultControllers()
{
    m_controlList.clear();

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
        addControlParameter(con, false);
    }
}

void
MidiDevice::deviceToInstrControllerPush()
{
    // Copy the instruments
    //

    InstrumentList::iterator iIt = m_instruments.begin();
    for (; iIt != m_instruments.end(); ++iIt)
    {
        (*iIt)->clearStaticControllers();
        ControlList::const_iterator cIt = m_controlList.begin();
        for (; cIt != m_controlList.end(); ++cIt)
        {
            // It appears -1 means not to display an IPB controller
            if (isVisibleControlParameter(*cIt)) {
                MidiByte controllerValue = cIt->getControllerValue();
                int defaultValue = cIt->getDefault();
                (*iIt)->setControllerValue(controllerValue, defaultValue);
            }
        }        
    }
}

void
MidiDevice::clearBankList()
{
    m_bankList.clear();
}

void
MidiDevice::clearProgramList()
{
    m_programList.clear();
}

void
MidiDevice::clearKeyMappingList()
{
    m_keyMappingList.clear();
}

void
MidiDevice::clearControlList()
{
    // Clear down instrument controllers first.
    InstrumentList insList = getAllInstruments();
    InstrumentList::iterator iIt = insList.begin();

    for(; iIt != insList.end(); ++iIt) {
        (*iIt)->clearStaticControllers();
    }

    m_controlList.clear();
}

void
MidiDevice::addProgram(const MidiProgram &prog)
{
    // Refuse duplicates
    for (ProgramList::const_iterator it = m_programList.begin();
         it != m_programList.end(); ++it) {
        if (it->partialCompare(prog)) return;
    }

    m_programList.push_back(prog);
}

void 
MidiDevice::addBank(const MidiBank &bank)
{
    m_bankList.push_back(bank);
}

void
MidiDevice::setMetronome(const MidiMetronome &metronome)
{
    delete m_metronome;
    m_metronome = new MidiMetronome(metronome);
}

BankList
MidiDevice::getBanks(bool percussion) const
{
    BankList banks;

    for (BankList::const_iterator it = m_bankList.begin();
         it != m_bankList.end(); ++it) {
        if (it->isPercussion() == percussion) banks.push_back(*it);
    }

    return banks;
}

BankList
MidiDevice::getBanksByMSB(bool percussion, MidiByte msb) const
{
    BankList banks;

    for (BankList::const_iterator it = m_bankList.begin();
         it != m_bankList.end(); ++it) {
        if (it->isPercussion() == percussion && it->getMSB() == msb)
            banks.push_back(*it);
    }

    return banks;
}

BankList
MidiDevice::getBanksByLSB(bool percussion, MidiByte lsb) const
{
    BankList banks;

    for (BankList::const_iterator it = m_bankList.begin();
         it != m_bankList.end(); ++it) {
        if (it->isPercussion() == percussion && it->getLSB() == lsb)
            banks.push_back(*it);
    }

    return banks;
}

const MidiBank *
MidiDevice::getBankByName(const std::string &name) const
{
    for (BankList::const_iterator i = m_bankList.begin();
         i != m_bankList.end(); ++i) {
        if (i->getName() == name) return &(*i);
    }
    return 0;
}

MidiByteList
MidiDevice::getDistinctMSBs(bool percussion, int lsb) const
{
    std::set<MidiByte> msbs;

    for (BankList::const_iterator it = m_bankList.begin();
         it != m_bankList.end(); ++it) {
        if (it->isPercussion() == percussion &&
            (lsb == -1 || it->getLSB() == lsb)) msbs.insert(it->getMSB());
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

    for (BankList::const_iterator it = m_bankList.begin();
         it != m_bankList.end(); ++it) {
        if (it->isPercussion() == percussion &&
            (msb == -1 || it->getMSB() == msb)) lsbs.insert(it->getLSB());
    }

    MidiByteList v;
    for (std::set<MidiByte>::iterator i = lsbs.begin(); i != lsbs.end(); ++i) {
        v.push_back(*i);
    }

    return v;
}

ProgramList
MidiDevice::getPrograms(const MidiBank &bank) const
{
    ProgramList programs;

    for (ProgramList::const_iterator it = m_programList.begin();
         it != m_programList.end(); ++it) {
        if (it->getBank().partialCompare(bank)) programs.push_back(*it);
    }

    return programs;
}

ProgramList
MidiDevice::getPrograms0thVariation(bool percussion, const MidiBank &bank) const
{
    // If we aren't in variations mode, just use getPrograms().
    if (m_variationType == NoVariations)
        return getPrograms(bank);

    // Get the variation bank list for this bank
    BankList bankList;
    if (m_variationType == VariationFromMSB) {
        bankList = getBanksByLSB(percussion, bank.getLSB());
    } else {
        bankList = getBanksByMSB(percussion, bank.getMSB());
    }

    if (!bankList.empty()) {
        MidiBank firstBank = bankList.front();
        return getPrograms(firstBank);
    }

    return ProgramList();
}

std::string
MidiDevice::getBankName(const MidiBank &bank) const
{
    for (BankList::const_iterator it = m_bankList.begin();
         it != m_bankList.end(); ++it) {
        if ((*it).partialCompare(bank)) return it->getName();
    }
    return "";
}

void
MidiDevice::addKeyMapping(const MidiKeyMapping &mapping)
{
    //!!! handle dup names
    m_keyMappingList.push_back(mapping);
}

const MidiKeyMapping *
MidiDevice::getKeyMappingByName(const std::string &name) const
{
    for (KeyMappingList::const_iterator i = m_keyMappingList.begin();
         i != m_keyMappingList.end(); ++i) {
        if (i->getName() == name) return &(*i);
    }
    return 0;
}

const MidiKeyMapping *
MidiDevice::getKeyMappingForProgram(const MidiProgram &program) const
{
    ProgramList::const_iterator it;

    for (it = m_programList.begin(); it != m_programList.end(); ++it) {
        if (it->partialCompare(program)) {
            std::string kmn = it->getKeyMapping();
            if (kmn == "") return 0;
            return getKeyMappingByName(kmn);
        }
    }

    return 0;
}

void
MidiDevice::setKeyMappingForProgram(const MidiProgram &program,
                                    std::string mapping)
{
    ProgramList::iterator it;

    for (it = m_programList.begin(); it != m_programList.end(); ++it) {
        if (it->partialCompare(program)) {
            it->setKeyMapping(mapping);
        }
    }
}
    

std::string
MidiDevice::toXmlString() const
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

    // and now bank information
    //
    BankList::const_iterator it;
    InstrumentList::const_iterator iit;
    ProgramList::const_iterator pt;

    for (it = m_bankList.begin(); it != m_bankList.end(); ++it)
    {
        midiDevice << "        <bank "
                   << "name=\"" << encode(it->getName()) << "\" "
                   << "percussion=\"" << (it->isPercussion() ? "true" : "false") << "\" "
                   << "msb=\"" << (int)it->getMSB() << "\" "
                   << "lsb=\"" << (int)it->getLSB() << "\">"
                   << std::endl;

        // Not terribly efficient
        //
        for (pt = m_programList.begin(); pt != m_programList.end(); ++pt)
        {
            if (pt->getBank().partialCompare(*it))
            {
                midiDevice << "            <program "
                           << "id=\"" << (int)pt->getProgram() << "\" "
                           << "name=\"" << encode(pt->getName()) << "\" ";
                if (!pt->getKeyMapping().empty()) {
                    midiDevice << "keymapping=\"" 
                               << encode(pt->getKeyMapping()) << "\" ";
                }
                midiDevice << "/>" << std::endl;
            }
        }

        midiDevice << "        </bank>" << std::endl << std::endl;
    }

    // Now controllers (before Instruments, which can depend on 
    // Controller colours)
    //
    midiDevice << "        <controls>" << std::endl;
    ControlList::const_iterator cIt;
    for (cIt = m_controlList.begin(); cIt != m_controlList.end() ; ++cIt)
        midiDevice << cIt->toXmlString();
    midiDevice << "        </controls>" << std::endl << std::endl;

    // Add instruments
    //
    for (iit = m_instruments.begin(); iit != m_instruments.end(); ++iit)
        midiDevice << (*iit)->toXmlString();

    KeyMappingList::const_iterator kit;

    for (kit = m_keyMappingList.begin(); kit != m_keyMappingList.end(); ++kit)
    {
        midiDevice << "        <keymapping "
                   << "name=\"" << encode(kit->getName()) << "\">\n";

        for (MidiKeyMapping::KeyNameMap::const_iterator nmi =
                 kit->getMap().begin(); nmi != kit->getMap().end(); ++nmi) {
            midiDevice << "          <key number=\"" << (int)nmi->first
                       << "\" name=\"" << encode(nmi->second) << "\"/>\n";
        }

        midiDevice << "        </keymapping>\n";
    }

    midiDevice << "    </device>" << std::endl;

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
    // Check / add controls to this instrument
    // No controls are pushed when called from the contructor since they are
    // not generated yet!
    ControlList::const_iterator it = m_controlList.begin();
    for (;
         it != m_controlList.end(); ++it)
    {
        if (isVisibleControlParameter(*it)) {
            int controller = (*it).getControllerValue();
            try {
                instrument->getControllerValue(controller);
            } catch(...) {
                instrument->setControllerValue(controller, it->getDefault());
            }
        }
    }
    
    m_instruments.push_back(instrument);
    generatePresentationList();
}

std::string
MidiDevice::getProgramName(const MidiProgram &program) const
{
    ProgramList::const_iterator it;

    for (it = m_programList.begin(); it != m_programList.end(); ++it)
    {
        if (it->partialCompare(program)) return it->getName();
    }

    return std::string("");
}

void
MidiDevice::replaceBankList(const BankList &bankList)
{
    m_bankList = bankList;
}

void
MidiDevice::replaceProgramList(const ProgramList &programList)
{
    m_programList = programList;
}

void
MidiDevice::replaceKeyMappingList(const KeyMappingList &keyMappingList)
{
    m_keyMappingList = keyMappingList;
}


// Merge the new bank list in without duplication
//
void
MidiDevice::mergeBankList(const BankList &bankList)
{
    BankList::const_iterator it;
    BankList::iterator oIt;
    bool clash = false;
    
    for (it = bankList.begin(); it != bankList.end(); ++it)
    {
        for (oIt = m_bankList.begin(); oIt != m_bankList.end(); ++oIt)
        {
            if ((*it).partialCompare(*oIt))
            {
                clash = true;
                break;
            }
        }

        if (clash == false)
            addBank(*it);
        else
            clash = false;
    }

}

void
MidiDevice::mergeProgramList(const ProgramList &programList)
{
    ProgramList::const_iterator it;
    ProgramList::iterator oIt;
    bool clash = false;

    for (it = programList.begin(); it != programList.end(); ++it)
    {
        for (oIt = m_programList.begin(); oIt != m_programList.end(); ++oIt)
        {
            if (it->partialCompare(*oIt))
            {
                clash = true;
                break;
            }
        }

        if (clash == false)
            addProgram(*it);
        else
            clash = false;
    }
}

void
MidiDevice::mergeKeyMappingList(const KeyMappingList &keyMappingList)
{
    KeyMappingList::const_iterator it;
    KeyMappingList::iterator oIt;
    bool clash = false;

    for (it = keyMappingList.begin(); it != keyMappingList.end(); ++it)
    {
        for (oIt = m_keyMappingList.begin(); oIt != m_keyMappingList.end(); ++oIt)
        {
            if (it->getName() == oIt->getName())
            {
                clash = true;
                break;
            }
        }

        if (clash == false)
            addKeyMapping(*it);
        else
            clash = false;
    }
}

void
MidiDevice::addControlParameter(const ControlParameter &con,
                                bool propagateToInstruments)
{
    if (isUniqueControlParameter(con)) { //Don't allow duplicates
        m_controlList.push_back(con);
        if (propagateToInstruments && isVisibleControlParameter(con)) {
            addControlToInstrument(con);
        }
    }
}

// Add controller CON at INDEX, shifting further controllers one
// position forwards.
// Used only by RemoveControlParameterCommand.
void
MidiDevice::addControlParameter(const ControlParameter &con, int index,
                                bool propagateToInstruments)
{
    ControlList controls;

    // if we're out of range just add the control
    if (index >= (int)m_controlList.size())
    {
        addControlParameter(con, propagateToInstruments);
        return;
    }

    // Rebuild the ControlList entry by entry, placing CON at INDEX.
    // For entry INDEX we do two push_back's, for other entries we do
    // one.
    for (int i = 0; i < (int)m_controlList.size(); ++i)
    {
        if (index == i) {
            controls.push_back(con);
            // !!! This seems to do more than we need, since we
            // discard the original m_controlList.
            addControlParameter(con, propagateToInstruments);
        }
        controls.push_back(m_controlList[i]);
    }

    // Assign the ControlList we just made.
    m_controlList = controls;
}


bool
MidiDevice::removeControlParameter(int index)
{
    ControlList::iterator it = m_controlList.begin();
    int i = 0;

    for (; it != m_controlList.end(); ++it)
    {
        if (index == i)
        {
            removeControlFromInstrument(*it);   
            m_controlList.erase(it);
            return true;
        }
        i++;
    }

    return false;
}

bool
MidiDevice::modifyControlParameter(const ControlParameter &con, int index)
{
    if (index < 0 || index > (int)m_controlList.size()) return false;
    removeControlFromInstrument(m_controlList[index]);
    m_controlList[index] = con;
    addControlToInstrument(con);
    return true;
}

void
MidiDevice::replaceControlParameters(const ControlList &con)
{
    // Clear down instrument controllers in preparation for replace.
    InstrumentList insList = getAllInstruments();
    InstrumentList::iterator iIt = insList.begin();

    for(; iIt != insList.end(); ++iIt) {
        (*iIt)->clearStaticControllers();
    }

    // Clear the Device control list
    m_controlList.clear();
    
    // Now add the controllers to the device,    
    ControlList::const_iterator cIt = con.begin();
    for(; cIt != con.end(); ++cIt) {
        addControlParameter(*cIt, true);
    }
}

#if 0
// Conform instrument controllers to a new setup.
void
MidiDevice::
conformInstrumentControllers(void)
{
    InstrumentList insList = getAllInstruments();

    // Treat each instrument
    for(InstrumentList::iterator iIt = insList.begin();
        iIt != insList.end();
        ++iIt) {
        // Get this instrument's static controllers.  As a seperate
        // copy, so it's not munged when we erase controllers.
        StaticControllers staticControllers = 
            (*iIt)->getStaticControllers();

        for (StaticControllerIterator it = staticControllers.begin();
             it != staticControllers.end();
             ++it) {
            MidiByte conNumber = it->first;

            const ControlParameter * con =
                findControlParameter(Rosegarden::Controller::EventType,
                                     conNumber);
            if (!con) {
                // It doesn't exist in device, so remove it from
                // instrument.
                (*iIt)->removeStaticController(conNumber);
            }
            else if ((*iIt)->getControllerValue(conNumber) == 0) {
                // Controller value probably has an old default value,
                // so set it to the new default.
                MidiByte value = con->getDefault();
                (*iIt)->setControllerValue(conNumber, value);
            } 
        }
    }
}
#endif

// Check to see if passed ControlParameter is unique.  Either the
// type must be unique or in the case of Controller::EventType the
// ControllerValue must be unique.
//
// Controllers (Control type)
//
//
bool 
MidiDevice::isUniqueControlParameter(const ControlParameter &con) const
{
    return
        findControlParameter(con.getType(), con.getControllerValue()) == 0;
}

const ControlParameter *
MidiDevice::
findControlParameter(std::string type, MidiByte conNumber) const
{
    ControlList::const_iterator it = m_controlList.begin();

    for (; it != m_controlList.end(); ++it)
    {
        if (it->getType() == type)
        {
            if (it->getType() == Rosegarden::Controller::EventType &&
                it->getControllerValue() != conNumber)
                { continue; }
            return &*it;
        }
    }
    return 0;
}

bool 
MidiDevice::isVisibleControlParameter(const ControlParameter &con) const
{
    return (con.getIPBPosition() > -1);
}

void
MidiDevice::addControlToInstrument(const ControlParameter &con)
{
    if (!isVisibleControlParameter(con)) {
        return;
    }

    // Run through all of this devices instruments and add default controls and
    // values to them.
    InstrumentList insList = getAllInstruments();
    InstrumentList::iterator iIt = insList.begin();

    for(; iIt != insList.end(); ++iIt) {
        MidiByte conNumber = con.getControllerValue();
        MidiByte conValue = con.getDefault();
        (*iIt)->setControllerValue(conNumber, conValue);
    }    
}

void
MidiDevice::removeControlFromInstrument(const ControlParameter &con)
{
    InstrumentList insList = getAllInstruments();
    InstrumentList::iterator iIt = insList.begin();

    for(; iIt != insList.end(); ++iIt) {
        (*iIt)->removeStaticController(con.getControllerValue());
    }
}

ControlList
MidiDevice::getIPBControlParameters() const
{
    ControlList retList;

    Rosegarden::MidiByte MIDI_CONTROLLER_VOLUME = 0x07;

    for (ControlList::const_iterator it = m_controlList.begin();
         it != m_controlList.end(); ++it)
    {
        if (it->getIPBPosition() != -1 && 
            it->getControllerValue() != MIDI_CONTROLLER_VOLUME)
            retList.push_back(*it);
    }

    return retList;
}

ControlParameter *
MidiDevice::getControlParameter(int index)
{
    if (index >= 0 && ((unsigned int)index) < (unsigned int)m_controlList.size())
        return &m_controlList[index];

    return 0;
}

const ControlParameter *
MidiDevice::getControlParameter(int index) const
{
    return ((MidiDevice *)this)->getControlParameter(index);
}

ControlParameter *
MidiDevice::getControlParameter(const std::string &type, Rosegarden::MidiByte controllerValue)
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

const ControlParameter *
MidiDevice::getControlParameter(const std::string &type, Rosegarden::MidiByte controllerValue) const
{
    return ((MidiDevice *)this)->getControlParameter(type, controllerValue);
}

}

