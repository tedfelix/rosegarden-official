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

#define RG_MODULE_STRING "[ReconnectDeviceCommand]"

#include "ReconnectDeviceCommand.h"

#include "misc/Debug.h"
#include "base/Device.h"
#include "base/MidiDevice.h"
#include "gui/application/RosegardenMainWindow.h"
#include "sequencer/RosegardenSequencer.h"
#include "misc/Strings.h"  // For strtoqstr()...
#include "base/Studio.h"


namespace Rosegarden
{


void
ReconnectDeviceCommand::execute()
{
    Device *device = m_studio->getDevice(m_deviceId);
    MidiDevice *midiDevice = dynamic_cast<MidiDevice *>(device);

    if (!midiDevice)
        return;

    RosegardenSequencer *sequencer = RosegardenSequencer::getInstance();

    if (!sequencer)
        return;

    // Preserve the old values for undo.
    m_oldUserConnection = midiDevice->getUserConnection();
    // We don't trust MidiDevice::m_currentConnection as that may
    // not be in sync.  Get the actual connection direct from the source.
    m_oldConnection = qstrtostr(
            sequencer->getConnection(midiDevice->getId()));

    // Make the actual connection.
    sequencer->setConnection(m_deviceId, strtoqstr(m_newConnection));

    // Update the MidiDevice
    midiDevice->setUserConnection(m_newConnection);
    midiDevice->setCurrentConnection(m_newConnection);
    midiDevice->sendChannelSetups();

    //RG_DEBUG << "execute(): reconnected device " << m_deviceId << " to " << m_newConnection;

    // ??? Instead of this kludge, we should be calling a Studio::hasChanged()
    //     which would then notify all observers (e.g. MIPP) who, in turn,
    //     would update themselves.
    RosegardenMainWindow::self()->uiUpdateKludge();
}

void
ReconnectDeviceCommand::unexecute()
{
    Device *device = m_studio->getDevice(m_deviceId);
    MidiDevice *midiDevice = dynamic_cast<MidiDevice *>(device);

    if (!midiDevice)
        return;

    // Make the actual connection.
    RosegardenSequencer::getInstance()->setConnection(
            m_deviceId, strtoqstr(m_oldConnection));

    // Update the MidiDevice
    midiDevice->setUserConnection(m_oldUserConnection);
    midiDevice->setCurrentConnection(m_oldConnection);
    midiDevice->sendChannelSetups();

    //RG_DEBUG << "unexecute(): reconnected device " << m_deviceId << " to " << m_oldConnection;

    // ??? Instead of this kludge, we should be calling a Studio::hasChanged()
    //     which would then notify all observers (e.g. MIPP) who, in turn,
    //     would update themselves.
    RosegardenMainWindow::self()->uiUpdateKludge();
}


}
