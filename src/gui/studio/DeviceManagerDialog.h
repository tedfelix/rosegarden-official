/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#ifndef DEVICESMANAGERNEW_H
#define DEVICESMANAGERNEW_H

#include "ui_DeviceManagerDialogUi.h"

#include "base/Device.h"
#include "base/MidiDevice.h"
#include "base/MidiTypes.h"
#include "base/Studio.h"

#include <QWidget>
#include <QDialog>
#include <QObject>

#include <set>

namespace Rosegarden
{


typedef std::vector<MidiDevice *> MidiDeviceList;

class RosegardenDocument;

/// The Manage MIDI Devices dialog
/**
 * \author Emanuel Rumpf
 */
class DeviceManagerDialog : public QMainWindow, public Ui::DeviceManagerDialogUi, public StudioObserver, public DeviceObserver
{
    Q_OBJECT

public:

    explicit DeviceManagerDialog (QWidget* parent);
    ~DeviceManagerDialog() override;

    /**
    *    Clear all lists
    */
    void clearAllPortsLists();

    /**
    *    make Slot connections
    */
    void connectSignalsToSlots();

    // unused MidiDevice* getDeviceByName(QString deviceName);
    MidiDevice* getDeviceById(DeviceId devId);

    MidiDevice* getMidiDeviceOfItem(QTreeWidgetItem* twItem);
    MidiDevice* getCurrentlySelectedDevice(QTreeWidget* treeWid);

    void connectMidiDeviceToPort (MidiDevice* device, QString newPort);

    /**
    *    If the selected device has changed, this
    *    marks (checks) the associated list entry in the ports list (connection)
    */
    void updateCheckStatesOfPortsList(QTreeWidget* treeWid_ports, QTreeWidget* treeWid_devices);

    /**
    *    adds/removes list entries in the visible devices-list (treeWid),
    *    if the (invisible) device-list of the sequencer has changed
    */
    void updateDevicesList(QTreeWidget* treeWid,
                            MidiDevice::DeviceDirection in_out_direction);

    /**
    *    search treeWid for the item associated with devId
    */
    QTreeWidgetItem* searchItemWithDeviceId(QTreeWidget* treeWid, DeviceId devId);

    QTreeWidgetItem* searchItemWithPort(QTreeWidget* treeWid, QString portName);

    /**
    *    add/remove list entries in the visible ports-list (connections),
    *    if the (invisible) connections of the sequencer/studio have changed.
    */
    void updatePortsList(QTreeWidget* treeWid, MidiDevice::DeviceDirection PlayRecDir);


signals:
    //void deviceNamesChanged();

    void editBanks (DeviceId);
    void editControllers (DeviceId);

    void deviceNameChanged(DeviceId);
    void deviceNamesChanged();

public slots:
    void slotOutputPortClicked(QTreeWidgetItem * twItem, int column);
    void slotPlaybackDeviceSelected();

    void slotInputPortClicked(QTreeWidgetItem * twItem, int column);
    void slotRecordDeviceSelected();

    void slotDeviceItemChanged (QTreeWidgetItem * twItem, int column);

    /**
       Force all double clicks from playback and record device to edit only
       device column.
     */
    void slotEdit(QTreeWidgetItem *item, int);

    void slotRefreshOutputPorts();
    void slotRefreshInputPorts();

    void slotAddPlaybackDevice();
    void slotAddRecordDevice();

    void slotDeletePlaybackDevice();
    void slotDeleteRecordDevice();

    void slotManageBanksOfPlaybackDevice();
    void slotEditControllerDefinitions();

    void show();
    void slotHelpRequested();

    void slotResyncDevicesReceived();

protected:
    Studio *m_studio;

    /**
    *    used to store the device ID in the QTreeWidgetItem
    *    of the visible device list (QTreeWidget)
    */
    int m_UserRole_DeviceId; // = Qt::UserRole + 1;

    QString m_noPortName;

 public slots:
    void slotCloseButtonPress();

 private:
    // studio observer interface
    virtual void deviceAdded(Device* device) override;
    virtual void deviceRemoved(Device* device) override;

    // device observer interface
    virtual void deviceModified(Device* device) override;

    void observeDevice(Device* device);
    void unobserveAllDevices();

    std::set<Device *> m_observedDevices;
    bool m_observingStudio;
    bool m_isClosing;

};


} // end namespace Rosegarden

#endif // DEVICESMANAGERNEW_H
