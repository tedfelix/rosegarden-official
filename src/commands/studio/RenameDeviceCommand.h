
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_RENAMEDEVICECOMMAND_H
#define RG_RENAMEDEVICECOMMAND_H

#include "base/Device.h"
#include <string>
#include "document/Command.h"
#include <QString>
#include <QCoreApplication>




namespace Rosegarden
{

class Studio;


class RenameDeviceCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::RenameDeviceCommand)

public:
    RenameDeviceCommand(Studio *studio,
                        DeviceId deviceId,
                        std::string name) :
        NamedCommand(getGlobalName()),
        m_studio(studio),
        m_deviceId(deviceId),
        m_name(name) { }

    static QString getGlobalName() { return tr("Rename Device"); }

    virtual void execute();
    virtual void unexecute();

protected:
    Studio *m_studio;
    DeviceId m_deviceId;
    std::string m_name;
    std::string m_oldName;
};



}

#endif
