
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

#ifndef RG_MIDIKEYMAPLISTVIEWITEM_H
#define RG_MIDIKEYMAPLISTVIEWITEM_H

#include "base/Device.h"
#include "MidiDeviceTreeWidgetItem.h"
#include <QString>


class QTreeWidgetItem;


namespace Rosegarden
{



class MidiKeyMapTreeWidgetItem : public MidiDeviceTreeWidgetItem
{
    //Q_OBJECT
public:
    MidiKeyMapTreeWidgetItem(DeviceId deviceId,
                           QTreeWidgetItem* parent, QString name);

    virtual int compare(QTreeWidgetItem *i, int col, bool ascending) const;

    QString getName() const { return m_name; }

protected:
    QString m_name;
};


}

#endif
