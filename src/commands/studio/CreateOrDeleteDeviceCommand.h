/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_CREATEORDELETEDEVICECOMMAND_H
#define RG_CREATEORDELETEDEVICECOMMAND_H

#include "base/Device.h"
#include "base/MidiDevice.h"
#include "document/Command.h"  // For NamedCommand

#include <QString>

#include <string>


namespace Rosegarden
{


class Studio;


class CreateOrDeleteDeviceCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::CreateOrDeleteDeviceCommand)

public:
    // Creation constructor
    CreateOrDeleteDeviceCommand
        (Studio *studio,
         const std::string& name,
         Device::DeviceType type,
         MidiDevice::DeviceDirection direction,
         const std::string& connection,
         bool withData = false,
         const std::string &librarianName = "",
         const std::string &librarianEmail = "",
         MidiDevice::VariationType variationType = MidiDevice::NoVariations,
         const BankList& bankList = BankList(),
         const ProgramList& programList = ProgramList(),
         const ControlList& controlList = ControlList(),
         const KeyMappingList& keyMappingList = KeyMappingList()

         ) :
    NamedCommand(getGlobalName(false)),
        m_studio(studio),
        m_deviceName(name),
        m_type(type),
        m_direction(direction),
        m_connection(connection),
        m_deviceId(Device::NO_DEVICE),
        m_baseInstrumentId(MidiInstrumentBase),
        m_deviceCreated(false),
        m_withData(withData),
        m_librarianName(librarianName),
        m_librarianEmail(librarianEmail),
        m_variationType(variationType),
        m_bankList(bankList),
        m_programList(programList),
        m_controlList(controlList),
        m_keyMappingList(keyMappingList)
            { }

    // Deletion constructor
    CreateOrDeleteDeviceCommand(Studio *studio,
                                DeviceId deviceId);

    static QString getGlobalName(bool deletion) {
        return (deletion ? tr("Delete Device") : tr("Create Device"));
    }

    void execute() override;
    void unexecute() override  { execute(); }

private:
    Studio *m_studio;
    std::string m_deviceName;
    Device::DeviceType m_type;
    MidiDevice::DeviceDirection m_direction;
    std::string m_connection;

    DeviceId m_deviceId;
    InstrumentId m_baseInstrumentId;
    /// true: Delete, false: Create
    bool m_deviceCreated;

    // device data
    bool m_withData;
    std::string m_librarianName;
    std::string m_librarianEmail;
    MidiDevice::VariationType m_variationType;
    BankList m_bankList;
    ProgramList m_programList;
    ControlList m_controlList;
    KeyMappingList m_keyMappingList;
};



}

#endif
