/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/



#include "MidiKeyMapTreeWidgetItem.h"

#include "MidiDeviceTreeWidgetItem.h"
#include "MidiBankTreeWidgetItem.h"
#include "base/Device.h"

#include <QString>
#include <QTreeWidget>
#include <QTreeWidgetItem>



namespace Rosegarden
{

MidiKeyMapTreeWidgetItem::MidiKeyMapTreeWidgetItem(DeviceId deviceId,
        QTreeWidgetItem* parent,
        QString name)
        : MidiDeviceTreeWidgetItem(deviceId, parent, name),
        m_name(name)
{
    setFlags(flags() | Qt::ItemIsEditable);  //qt4
}

int MidiKeyMapTreeWidgetItem::compare(QTreeWidgetItem *i, int col, bool ascending) const
{
    if (dynamic_cast<MidiBankTreeWidgetItem *>(i)) {
        return 1; // banks before key maps
    }

    return MidiDeviceTreeWidgetItem::compare(i, col, ascending);
}

}
