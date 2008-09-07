
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

#ifndef _RG_MIDIDEVICELISTVIEWITEM_H_
#define _RG_MIDIDEVICELISTVIEWITEM_H_

#include "base/Device.h"
#include <QListView>
#include <QString>


namespace Rosegarden
{



class MidiDeviceListViewItem : public QListViewItem
{
public:
    // Device
    MidiDeviceListViewItem(DeviceId id,
                           QListView* parent, QString name);

    // Bank
    MidiDeviceListViewItem(DeviceId id,
                           QListViewItem* parent, QString name,
                           bool percussion,
                           int msb, int lsb);

    // Key Mapping
    MidiDeviceListViewItem(DeviceId id,
                           QListViewItem* parent, QString name);

    DeviceId getDeviceId() const { return m_deviceId; }

    virtual int compare(QListViewItem *i, int col, bool ascending) const;

protected:

    //--------------- Data members ---------------------------------
    DeviceId m_deviceId;
};


}

#endif
