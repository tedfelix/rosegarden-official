// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#ifndef _DEVICEEDITOR_H_
#define _DEVICEEDITOR_H_

#include <kdialogbase.h>

#include "Device.h"
#include "Studio.h"

#include <set>

class RosegardenGUIDoc;
class QTable;


class DeviceEditorDialog : public KDialogBase
{
    Q_OBJECT
public:
    DeviceEditorDialog(QWidget *parent, RosegardenGUIDoc *document);
    ~DeviceEditorDialog();

    void setModified(bool value);

protected slots:
    void slotOk();
    void slotApply();
    void slotClose();

    void slotAddPlayDevice();
    void slotAddRecordDevice();
    void slotDeleteDevice();
    void slotValueChanged(int row, int col);

private:
    RosegardenGUIDoc *m_document;
    Rosegarden::Studio *m_studio;

    QStringList m_playConnections;
    QStringList m_recordConnections;
    void makeConnectionList(unsigned int direction, QStringList &list);
    
    QTable *m_table;

    Rosegarden::DeviceList m_devices;
    std::set<Rosegarden::DeviceId> m_deletedDevices;

    void populate();
    int getDeviceIdAt(int row); // -1 for new device without an id yet
    
    bool m_modified;
};

#endif
