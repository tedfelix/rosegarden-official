/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

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

#include <QTreeWidgetItem>
#include <QString>
#include <QCoreApplication>  // Q_DECLARE_TR_FUNCTIONS


namespace Rosegarden
{


class MidiDevice;


class MidiDeviceTreeWidgetItem : public QTreeWidgetItem
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::MidiDeviceTreeWidgetItem)

public:

    /// Construct an item and insert into parent.
    MidiDeviceTreeWidgetItem(
            QTreeWidget *parent, MidiDevice *device, const QString &name);

    /// Bank
    /**
     * ??? But a Bank is not a kind of Device.  This ctor does not
     *     belong here.
     */
    MidiDeviceTreeWidgetItem(MidiDevice* device,
                             QTreeWidgetItem* parent, QString name,
                             bool percussion,
                             int msb, int lsb);

    /// Key Mapping
    /**
     * ??? But a key map is not a kind of Device.  This ctor does not
     *     belong here.
     */
    MidiDeviceTreeWidgetItem(MidiDevice* device,
                             QTreeWidgetItem* parent, QString name);

    MidiDevice* getDevice() const { return m_device; }

    virtual int compare(QTreeWidgetItem *i, int col, bool ascending) const;

    QString getName() const { return m_name; }

protected:

    QString m_name;

private:

    MidiDevice *m_device;

};


}

#endif
