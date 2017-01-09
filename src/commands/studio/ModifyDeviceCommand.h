
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

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
#include <string>
#include "document/Command.h"
#include <QString>
#include <QCoreApplication>


class Modify;


namespace Rosegarden
{

class Studio;


class ModifyDeviceCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::ModifyDeviceCommand)

public:
    // Any of the arguments passed by pointer may be null (except for
    // the Studio) -- in which case they will not be changed in the device.
    ModifyDeviceCommand(Studio *studio,
                        DeviceId device,
                        const std::string &name,
                        const std::string &librarianName,
                        const std::string &librarianEmail);
    
    void setVariation  (MidiDevice::VariationType variationType);
    void setBankList   (const BankList    &bankList);
    void setProgramList(const ProgramList &programList);
    void setControlList(const ControlList &controlList);
    void setKeyMappingList(const KeyMappingList &keyMappingList);
    void setOverwrite  (bool overwrite) { m_overwrite = overwrite; }
    void setRename     (bool rename)    { m_rename = rename; }

    /// supersedes setBankList() and setProgramList()
    void clearBankAndProgramList() { m_clearBankAndProgramList = true; }

    static QString getGlobalName() { return tr("Modify &MIDI Bank"); }

    virtual void execute();
    virtual void unexecute();

protected:

    Studio                    *m_studio;
    int                        m_device;

    std::string                m_name;
    std::string                m_librarianName;
    std::string                m_librarianEmail;
    MidiDevice::VariationType  m_variationType;
    BankList                   m_bankList;
    ProgramList                m_programList;
    ControlList                m_controlList;
    KeyMappingList             m_keyMappingList;

    std::string                m_oldName;
    std::string                m_oldLibrarianName;
    std::string                m_oldLibrarianEmail;
    MidiDevice::VariationType  m_oldVariationType;
    BankList                   m_oldBankList;
    ProgramList                m_oldProgramList;
    ControlList                m_oldControlList;
    KeyMappingList             m_oldKeyMappingList;
    ProgramList                m_oldInstrumentPrograms;

    bool                       m_overwrite;
    bool                       m_rename;
    bool                       m_changeVariation;
    bool                       m_changeBanks;
    bool                       m_changePrograms;
    bool                       m_changeControls;
    bool                       m_changeKeyMappings;
    bool                       m_clearBankAndProgramList;

};


}

#endif
