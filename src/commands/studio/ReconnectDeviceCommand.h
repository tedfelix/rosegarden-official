
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_RECONNECTDEVICECOMMAND_H
#define RG_RECONNECTDEVICECOMMAND_H

#include "document/Command.h"  // For NamedCommand
#include "base/Device.h"  // For DeviceId

#include <QString>

#include <string>


namespace Rosegarden
{


class Studio;


class ReconnectDeviceCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::ReconnectDeviceCommand)

public:
    ReconnectDeviceCommand(Studio *studio,
                           DeviceId deviceId,
                           std::string newConnection) :
        NamedCommand(getGlobalName()),
        m_studio(studio),
        m_deviceId(deviceId),
        m_newConnection(newConnection) { }

    static QString getGlobalName()  { return tr("Reconnect Device"); }

    void execute() override;
    void unexecute() override;

protected:
    Studio *m_studio;
    DeviceId m_deviceId;
    std::string m_newConnection;
    std::string m_oldUserConnection;
    std::string m_oldConnection;
};


}

#endif
