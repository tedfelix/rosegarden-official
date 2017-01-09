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


#include "MidiDeviceTreeWidgetItem.h"

#include "base/Device.h"
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QString>


namespace Rosegarden
{

MidiDeviceTreeWidgetItem::MidiDeviceTreeWidgetItem(DeviceId deviceId,
        QTreeWidget* parent, QString name)
        : QTreeWidgetItem(parent),
        m_deviceId(deviceId)
{
    setText( 0, name );
}

MidiDeviceTreeWidgetItem::MidiDeviceTreeWidgetItem(DeviceId deviceId,
        QTreeWidgetItem* parent, QString name,
        bool percussion,
        int msb, int lsb)
        : QTreeWidgetItem(parent, QStringList()
                                 << name
                                 << QString(percussion ? tr("Percussion Bank") : tr("Bank"))
                                 << QString().setNum(msb)
                                 << QString().setNum(lsb)),
          m_deviceId(deviceId)
{
}

MidiDeviceTreeWidgetItem::MidiDeviceTreeWidgetItem(DeviceId deviceId,
        QTreeWidgetItem* parent, QString name)
    : QTreeWidgetItem(parent, //name, 
                        QStringList() << name << tr("Key Mapping") << "" << ""),
      m_deviceId(deviceId)
{
}

int MidiDeviceTreeWidgetItem::compare(QTreeWidgetItem *i, int col, bool ascending) const
{
    MidiDeviceTreeWidgetItem* item = dynamic_cast<MidiDeviceTreeWidgetItem*>(i);
    if (!item){
            return 1;
//         return QTreeWidgetItem::compare(i, col, ascending);        //### //@@@ FIX : compare function
    }
    if (col == 0)
        return
            getDeviceId() > item->getDeviceId() ? 1 :
            getDeviceId() == item->getDeviceId() ? 0 :
            -1;

    int thisVal = text(col).toInt(),
                  otherVal = item->text(col).toInt();

    if (thisVal == otherVal) {
        if (col == 2) { // if sorting on MSB, suborder with LSB
            return compare(i, 3, ascending);
        } else {
            return 0;
        }
    }

    // 'ascending' should be ignored according to Qt docs
    //
    return (thisVal > otherVal) ? 1 : -1;

    //!!! how to use percussion here?
}

}
