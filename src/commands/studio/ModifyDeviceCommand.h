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

#include "base/Device.h"  // DeviceId
#include "base/MidiDevice.h"
#include "document/Command.h"  // NamedCommand

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

    void setRename(bool rename)  { m_rename = rename; }
    void setVariation(MidiDevice::VariationType variationType);
    void setBankSelectType(MidiDevice::BankSelectType bankSelectType);
    void setBankList(const BankList &bankList);
    void setProgramList(const ProgramList &programList);
    void setControlList(const ControlList &controlList);
    void setKeyMappingList(const KeyMappingList &keyMappingList);

    /// Overwrite or merge bank list, program list, and key mappings.
    void setOverwrite(bool overwrite)  { m_overwrite = overwrite; }

    static QString getGlobalName()  { return tr("Modify &MIDI Bank"); }

    void execute() override;
    void unexecute() override;

private:

    Studio *m_studio;
    DeviceId m_deviceID;

    // Name
    std::string m_oldName;
    bool m_rename{true};
    std::string m_deviceName;

    // Librarian Name
    std::string m_oldLibrarianName;
    std::string m_librarianName;

    // Librarian email
    std::string m_oldLibrarianEmail;
    std::string m_librarianEmail;

    // Variation Type
    MidiDevice::VariationType m_oldVariationType{MidiDevice::NoVariations};
    bool m_changeVariation{false};
    MidiDevice::VariationType m_variationType{MidiDevice::NoVariations};

    // Bank Select Type
    MidiDevice::BankSelectType m_oldBankSelectType{
            MidiDevice::BankSelectType::Normal};
    bool m_changeBankSelectType{false};
    MidiDevice::BankSelectType m_bankSelectType{
            MidiDevice::BankSelectType::Normal};

    // Bank List
    BankList m_oldBankList;
    bool m_changeBanks{false};
    BankList m_bankList;

    // Program List
    ProgramList m_oldProgramList;
    bool m_changePrograms{false};
    ProgramList m_programList;

    /// Programs for each Instrument.  These are always saved and restored.
    /**
     * We preserve these to handle the case where the user changes a device
     * then changes the programs on the instruments, then decides to undo.
     * Instead of showing the modified programs in the old device, the
     * programs that were selected before are restored.  It's a partial
     * undo for Instrument program selections.
     */
    ProgramList m_oldInstrumentPrograms;

    // Controller List
    ControlList m_oldControlList;
    bool m_changeControls{false};
    ControlList m_controlList;

    // Key Mapping List
    KeyMappingList m_oldKeyMappingList;
    bool m_changeKeyMappings{false};
    KeyMappingList m_keyMappingList;

    bool m_overwrite{true};

};


}

#endif
