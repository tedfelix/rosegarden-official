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


#ifndef _DEVICEMANAGER_H_
#define _DEVICEMANAGER_H_

#include <kmainwindow.h>

#include "MidiDevice.h"
#include "Studio.h"

#include <set>

class RosegardenGUIDoc;
class QTable;
class QPushButton;


class DeviceManagerDialog : public KMainWindow
{
    Q_OBJECT
public:
    DeviceManagerDialog(QWidget *parent, RosegardenGUIDoc *document);
    ~DeviceManagerDialog();

    void setModified(bool value);

signals:
    void deviceNamesChanged();

    void editBanks(Rosegarden::DeviceId);
    void editControllers(Rosegarden::DeviceId);

    void closing();

protected slots:
    void slotClose();
    void slotAddPlayDevice();
    void slotAddRecordDevice();
    void slotDeletePlayDevice();
    void slotDeleteRecordDevice();
    void slotPlayValueChanged(int row, int col);
    void slotRecordValueChanged(int row, int col);
    void slotPlayDeviceSelected(int row, int col);
    void slotRecordDeviceSelected(int row, int col);

    // for play devices only:
    void slotImport();
    void slotExport();
    void slotSetBanks();
    void slotSetControllers();

    void slotDevicesResyncd();
    void populate();

protected:
    virtual void closeEvent(QCloseEvent *);

private:
    RosegardenGUIDoc *m_document;
    Rosegarden::Studio *m_studio;

    QPushButton *m_deletePlayButton;
    QPushButton *m_deleteRecordButton;
    QPushButton *m_importButton;
    QPushButton *m_exportButton;
    QPushButton *m_banksButton;
    QPushButton *m_controllersButton;

    QStringList m_playConnections;
    QStringList m_recordConnections;
    void makeConnectionList(unsigned int direction, QStringList &list);
    
    QTable *m_playTable;
    QTable *m_recordTable;

    typedef std::vector<Rosegarden::MidiDevice *> MidiDeviceList;
    MidiDeviceList m_playDevices;
    MidiDeviceList m_recordDevices;

    Rosegarden::DeviceId getPlayDeviceIdAt(int row); // NO_DEVICE = not found
    Rosegarden::DeviceId getRecordDeviceIdAt(int row); // NO_DEVICE = not found

    static const char* const DeviceManagerConfigGroup;
};

#endif
