/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#include "ChangeRecordDeviceCommand.h"
#include "StudioControl.h"
#include "document/ConfigGroups.h"
#include "sound/MappedEvent.h"
#include <kapplication.h>
#include <kconfig.h>

namespace Rosegarden
{

void
ChangeRecordDeviceCommand::swap()
 {

        KConfig *config = kapp->config();
        config->setGroup(Rosegarden::SequencerOptionsConfigGroup);
        QStringList devList = config->readListEntry("midirecorddevice");
        QString sdevice = QString::number(m_deviceId);
        if (m_action) 
        {
            if(!devList.contains(sdevice))
                devList.append(sdevice);
        }
        else
        {
            if(devList.contains(sdevice))
                devList.remove(sdevice);
        }
        config->writeEntry("midirecorddevice", devList);

        // send the selected device to the sequencer
        Rosegarden::MappedEvent mEdevice
            (Rosegarden::MidiInstrumentBase, 
             Rosegarden::MappedEvent::SystemRecordDevice,
             Rosegarden::MidiByte(m_deviceId),
             Rosegarden::MidiByte(m_action));
        Rosegarden::StudioControl::sendMappedEvent(mEdevice);

        m_action = !m_action;
}

}
