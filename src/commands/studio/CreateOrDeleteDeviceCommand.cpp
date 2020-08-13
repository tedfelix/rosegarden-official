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

#define RG_MODULE_STRING "[CreateOrDeleteDeviceCommand]"

#include "CreateOrDeleteDeviceCommand.h"

#include "misc/Debug.h"
#include "base/Device.h"
#include "gui/studio/DeviceManagerDialog.h"
#include "base/MidiDevice.h"
#include "gui/application/RosegardenMainWindow.h"
#include "sequencer/RosegardenSequencer.h"
#include "misc/Strings.h"  // For qstrtostr()...
#include "base/Studio.h"


namespace Rosegarden
{

CreateOrDeleteDeviceCommand::CreateOrDeleteDeviceCommand(Studio *studio,
                                                         DeviceId id) :
    NamedCommand(getGlobalName(true)),
    m_studio(studio),
    m_deviceId(id),
    m_deviceCreated(true)  // We are doing delete.
{
    Device *device = m_studio->getDevice(m_deviceId);

    if (!device) {
        RG_WARNING << "CreateOrDeleteDeviceCommand(): WARNING: No such device as " << m_deviceId;
        return;
    }

    // Save for undo.

    m_name = device->getName();
    m_type = device->getType();
    m_direction = MidiDevice::Play;

    MidiDevice *midiDevice = dynamic_cast<MidiDevice *>(device);
    if (midiDevice)
        m_direction = midiDevice->getDirection();

    m_connection = qstrtostr(RosegardenSequencer::getInstance()->
            getConnection(midiDevice->getId()));
}

void
CreateOrDeleteDeviceCommand::execute()
{
    // Create
    if (!m_deviceCreated) {

        // !DEVPUSH: Not ideal; we probably just want to add it to the
        //           studio (and then trigger a re-push) rather than add
        //           it twice to studio and sequencer

        // don't want to do this again on undo even if it fails -- only on redo
        m_deviceCreated = true;

        m_deviceId = m_studio->getSpareDeviceId(m_baseInstrumentId);

        bool success = RosegardenSequencer::getInstance()->
            addDevice(m_type, m_deviceId, m_baseInstrumentId, m_direction);

        if (!success) {
            RG_WARNING << "execute(): WARNING: addDevice() failed";
            return;
        }

        //RG_DEBUG << "execute() - added device " << m_deviceId << " with base instrument id " << m_baseInstrumentId;

        // Make the connection.
        RosegardenSequencer::getInstance()->setConnection(
                m_deviceId, strtoqstr(m_connection));

        //RG_DEBUG << "execute() - reconnected device " << m_deviceId << " to " << m_connection;

        // Add to Studio.
        m_studio->addDevice(m_name, m_deviceId, m_baseInstrumentId, m_type);

        Device *device = m_studio->getDevice(m_deviceId);
        if (device) {
            MidiDevice *midiDevice = dynamic_cast<MidiDevice *>(device);
            if (midiDevice) {
                midiDevice->setDirection(m_direction);
                midiDevice->setUserConnection(m_connection);
                midiDevice->setCurrentConnection(m_connection);
            }
        }

        // Update view automatically (without pressing refresh button).
        DeviceManagerDialog *deviceManagerDialog =
                RosegardenMainWindow::self()->getDeviceManager();
        if (deviceManagerDialog)
            deviceManagerDialog->slotResyncDevicesReceived();

    } else {  // Delete

        // Delete the device from the sequencer.
        RosegardenSequencer::getInstance()->removeDevice(m_deviceId);

        //RG_DEBUG << "execute() - removed device " << m_deviceId;

        // Delete the device from the Studio.
        m_studio->removeDevice(m_deviceId);

        m_deviceId = Device::NO_DEVICE;

        m_deviceCreated = false;
    }

    // ??? Instead of this kludge, we should be calling a Studio::hasChanged()
    //     which would then notify all observers (e.g. MIPP) who, in turn,
    //     would update themselves.
    RosegardenMainWindow::self()->uiUpdateKludge();
}


}
