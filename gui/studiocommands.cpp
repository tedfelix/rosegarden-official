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

#include "studiocommands.h"

#include "Studio.h"
#include "MidiDevice.h"

ModifyDeviceCommand::ModifyDeviceCommand(
        Rosegarden::Studio *studio,
        int device,
        const std::string &name,
        std::vector<Rosegarden::MidiBank> bankList,
        std::vector<Rosegarden::MidiProgram> programList):
    XKCommand(getGlobalName()),
    m_studio(studio),
    m_device(device),
    m_name(name),
    m_bankList(bankList),
    m_programList(programList)
{
}

void
ModifyDeviceCommand::execute()
{
    Rosegarden::MidiDevice *device = m_studio->getMidiDevice(m_device);

    m_oldName = device->getName();
    m_oldBankList = device->getBanks();
    m_oldProgramList = device->getPrograms();

    device->setName(m_name);
    device->replaceBankList(m_bankList);
    device->replaceProgramList(m_programList);
}

void
ModifyDeviceCommand::unexecute()
{
    Rosegarden::MidiDevice *device = m_studio->getMidiDevice(m_device);

    device->setName(m_oldName);
    device->replaceBankList(m_oldBankList);
    device->replaceProgramList(m_oldProgramList);

}


