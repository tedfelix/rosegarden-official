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

#ifndef RG_MIDIDEVICELISTVIEWITEM_H
#define RG_MIDIDEVICELISTVIEWITEM_H

#include "base/Device.h"

#include <QTreeWidget>
#include <QString>
#include <QCoreApplication>

namespace Rosegarden
{


class MidiDeviceTreeWidgetItem : public QTreeWidgetItem
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::MidiDeviceTreeWidgetItem)

public:
    // Device
    MidiDeviceTreeWidgetItem(DeviceId id,
                           QTreeWidget* parent, QString name);

    // Bank
    MidiDeviceTreeWidgetItem(DeviceId id,
                           QTreeWidgetItem* parent, QString name,
                           bool percussion,
                           int msb, int lsb);

    // Key Mapping
    MidiDeviceTreeWidgetItem(DeviceId id,
                           QTreeWidgetItem* parent, QString name);

    DeviceId getDeviceId() const { return m_deviceId; }

    virtual int compare(QTreeWidgetItem *i, int col, bool ascending) const;

protected:

    //--------------- Data members ---------------------------------
    DeviceId m_deviceId;
};


}

#endif
