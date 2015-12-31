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


#include "RenameDeviceCommand.h"

#include "misc/Strings.h"
#include "base/Device.h"
#include "base/Studio.h"
#include <QString>
#include "sequencer/RosegardenSequencer.h"
#include "gui/application/RosegardenMainWindow.h"


namespace Rosegarden
{

void
RenameDeviceCommand::execute()
{
    Device *device = m_studio->getDevice(m_deviceId);
    if (!device) return;
    if (m_oldName == "") m_oldName = device->getName();
    RosegardenSequencer::getInstance()->renameDevice
        (m_deviceId, strtoqstr(m_name));
    device->setName(m_name);
    // ??? Instead of this kludge, we should be calling a Studio::hasChanged()
    //     which would then notify all observers (e.g. MIPP) who, in turn,
    //     would update themselves.
    RosegardenMainWindow::self()->uiUpdateKludge();
}

void
RenameDeviceCommand::unexecute()
{
    Device *device = m_studio->getDevice(m_deviceId);
    if (!device) return;
    device->setName(m_oldName);
    RosegardenSequencer::getInstance()->renameDevice
        (m_deviceId, strtoqstr(m_oldName));
    // ??? Instead of this kludge, we should be calling a Studio::hasChanged()
    //     which would then notify all observers (e.g. MIPP) who, in turn,
    //     would update themselves.
    RosegardenMainWindow::self()->uiUpdateKludge();
}

}
