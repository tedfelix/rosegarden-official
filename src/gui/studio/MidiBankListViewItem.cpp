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


#include "MidiBankListViewItem.h"

#include <klocale.h>
#include "base/Device.h"
#include "MidiDeviceListViewItem.h"
#include "MidiKeyMapListViewItem.h"
#include <qstring.h>


namespace Rosegarden
{

MidiBankListViewItem::MidiBankListViewItem(DeviceId deviceId,
        int bankNb,
        QListViewItem* parent,
        QString name,
        bool percussion, int msb, int lsb)
        : MidiDeviceListViewItem(deviceId, parent, name, percussion, msb, lsb),
        m_percussion(percussion),
        m_bankNb(bankNb)
{}

void MidiBankListViewItem::setPercussion(bool percussion)
{
    m_percussion = percussion;
    setText(1, QString(percussion ? i18n("Percussion Bank") : i18n("Bank")));
}

void MidiBankListViewItem::setMSB(int msb)
{
    setText(2, QString().setNum(msb));
}

void MidiBankListViewItem::setLSB(int lsb)
{
    setText(3, QString().setNum(lsb));
}

int MidiBankListViewItem::compare(QListViewItem *i, int col, bool ascending) const
{
    MidiBankListViewItem* bankItem = dynamic_cast<MidiBankListViewItem*>(i);

    if (!bankItem) {
        MidiKeyMapListViewItem *keyItem = dynamic_cast<MidiKeyMapListViewItem *>(i);
        if (keyItem)
            return -1; // banks before key maps
    }

    if (!bankItem || (col != 2 && col != 3)) {
        return MidiDeviceListViewItem::compare(i, col, ascending);
    }

    int thisVal = text(col).toInt(),
                  otherVal = bankItem->text(col).toInt();

    if (thisVal == otherVal) {
        if (col == 2) { // if sorting on MSB, suborder with LSB
            return compare(i, 3, ascending);
        } else {
            return 0;
        }
    }

    // 'ascending' should be ignored according to Qt docs
    //
    return
        thisVal > otherVal ? 1 :
        thisVal == otherVal ? 0	:
        -1;

}

}
