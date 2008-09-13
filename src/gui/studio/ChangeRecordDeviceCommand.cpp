/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

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
#include <QApplication>
#include <QSettings>

namespace Rosegarden
{

void
ChangeRecordDeviceCommand::swap()
{

    QSettings settings;
    settings.beginGroup( Rosegarden::SequencerOptionsConfigGroup );

    QStringList devList = settings.value("midirecorddevice").toStringList();
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
    settings.setValue("midirecorddevice", devList);

    // send the selected device to the sequencer
    Rosegarden::MappedEvent mEdevice
        (Rosegarden::MidiInstrumentBase, 
         Rosegarden::MappedEvent::SystemRecordDevice,
         Rosegarden::MidiByte(m_deviceId),
         Rosegarden::MidiByte(m_action));
    Rosegarden::StudioControl::sendMappedEvent(mEdevice);

    m_action = !m_action;
    settings.endGroup();
}

}
