/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "ModifyDeviceCommand.h"

#include "base/Device.h"
#include "base/MidiDevice.h"
#include "base/Studio.h"
#include "gui/application/RosegardenMainWindow.h"

#include <QString>
#include <iostream>


namespace Rosegarden
{

ModifyDeviceCommand::ModifyDeviceCommand(
    Studio *studio,
    DeviceId device,
    const std::string &name,
    const std::string &librarianName,
    const std::string &librarianEmail) :
        NamedCommand(getGlobalName()),
        m_studio(studio),
        m_device(device),
        m_name(name),
        m_librarianName(librarianName),
        m_librarianEmail(librarianEmail),
        m_overwrite(true),
        m_rename(true),
        m_changeVariation(false),
        m_changeBanks(false),
        m_changePrograms(false),
        m_changeControls(false),
        m_changeKeyMappings(false),
        m_clearBankAndProgramList(false)
{}

void ModifyDeviceCommand::setVariation(MidiDevice::VariationType variationType)
{
    m_variationType = variationType;
    m_changeVariation = true;
}

void ModifyDeviceCommand::setBankList(const BankList &bankList)
{
    m_bankList = bankList;
    m_changeBanks = true;
}

void ModifyDeviceCommand::setProgramList(const ProgramList &programList)
{
    m_programList = programList;
    m_changePrograms = true;
}

void ModifyDeviceCommand::setControlList(const ControlList &controlList)
{
    m_controlList = controlList;
    m_changeControls = true;
}

void ModifyDeviceCommand::setKeyMappingList(const KeyMappingList &keyMappingList)
{
    m_keyMappingList = keyMappingList;
    m_changeKeyMappings = true;
}

void
ModifyDeviceCommand::execute()
{
    Device *device = m_studio->getDevice(m_device);
    if (!device) {
        std::cerr << "ERROR: ModifyDeviceCommand::execute(): no such device as " << m_device << std::endl;
        return;
    }

    MidiDevice *midiDevice = dynamic_cast<MidiDevice *>(device);
    if (!midiDevice) {
        std::cerr << "ERROR: ModifyDeviceCommand::execute(): device " << m_device << " is not a MIDI device" << std::endl;
        return;
    }

    // Save Original Values for Undo

    // ??? Really wish we could just m_oldDevice = *(midiDevice).  See below.
    m_oldName = midiDevice->getName();
    m_oldBankList = midiDevice->getBanks();
    m_oldProgramList = midiDevice->getPrograms();
    m_oldControlList = midiDevice->getControlParameters();
    m_oldKeyMappingList = midiDevice->getKeyMappings();
    m_oldLibrarianName = midiDevice->getLibrarianName();
    m_oldLibrarianEmail = midiDevice->getLibrarianEmail();
    m_oldVariationType = midiDevice->getVariationType();
    InstrumentList instruments = midiDevice->getAllInstruments();
    for (size_t i = 0; i < instruments.size(); ++i) {
        // ??? Preserving just the programs isn't enough.  We need
        //     to preserve the rest of the Instrument as well.  However,
        //     the auto/fixed channel feature has made it impossible
        //     to safely make copies of Instrument objects.  Also, Instrument
        //     has an ID.  How should that be handled for undo?  ISTM
        //     that we either need to introduce some sort of copyForUndo()
        //     hack to each object, or develop a set of standards for coding
        //     objects that are undo-safe.  Sounds like a pretty big project.
        m_oldInstrumentPrograms.push_back(instruments[i]->getProgram());
    }

    // Make the Changes

    if (m_changeVariation)
        midiDevice->setVariationType(m_variationType);

    if (m_overwrite) {
        if (m_clearBankAndProgramList) {
            midiDevice->clearBankList();
            midiDevice->clearProgramList();
            midiDevice->clearKeyMappingList();
        } else {
            if (m_changeBanks)
                midiDevice->replaceBankList(m_bankList);
            if (m_changePrograms)
                midiDevice->replaceProgramList(m_programList);
            if (m_changeBanks || m_changePrograms) {
                // Make sure the instruments make sense.
                for (size_t i = 0; i < instruments.size(); ++i) {
                    instruments[i]->pickFirstProgram(
                            midiDevice->isPercussionNumber(i));
                }
            }
        }

        if (m_changeKeyMappings) {
            midiDevice->replaceKeyMappingList(m_keyMappingList);
        }

        if (m_rename)
            midiDevice->setName(m_name);
        midiDevice->setLibrarian(m_librarianName, m_librarianEmail);
    } else {
        if (m_clearBankAndProgramList) {
            midiDevice->clearBankList();
            midiDevice->clearProgramList();
        } else {
            if (m_changeBanks)
                midiDevice->mergeBankList(m_bankList);
            if (m_changePrograms)
                midiDevice->mergeProgramList(m_programList);
        }

        if (m_changeKeyMappings) {
            midiDevice->mergeKeyMappingList(m_keyMappingList);
        }

        if (m_rename) {
            std::string mergeName = midiDevice->getName() +
                                    std::string("/") + m_name;
            midiDevice->setName(mergeName);
        }
    }

    //!!! merge option?
    if (m_changeControls) {
        midiDevice->replaceControlParameters(m_controlList);
    }

    // ??? Instead of this kludge, we should be calling a Studio::hasChanged()
    //     which would then notify all observers (e.g. MIPP) who, in turn,
    //     would update themselves.
    RosegardenMainWindow::self()->uiUpdateKludge();
}

void
ModifyDeviceCommand::unexecute()
{
    Device *device = m_studio->getDevice(m_device);
    if (!device) {
        std::cerr << "ERROR: ModifyDeviceCommand::unexecute(): no such device as " << m_device << std::endl;
        return;
    }

    MidiDevice *midiDevice = dynamic_cast<MidiDevice *>(device);
    if (!midiDevice) {
        std::cerr << "ERROR: ModifyDeviceCommand::unexecute(): device " << m_device << " is not a MIDI device" << std::endl;
        return;
    }

    if (m_rename)
        midiDevice->setName(m_oldName);
    midiDevice->replaceBankList(m_oldBankList);
    midiDevice->replaceProgramList(m_oldProgramList);
    midiDevice->replaceControlParameters(m_oldControlList);
    midiDevice->replaceKeyMappingList(m_oldKeyMappingList);
    midiDevice->setLibrarian(m_oldLibrarianName, m_oldLibrarianEmail);
    if (m_changeVariation)
        midiDevice->setVariationType(m_oldVariationType);

    InstrumentList instruments = midiDevice->getAllInstruments();
    for (size_t i = 0; i < instruments.size(); ++i) {
        instruments[i]->setProgram(m_oldInstrumentPrograms[i]);
    }

    // ??? Instead of this kludge, we should be calling a Studio::hasChanged()
    //     which would then notify all observers (e.g. MIPP) who, in turn,
    //     would update themselves.
    RosegardenMainWindow::self()->uiUpdateKludge();
}

}
