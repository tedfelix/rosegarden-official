/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "CreateOrDeleteDeviceCommand.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Device.h"
#include "base/MidiDevice.h"
#include "base/Studio.h"
#include <qcstring.h>
#include <qdatastream.h>
#include <qstring.h>
#include "gui/application/RosegardenApplication.h"


namespace Rosegarden
{

CreateOrDeleteDeviceCommand::CreateOrDeleteDeviceCommand(Studio *studio,
        DeviceId id) :
        KNamedCommand(getGlobalName(true)),
        m_studio(studio),
        m_deviceId(id),
        m_deviceCreated(true)
{
    Device *device = m_studio->getDevice(m_deviceId);

    if (device) {
        m_name = device->getName();
        m_type = device->getType();
        m_direction = MidiDevice::Play;
        MidiDevice *md =
            dynamic_cast<MidiDevice *>(device);
        if (md)
            m_direction = md->getDirection();
        m_connection = device->getConnection();
    } else {
        RG_DEBUG << "CreateOrDeleteDeviceCommand: No such device as "
        << m_deviceId << endl;
    }
}

void
CreateOrDeleteDeviceCommand::execute()
{
    if (!m_deviceCreated) {

        // Create

        // don't want to do this again on undo even if it fails -- only on redo
        m_deviceCreated = true;


        QByteArray data;
        QByteArray replyData;
        QCString replyType;
        QDataStream arg(data, IO_WriteOnly);

        arg << (int)m_type;
        arg << (unsigned int)m_direction;

        if (!rgapp->sequencerCall("addDevice(int, unsigned int)",
                                  replyType, replyData, data)) {
            SEQMAN_DEBUG << "CreateDeviceCommand::execute - "
            << "failure in sequencer addDevice" << endl;
            return ;
        }

        QDataStream reply(replyData, IO_ReadOnly);
        reply >> m_deviceId;

        if (m_deviceId == Device::NO_DEVICE) {
            SEQMAN_DEBUG << "CreateDeviceCommand::execute - "
            << "sequencer addDevice failed" << endl;
            return ;
        }

        SEQMAN_DEBUG << "CreateDeviceCommand::execute - "
        << " added device " << m_deviceId << endl;

        arg.device()->reset();
        arg << (unsigned int)m_deviceId;
        arg << strtoqstr(m_connection);

        if (!rgapp->sequencerCall("setConnection(unsigned int, QString)",
                                  replyType, replyData, data)) {
            SEQMAN_DEBUG << "CreateDeviceCommand::execute - "
            << "failure in sequencer setConnection" << endl;
            return ;
        }

        SEQMAN_DEBUG << "CreateDeviceCommand::execute - "
        << " reconnected device " << m_deviceId
        << " to " << m_connection << endl;

        // Add the device to the Studio now, so that we can name it --
        // otherwise the name will be lost
        m_studio->addDevice(m_name, m_deviceId, m_type);
        Device *device = m_studio->getDevice(m_deviceId);
        if (device) {
            MidiDevice *md = dynamic_cast<MidiDevice *>
                             (device);
            if (md)
                md->setDirection(m_direction);
        }

    } else {

        // Delete

        QByteArray data;
        QByteArray replyData;
        QCString replyType;
        QDataStream arg(data, IO_WriteOnly);

        if (m_deviceId == Device::NO_DEVICE)
            return ;

        arg << (int)m_deviceId;

        if (!rgapp->sequencerCall("removeDevice(unsigned int)",
                                  replyType, replyData, data)) {
            SEQMAN_DEBUG << "CreateDeviceCommand::execute - "
            << "failure in sequencer addDevice" << endl;
            return ;
        }

        SEQMAN_DEBUG << "CreateDeviceCommand::unexecute - "
        << " removed device " << m_deviceId << endl;

        m_studio->removeDevice(m_deviceId);

        m_deviceId = Device::NO_DEVICE;
        m_deviceCreated = false;
    }
}

}
