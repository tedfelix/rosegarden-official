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

#ifndef _RG_DEVICEEDITORDIALOG_H_
#define _RG_DEVICEEDITORDIALOG_H_

#include "base/Studio.h"
#include "base/MidiDevice.h"
#include <kdialogbase.h>
#include <qstringlist.h>
#include <set>

class QWidget;
class QTable;


namespace Rosegarden
{

class Studio;
class RosegardenGUIDoc;


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
    Studio *m_studio;

    QStringList m_playConnections;
    QStringList m_recordConnections;
    void makeConnectionList(MidiDevice::DeviceDirection direction, 
			    QStringList &list);
    
    QTable *m_table;

    DeviceList m_devices;
    std::set<DeviceId> m_deletedDevices;

    void populate();
    int getDeviceIdAt(int row); // -1 for new device without an id yet
    
    bool m_modified;
};


}

#endif
