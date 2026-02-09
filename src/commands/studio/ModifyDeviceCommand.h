/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MODIFYDEVICECOMMAND_H
#define RG_MODIFYDEVICECOMMAND_H

#include "base/Device.h"
#include "base/MidiDevice.h"
#include "document/Command.h"

#include <QString>
#include <QCoreApplication>

#include <string>


namespace Rosegarden
{


class Studio;


class ModifyDeviceCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::ModifyDeviceCommand)

public:

    ModifyDeviceCommand(Studio *studio,
                        DeviceId deviceID,
                        const std::string &name,
                        const std::string &librarianName,
                        const std::string &librarianEmail,
                        const QString &commandName = "");

    void setVariation(MidiDevice::VariationType variationType);
    void setBankSelectType(MidiDevice::BankSelectType bankSelectType);
    void setBankList(const BankList &bankList);
    void setProgramList(const ProgramList &programList);
    void setControlList(const ControlList &controlList);
    void setKeyMappingList(const KeyMappingList &keyMappingList);
    void setOverwrite(bool overwrite)  { m_overwrite = overwrite; }
    void setRename(bool rename)  { m_rename = rename; }

    static QString getGlobalName()  { return tr("Modify &MIDI Bank"); }

    void execute() override;
    void unexecute() override;

private:

    Studio *m_studio;
    DeviceId m_deviceID;

    // Bank Select Type
    MidiDevice::BankSelectType m_oldBankSelectType{
            MidiDevice::BankSelectType::Normal};
    bool m_changeBankSelectType{false};
    MidiDevice::BankSelectType m_bankSelectType{
            MidiDevice::BankSelectType::Normal};

    std::string                m_deviceName;
    std::string                m_librarianName;
    std::string                m_librarianEmail;
    MidiDevice::VariationType  m_variationType{MidiDevice::NoVariations};
    BankList                   m_bankList;
    ProgramList                m_programList;
    ControlList                m_controlList;
    KeyMappingList             m_keyMappingList;

    std::string                m_oldName;
    std::string                m_oldLibrarianName;
    std::string                m_oldLibrarianEmail;
    MidiDevice::VariationType  m_oldVariationType{MidiDevice::NoVariations};
    BankList                   m_oldBankList;
    ProgramList                m_oldProgramList;
    ControlList                m_oldControlList;
    KeyMappingList             m_oldKeyMappingList;
    ProgramList                m_oldInstrumentPrograms;

    bool                       m_overwrite{true};
    bool                       m_rename{true};
    bool                       m_changeVariation{false};
    bool                       m_changeBanks{false};
    bool                       m_changePrograms{false};
    bool                       m_changeControls{false};
    bool                       m_changeKeyMappings{false};

};


}

#endif
