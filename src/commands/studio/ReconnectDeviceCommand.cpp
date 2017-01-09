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


#define RG_MODULE_STRING "[ReconnectDeviceCommand]"

#include "ReconnectDeviceCommand.h"

#include "misc/Strings.h"
#include "misc/Debug.h"
#include "base/Device.h"
#include "base/Studio.h"
#include <QByteArray>
#include <QDataStream>
#include <QString>
#include "sequencer/RosegardenSequencer.h"
#include "gui/application/RosegardenMainWindow.h"


namespace Rosegarden
{


void
ReconnectDeviceCommand::execute()
{
    Device *device = m_studio->getDevice(m_deviceId);

    if (device) {
        RosegardenSequencer *sequencer = RosegardenSequencer::getInstance();

        m_oldConnection = qstrtostr(
                sequencer->getConnection(device->getId()));

        sequencer->setConnection(m_deviceId, strtoqstr(m_newConnection));
        device->setConnection(m_newConnection);
        device->sendChannelSetups();

        //RG_DEBUG << "execute(): reconnected device " << m_deviceId << " to " << m_newConnection;
    }

    // ??? Instead of this kludge, we should be calling a Studio::hasChanged()
    //     which would then notify all observers (e.g. MIPP) who, in turn,
    //     would update themselves.
    RosegardenMainWindow::self()->uiUpdateKludge();
}

void
ReconnectDeviceCommand::unexecute()
{
    Device *device = m_studio->getDevice(m_deviceId);

    if (device) {
        RosegardenSequencer::getInstance()->setConnection(
                m_deviceId, strtoqstr(m_oldConnection));
        device->setConnection(m_oldConnection);
        device->sendChannelSetups();

        //RG_DEBUG << "unexecute(): reconnected device " << m_deviceId << " to " << m_oldConnection;
    }

    // ??? Instead of this kludge, we should be calling a Studio::hasChanged()
    //     which would then notify all observers (e.g. MIPP) who, in turn,
    //     would update themselves.
    RosegardenMainWindow::self()->uiUpdateKludge();
}


}
