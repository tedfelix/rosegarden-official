
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MIDIBANKLISTVIEWITEM_H
#define RG_MIDIBANKLISTVIEWITEM_H

#include "base/Device.h"
#include "MidiDeviceTreeWidgetItem.h"
#include <QString>

#include <QCoreApplication>

//class QListWidgetItem;
class QTreeWidgetItem;

namespace Rosegarden
{



class MidiBankTreeWidgetItem : public MidiDeviceTreeWidgetItem
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::MidiBankTreeWidgetItem)

public:
    MidiBankTreeWidgetItem(DeviceId deviceId,
                           int bankNb,
                           QTreeWidgetItem* parent, QString name,
                           bool percussion,
                           int msb, int lsb);

    int getBank()     { return m_bankNb; }

    void setPercussion(bool percussion);
    bool isPercussion() const { return m_percussion; }
    void setMSB(int msb);
    void setLSB(int msb);

    virtual int compare(QTreeWidgetItem *i, int col, bool ascending) const;
    
protected:

    //--------------- Data members ---------------------------------
    bool   m_percussion;
    int    m_bankNb;
};


}

#endif
