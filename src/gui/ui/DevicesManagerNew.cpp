/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.

    Written February 2009 by Emanuel Rumpf.

 */


#include "gui/ui/DevicesManagerNew.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Device.h"
#include "base/Event.h"
#include "base/Instrument.h"
#include "base/MidiDevice.h"
#include "base/MidiTypes.h"
#include "base/Studio.h"
#include "commands/studio/CreateOrDeleteDeviceCommand.h"
#include "commands/studio/ModifyDeviceCommand.h"
#include "commands/studio/ReconnectDeviceCommand.h"
#include "commands/studio/RenameDeviceCommand.h"
#include "document/CommandHistory.h"
#include "document/RosegardenDocument.h"
#include "sequencer/RosegardenSequencer.h"
#include "gui/general/IconLoader.h"

#include <QWidget>
#include <QMainWindow>
#include <QObject>
#include <QFont>
#include <QMessageBox>


namespace Rosegarden
{


DevicesManagerNew::~DevicesManagerNew()
{
    // destructor
}


DevicesManagerNew::DevicesManagerNew(QWidget * parent,
                                     RosegardenDocument *
                                     doc) : QMainWindow(parent),
    Ui::DevicesManagerNewUi(), m_doc(doc)
{
    RG_DEBUG << "DevicesManagerNew::ctor" << endl;

    // start constructor
    setObjectName("DeviceManager");
    setWindowModality(Qt::NonModal);

    m_UserRole_DeviceId = Qt::UserRole + 1;
    m_noPortName = tr("[ No port ]");

    //m_doc = 0;    // RG document
    m_studio = &m_doc->getStudio();

    setupUi(this);

    // gui adjustments
    //
    // adjust some column widths for better visibility
    m_treeWidget_playbackDevices->setColumnWidth(0, 200);   // column, width
    m_treeWidget_recordDevices->setColumnWidth(0, 150);
    m_treeWidget_recordDevices->setColumnWidth(1, 50);
    m_treeWidget_recordDevices->setColumnWidth(3, 150);

    connectSignalsToSlots();

    clearAllPortsLists();
}

void
DevicesManagerNew::show()
{
    RG_DEBUG << "DevicesManagerNew::show()" << endl;

    slotRefreshOutputPorts();
    slotRefreshInputPorts();

    QMainWindow::show();
}


void
DevicesManagerNew::slotClose()
{
    RG_DEBUG << "DevicesManagerNew::slotClose()" << endl;
    /*
       if (m_doc) {
       //CommandHistory::getInstance()->detachView(actionCollection());    //&&&
       m_doc = 0;
       }
     */
    close(0);
}


void
DevicesManagerNew::slotRefreshOutputPorts()
{
    RG_DEBUG << "DevicesManagerNew::slotRefreshOutputPorts()" << endl;

    updatePortsList(m_treeWidget_outputPorts, MidiDevice::Play);

    updateDevicesList(0, m_treeWidget_playbackDevices,
                      MidiDevice::Play);

    updateCheckStatesOfPortsList(m_treeWidget_outputPorts,
                                 m_treeWidget_playbackDevices);
}


void
DevicesManagerNew::slotRefreshInputPorts()
{
    RG_DEBUG << "DevicesManagerNew::slotRefreshInputPorts()" << endl;

    updatePortsList(m_treeWidget_inputPorts, MidiDevice::Record);

    updateDevicesList(0, m_treeWidget_recordDevices,
                      MidiDevice::Record);

    updateCheckStatesOfPortsList(m_treeWidget_inputPorts,
                                 m_treeWidget_recordDevices);
}




void
DevicesManagerNew::slotPlaybackDeviceSelected()
{
    RG_DEBUG << "DevicesManagerNew::slotPlaybackDeviceSelected()" << endl;
    //
    updateCheckStatesOfPortsList(m_treeWidget_outputPorts,
                                 m_treeWidget_playbackDevices);

    // center selected item
    QTreeWidgetItem *twItemS;
    MidiDevice *mdev;
    mdev = getCurrentlySelectedDevice(m_treeWidget_playbackDevices);
    if (!mdev) {
        return;
    }
    twItemS =
            searchItemWithPort(m_treeWidget_outputPorts,
                               strtoqstr(mdev->getConnection()));
    if (twItemS) {
        m_treeWidget_outputPorts->scrollToItem(twItemS,
                                               QAbstractItemView::
                                               PositionAtCenter);
    }
}


void
DevicesManagerNew::slotRecordDeviceSelected()
{
    RG_DEBUG << "DevicesManagerNew::slotRecordDevicesSelected()" << endl;
    //
    updateCheckStatesOfPortsList(m_treeWidget_inputPorts,
                                 m_treeWidget_recordDevices);

    // center selected item
    QTreeWidgetItem *twItemS;
    MidiDevice *mdev;
    mdev = getCurrentlySelectedDevice(m_treeWidget_recordDevices);
    if (!mdev) {
        return;
    }
    twItemS =
            searchItemWithPort(m_treeWidget_inputPorts,
                               strtoqstr(mdev->getConnection()));
    if (twItemS) {
        m_treeWidget_inputPorts->scrollToItem(twItemS,
                                              QAbstractItemView::
                                              PositionAtCenter);
    }
}



void
DevicesManagerNew::slotOutputPortClicked(QTreeWidgetItem *twItem, int column)
{
    RG_DEBUG << "DevicesManagerNew::slotOutputPortClicked(...)" << endl;

    MidiDevice *mdev;
    QString portName;
    int OutputPortListColumn_Name = 0;

    portName = twItem->text(OutputPortListColumn_Name);

    RG_DEBUG << "DevicesManagerNew: portName: " << portName << endl;

    mdev = getCurrentlySelectedDevice(m_treeWidget_playbackDevices);
    if (!mdev) {
        return;
    }
    connectMidiDeviceToPort(mdev, portName);

    // update the playback-devices-list 
    updateDevicesList(0, m_treeWidget_playbackDevices,
                      MidiDevice::Play);

    updateCheckStatesOfPortsList(m_treeWidget_outputPorts,
                                 m_treeWidget_playbackDevices);
}


void
DevicesManagerNew::slotInputPortClicked(QTreeWidgetItem *
                                                   twItem, int column)
{
    RG_DEBUG << "DevicesManagerNew::slotInputPortClicked(...)" << endl;

    MidiDevice *mdev;
    QString portName;
    int InputPortListColumn_Name = 0;

    portName = twItem->text(InputPortListColumn_Name);
    mdev = getCurrentlySelectedDevice(m_treeWidget_recordDevices);
    if (!mdev) {
        return;
    }
    connectMidiDeviceToPort(mdev, portName);

    // center selected item
    QTreeWidgetItem *twItemS;
    twItemS = searchItemWithPort(m_treeWidget_inputPorts, portName);
    if (twItemS) {
        m_treeWidget_inputPorts->scrollToItem(twItemS,
                                              QAbstractItemView::
                                              PositionAtCenter);
    }

    // update the record-devices-list
    updateDevicesList(0, m_treeWidget_recordDevices,
                      MidiDevice::Record);
    updateCheckStatesOfPortsList(m_treeWidget_inputPorts,
                                 m_treeWidget_recordDevices);
}


QTreeWidgetItem
*DevicesManagerNew::searchItemWithPort(QTreeWidget *treeWid,
                                                    QString
                                                    portName)
{
    RG_DEBUG << "DevicesManagerNew::searchItemWithPort(...)" << endl;

    // find the TreeWidgetItem, that is associated with the port with portName
    // searches Items of treeWid
    int i, cnt;
    QTreeWidgetItem *twItem;
    QString portNameX;

    if (portName == "") {
        portName = m_noPortName;
    }

    cnt = treeWid->topLevelItemCount();
    for (i = 0; i < cnt; i++) {
        twItem = treeWid->topLevelItem(i);

        portNameX = twItem->text(0);
        if (portNameX == portName) {
            return twItem;
        }
    }
    return 0;               //twItem;
}


QTreeWidgetItem
*DevicesManagerNew::searchItemWithDeviceId(QTreeWidget *treeWid,
                                           DeviceId devId)
{
    RG_DEBUG << "DevicesManagerNew::searchItemWithDeviceId(..., devId = "
             << devId << ")" << endl;

    // find the TreeWidgetItem, that is associated with the device with devId
    // searches Items of treeWid
    int i, cnt;
    QTreeWidgetItem *twItem;
    DeviceId devIdx;

    cnt = treeWid->topLevelItemCount();
    for (i = 0; i < cnt; i++) {
        twItem = treeWid->topLevelItem(i);

        devIdx = twItem->data(0, m_UserRole_DeviceId).toInt();
        if (devIdx == devId) {
            return twItem;
        }
    }
    return 0;               //twItem;
}


void
DevicesManagerNew::updateDevicesList(DeviceList * devices,
                                     QTreeWidget * treeWid,
                                     MidiDevice::DeviceDirection in_out_direction)
{
    RG_DEBUG << "DevicesManagerNew::updateDevicesList(...)" << endl;

    /**
     * This method:
     * 1. removes deleted devices from list
     * 2. adds a list entry, if a new device was found
     * 3. updates the port-names (connections) of the listed devices
     *
     * params:
     * in_out_direction must be MidiDevice::Play or MidiDevice::Record
     **/
//         * col: the column in the treeWidget to show the connection-name (port)
    int i, cnt;
    QTreeWidgetItem *twItem;
    DeviceId devId = Device::NO_DEVICE;
    Device *device;
    MidiDevice *mdev;
    QString outPort;
    QList < MidiDevice * >midiDevices;
    QString devName;

    //DeviceList *
    devices = m_studio->getDevices();

//         QStringList listEntries;
    QList <DeviceId> listEntries;


//         cnt = m_treeWidget_playbackDevices->topLevelItemCount();
    cnt = treeWid->topLevelItemCount();
    // create a list of IDs of all listed devices
    //
    //for( i=0; i < cnt; i++ ){
    i = 0;
    while (i < cnt) {
        twItem = treeWid->topLevelItem(i);

        devId = twItem->data(0, m_UserRole_DeviceId).toInt();
        mdev = getDeviceById(devId);

        // if the device does not exist (anymore),
        // auto-remove the device from the list
        if (!mdev) {    //== Device::NO_DEVICE ){
            twItem = treeWid->takeTopLevelItem(i); // remove list entry
            //
            cnt = treeWid->topLevelItemCount(); // update count
            continue;
        }
        listEntries << static_cast < int >(devId); // append to list
        //
        i += 1;
    }


    cnt = devices->size();
    //
    for (i = 0; i < cnt; i++) {
        device = devices->at(i);

        if (device->getType() == Device::Midi) {
            mdev = dynamic_cast < MidiDevice * >(device);
            if (mdev) {
                midiDevices << mdev; // append
            } else {
                //RG_DEBUG << "ERROR: mdev is NULL in updateDevicesList() " << endl;
                //continue;
            }
        }               // end if MidiDevice
    }


    cnt = midiDevices.size();
    //
    for (i = 0; i < cnt; i++) {

        mdev = midiDevices.at(i);

        RG_DEBUG << "DevicesManagerNew: mdev = midiDevices.at(" << i << ") == id: " << mdev->getId() << endl;

        if (mdev->getDirection() == in_out_direction) {

            RG_DEBUG << "DevicesManagerNew: direction matches in_out_direction" << endl;

            //m_playDevices.push_back ( mdev );
            devId = mdev->getId();
            outPort = strtoqstr(mdev->getConnection());

            if (!listEntries.contains(devId)) {
                // device is not listed
                // create new entry
                RG_DEBUG << "DevicesManagerNew: listEntries does not contain devId " 
                         << devId << endl;
                twItem =
                        new QTreeWidgetItem(treeWid,
                                            QStringList() <<
                                            strtoqstr(mdev->getName()));

                // set port text
                if (mdev->getDirection() == MidiDevice::Record) {
                    // set record en-/disabled
                    twItem->setText(1,
                                    mdev->isRecording() ? tr("Yes") :
                                    tr("No"));
                    twItem->setText(2, outPort);
                } else {
                    twItem->setText(1, outPort);
                }

                // save deviceId in data field
                twItem->setData(0, m_UserRole_DeviceId,
                                QVariant((int) mdev->getId()));
                twItem->setFlags(twItem->flags() | Qt::ItemIsEditable);
                twItem->setSizeHint(0, QSize(24, 24));
                treeWid->addTopLevelItem(twItem);

            } else {  // list contains mdev (midi-device)
                RG_DEBUG << "DevicesManagerNew: device is already listed" << endl;

                // device is already listed
                twItem = searchItemWithDeviceId(treeWid, devId);
                if (twItem) {
                    RG_DEBUG << "DevicesManagerNew: twItem is non-zero" << endl;

                    devName = strtoqstr(mdev->getName());

                    RG_DEBUG << "DevicesManagerNew: devName: " << devName << endl;

                    if (devName != twItem->text(0)) {
                        RG_DEBUG << "DevicesManagerNew: devName != twItem->text(0)" << endl;

                        twItem->setText(0, devName);
                    }
                    // update connection-name (port)
                    if (mdev->getDirection() == MidiDevice::Record) {
                        twItem->setText(1,
                                        mdev->isRecording() ? tr("Yes")
                                : tr("No"));
                        twItem->setText(2, outPort);
                    } else {
                        RG_DEBUG << "DevicesManagerNew: twItem->setText(1, "
                                 << outPort << ")" << endl;

                        twItem->setText(1, outPort);
                    }
                } else {
                    RG_DEBUG <<
                    "Warning: twItem is NULL in DevicesManagerNew::updateDevicesList() "
                             << endl;
                }
            }       // if contains


        } else          // other connection-direction, ignore
        {
            // skip
        }

    }                       // end for device

}                               // end funciton updateDevicesList()


MidiDevice
*DevicesManagerNew::getDeviceById(DeviceId devId)
{
    RG_DEBUG << "DevicesManagerNew::getDeviceById(...)" << endl;

    MidiDevice *mdev;
    Device *dev;
    dev = m_studio->getDevice(devId);
    mdev = dynamic_cast < MidiDevice * >(dev);
    return mdev;
}

MidiDevice
*DevicesManagerNew::getDeviceByName(QString deviceName)
{
    RG_DEBUG << "DevicesManagerNew::getDeviceByName(...)" << endl;

    Device *dev;
    MidiDevice *mdev;
    int i, cnt;
    DeviceList *devices;

    devices = m_studio->getDevices();
    cnt = devices->size();

    // search in the device list for deviceName
    for (i = 0; i < cnt; i++) {
        dev = devices->at(i);

        if (dev->getType() == Device::Midi) {
            mdev = dynamic_cast < MidiDevice * >(dev);
            if (mdev->getName() == qstrtostr(deviceName)) {
                return mdev;
            }
            //if ( mdev->getDirection() == MidiDevice::Play )

        }
    }

    return 0;
}

MidiDevice
*DevicesManagerNew::getCurrentlySelectedDevice(QTreeWidget *treeWid)
{
    RG_DEBUG << "DevicesManagerNew::getCurrentlySelectedDevice(...)" << endl;

    // return the device user-selected in the m_treeWidget_playbackDevices
    //
    QTreeWidgetItem *twItem;
    MidiDevice *mdev;
    DeviceId devId;
    QList <QTreeWidgetItem *> twItemsSelected;

    //twItem = m_treeWidget_playbackDevices->currentItem();

    twItemsSelected = treeWid->selectedItems();
    twItem = 0;
    if (twItemsSelected.count() >= 1)
        twItem = twItemsSelected.at(0);

    if (!twItem)
        return 0;

    devId = twItem->data(0, m_UserRole_DeviceId).toInt();
    mdev = getDeviceById(devId);
//        mdev = getDeviceByName( twItem->text( 0 ) );    // item->text(column)
    RG_DEBUG << "DevicesManagerNew: returning non-zero mdev" << endl;
    return mdev;
}


MidiDevice
*DevicesManagerNew::getMidiDeviceOfItem(QTreeWidgetItem *twItem)
{
    RG_DEBUG << "DevicesManagerNew::getMidiDeviceOfItem(...)" << endl;

    // return the device represented by twItem
    //
    MidiDevice *mdev;
    DeviceId devId;

    if (!twItem)
        return 0;
    devId = twItem->data(0, m_UserRole_DeviceId).toInt();
    mdev = getDeviceById(devId);
//        mdev = getDeviceByName( twItem->text( 0 ) );    // item->text(column)
    return mdev;
}


void
DevicesManagerNew::updateCheckStatesOfPortsList(QTreeWidget *treeWid_ports,
                                                QTreeWidget *treeWid_devices)
{
    RG_DEBUG << "DevicesManagerNew::updateCheckStatesOfPortsList(...)" << endl;

    QTreeWidgetItem *twItem;
    QFont font;

    MidiDevice *mdev = getCurrentlySelectedDevice(treeWid_devices);
    if (!mdev) return;

    IconLoader il;

    QString outPort = strtoqstr(mdev->getConnection());

    if (outPort.isEmpty()) {
        outPort = m_noPortName; // nullPort
    }

    RG_DEBUG << "DevicesManagerNew: outPort: " << outPort
             << " id: " << mdev->getId() << endl;

    int cnt = treeWid_ports->topLevelItemCount();

    for (int i = 0; i < cnt; i++) {
        twItem = treeWid_ports->topLevelItem(i);

        twItem->setIcon(0, il.load("DevicesManagerNew/icon-plugged-out.png"));
        twItem->setSizeHint(0, QSize(24, 24));
        font = twItem->font(0);
        font.setWeight(QFont::Normal);
        twItem->setFont(0, font); // 0=column

        if (outPort == twItem->text(0)) {
            font.setWeight(QFont::Bold);
            twItem->setFont(0, font); // 0=column
            twItem->setIcon(0, il.load("DevicesManagerNew/icon-plugged-in.png"));
        }


    } // end for i

    treeWid_ports->update();        // update view

}  // end updateCheckStatesOfPortsList()


void
DevicesManagerNew::connectMidiDeviceToPort(MidiDevice * mdev,
                                           QString portName)
{
    RG_DEBUG << "DevicesManagerNew::connectMidiDeviceToPort(" << mdev->getId()
             << ", " << portName << ")" << endl;

    if (!mdev) {
        RG_DEBUG << "Warning: mdev is NULL in DevicesManagerNew::connectPlaybackDeviceToOutputPort() "
                 << endl;
        return;
    }
    if (mdev->getType() != Device::Midi) {
        RG_DEBUG << "Warning: Type of mdev is not Device::Midi in DevicesManagerNew::connectMidiDeviceToPort() "
                 << endl;
        //return;
    }
    QString outPort;
    outPort = strtoqstr(mdev->getConnection());

    DeviceId devId = mdev->getId();

    if (outPort != portName) {
        if ((portName == "") || (portName == m_noPortName)) {
            CommandHistory::getInstance()->addCommand(new
                                                      ReconnectDeviceCommand
                                                                (m_studio, devId,
                                                                ""));
            mdev->setConnection("");
            //### FIXME!!!: This does not really disconnect the port !
            
        } else {
            RG_DEBUG << "DevicesManagerNew: portName: " << portName 
                     << " devId " << devId << endl;

            CommandHistory::getInstance()->addCommand(
                                                   new ReconnectDeviceCommand (
                                                           m_studio, devId,
                                                           qstrtostr (portName)));

            // ah HAH!  here was the culprit preventing the various intended
            // updates from working correctly                                                           
            mdev->setConnection(qstrtostr(portName));
        }
    }
}


void
DevicesManagerNew::clearAllPortsLists()
{
    RG_DEBUG << "DevicesManagerNew::clearAllPortsLists()" << endl;

//         m_playDevices.clear();
//         m_recordDevices.clear();

    m_treeWidget_playbackDevices->clear();
    m_treeWidget_recordDevices->clear();
    m_treeWidget_outputPorts->clear();
    m_treeWidget_inputPorts->clear();
}


void
DevicesManagerNew::updatePortsList(QTreeWidget * treeWid,
                                   MidiDevice::DeviceDirection PlayRecDir)
{
    RG_DEBUG << "DevicesManagerNew::updatePortsList(...)" << endl;

    /**
     *    updates (adds/removes) the ports listed in the TreeWidget
     *    this does not update/set the checkStates !
     **/
    int i, portsCount, cnt;
    QTreeWidgetItem *twItem;
    QString portName;
    QStringList portNamesAvail;     // available ports names (connections)
    QStringList portNamesListed;

    RosegardenSequencer *seq;
    seq = RosegardenSequencer::getInstance();

    portsCount = seq->getConnections(Device::Midi, PlayRecDir);     // MidiDevice::Play or MidiDevice::Record
    for (i = 0; i < portsCount; ++i) {
        portName = seq->getConnection(Device::Midi, PlayRecDir, i); // last: int connectionNr
        portNamesAvail << portName; // append
    }


    // create a list of all listed portNames
    cnt = treeWid->topLevelItemCount();
    i = 0;
    while (i < cnt) {
        twItem = treeWid->topLevelItem(i);

        portName = twItem->text(0);

        if (!portNamesAvail.contains(portName)) {
            // port disappeared, remove entry

            twItem = treeWid->takeTopLevelItem(i); // remove list entry
            //
            cnt = treeWid->topLevelItemCount(); // update count
            continue;
        }

        portNamesListed << portName;    // append to list
        //
        i += 1;
    }

    portNamesAvail << m_noPortName; // add nullPort
    portsCount += 1;        // inc count

    for (i = 0; i < portsCount; ++i) {

        //portName = seq->getConnection( Device::Midi, PlayRecDir, i );    // last: int connectionNr
        portName = portNamesAvail.at(i);

        if (!portNamesListed.contains(portName)) {
            // item is not in list
            // create new entry
            twItem =
                    new QTreeWidgetItem(treeWid,
                                        QStringList() << portName);

            //twItem->setCheckState( 0, Qt::Unchecked );
            //itemx->setFlags( itemx->flags() &! Qt::ItemIsUserCheckable );
            treeWid->addTopLevelItem(twItem);
        }

    }                       // end for i

}                               // end updatePortsList()


void
DevicesManagerNew::slotAddPlaybackDevice()
{
    RG_DEBUG << "DevicesManagerNew::slotAddPlaybackDevice()" << endl;

    QString connection = "";
    //         if ( m_playConnections.size() > 0 )
    //             connection = m_playConnections[m_playConnections.size() - 1];

    CreateOrDeleteDeviceCommand *command =
            new CreateOrDeleteDeviceCommand(m_studio,
                                            qstrtostr(tr("New Device")),
                                            Device::Midi,
                                            MidiDevice::Play,
                                            qstrtostr(connection));
    CommandHistory::getInstance()->addCommand(command);

    //     updatePlaybackDevicesList();
    slotRefreshOutputPorts();
}

void
DevicesManagerNew::slotAddRecordDevice()
{
    RG_DEBUG << "DevicesManagerNew::slotAddRecordDevice()" << endl;

    QString connection = "";
    //     if ( m_recordConnections.size() > 0 )
    //         connection = m_recordConnections[m_recordConnections.size() - 1];

    CreateOrDeleteDeviceCommand *command =
            new CreateOrDeleteDeviceCommand(m_studio,
                                            qstrtostr(tr("New Device")),
                                            Device::Midi,
                                            MidiDevice::Record,
                                            qstrtostr(connection));
    CommandHistory::getInstance()->addCommand(command);

    //     updateRecordDevicesList();
    slotRefreshInputPorts();
}


void
DevicesManagerNew::slotDeletePlaybackDevice()
{
    RG_DEBUG << "DevicesManagerNew::slotDeletePlaybackDevice()" << endl;

    MidiDevice *mdev;
    mdev = getCurrentlySelectedDevice(m_treeWidget_playbackDevices);
    if (!mdev)
        return;
    DeviceId id = mdev->getId();
    
    if (id == Device::NO_DEVICE)
        return;
    CreateOrDeleteDeviceCommand *command =
            new CreateOrDeleteDeviceCommand(m_studio, id);
    CommandHistory::getInstance()->addCommand(command);

    RosegardenSequencer::getInstance()->removeDevice(id);

    slotRefreshOutputPorts();
}


void
DevicesManagerNew::slotDeleteRecordDevice()
{
    RG_DEBUG << "DevicesManagerNew::slotDeleteRecordDevice()" << endl;

    MidiDevice *mdev;
    mdev = getCurrentlySelectedDevice(m_treeWidget_recordDevices);
    if (!mdev)
        return;
    DeviceId id = mdev->getId();
    
    if (id == Device::NO_DEVICE)
        return;
    CreateOrDeleteDeviceCommand *command =
            new CreateOrDeleteDeviceCommand(m_studio, id);
    CommandHistory::getInstance()->addCommand(command);

    slotRefreshInputPorts();
}


void
DevicesManagerNew::slotManageBanksOfPlaybackDevice()
{
    RG_DEBUG << "DevicesManagerNew::slotManageBanksOfPlaybackDevice()" << endl;

    MidiDevice *mdev;
    mdev = getCurrentlySelectedDevice(m_treeWidget_playbackDevices);
    if (!mdev)
        return;
    DeviceId devId = mdev->getId();
    if (devId == Device::NO_DEVICE)
        return;
    
    emit editBanks(devId);
}


void
DevicesManagerNew::slotEditControllerDefinitions()
{
    RG_DEBUG << "DevicesManagerNew::slotEditControllerDefinitions()" << endl;

    MidiDevice *mdev;
    mdev = getCurrentlySelectedDevice(m_treeWidget_playbackDevices);
    if (!mdev)
        return;
    DeviceId devId = mdev->getId();
    if (devId == Device::NO_DEVICE)
        return;
    
    emit editControllers(devId);
}


void
DevicesManagerNew::slotHelpRequested()
{
    QMessageBox::information(this,
                             tr
                                               ("Help for the Midi Devices-Manager Dialog"),
                             tr
                                               ("This is Rosegardens central connection station. Create and connect your Midi Devices here! "),
                             QMessageBox::Ok, QMessageBox::Ok);
}


void
DevicesManagerNew::slotDeviceItemChanged(QTreeWidgetItem * twItem,
                                         int column)
{
    RG_DEBUG << "DevicesManagerNew::slotDeviceItemChanged(...)" << endl;

    /** called, if an item of the playback or record devices list (treeWidgetItem) changed
        if the device-name column has been changed, the device will be renamed.
     **/
    MidiDevice *mdev;
    QString devName;

    mdev = getMidiDeviceOfItem(twItem);
    if (!mdev) {
        RG_DEBUG <<
        "Warning: mdev is NULL in DevicesManagerNew::slotPlaybackDeviceItemChanged() "
                 << endl;
        return;
    }
    devName = twItem->text(0);

    if (devName != strtoqstr(mdev->getName())) {
        CommandHistory::getInstance()->addCommand(new RenameDeviceCommand(m_studio,
                                                                          mdev->getId(),
                                                                          qstrtostr
                                                                          (devName)));
        emit sigDeviceNameChanged(mdev->getId());
    }

}


void
DevicesManagerNew::slotRecordDevicesListItemClicked(QTreeWidgetItem * twItem,
                                                    int col)
{
    RG_DEBUG << "DevicesManagerNew::slotRecordDevicesListItemClicked(...)" << endl;
    
    MidiDevice *mdev;
    mdev = getMidiDeviceOfItem(twItem);
    if (!mdev)
        return;

    if (col == 1) {         // column: enable recording

        if (mdev->isRecording()) {

            mdev->setRecording(false);
        } else {
            mdev->setRecording(true);
        }

        // update list entry
        twItem->setText(1, mdev->isRecording() ? tr("Yes") : tr("No"));
    }
}


void
DevicesManagerNew::connectSignalsToSlots()
{
    RG_DEBUG << "DevicesManagerNew::connectSignalsToSlots()" << endl;

    // playback devices
    connect(m_treeWidget_outputPorts,
            SIGNAL(itemClicked(QTreeWidgetItem *, int)), this,
            SLOT(slotOutputPortClicked(QTreeWidgetItem *, int)));
    connect(m_treeWidget_playbackDevices,
            SIGNAL(itemSelectionChanged()), this,
            SLOT(slotPlaybackDeviceSelected()));

    connect(m_treeWidget_playbackDevices,
            SIGNAL(itemChanged(QTreeWidgetItem *, int)), this,
            SLOT(slotDeviceItemChanged(QTreeWidgetItem *, int)));


    // record devices
    connect(m_treeWidget_inputPorts,
            SIGNAL(itemClicked(QTreeWidgetItem *, int)), this,
            SLOT(slotInputPortClicked(QTreeWidgetItem *, int)));
    connect(m_treeWidget_recordDevices, SIGNAL(itemSelectionChanged()),
            this, SLOT(slotRecordDeviceSelected()));

    connect(m_treeWidget_recordDevices,
            SIGNAL(itemClicked(QTreeWidgetItem *, int)), this,
            SLOT(slotRecordDevicesListItemClicked
                                   (QTreeWidgetItem *, int)));
    connect(m_treeWidget_recordDevices,
            SIGNAL(itemChanged(QTreeWidgetItem *, int)), this,
            SLOT(slotDeviceItemChanged(QTreeWidgetItem *, int)));


    // refresh buttons
    connect(pushButton_refreshOutputPorts, SIGNAL(clicked()),
            this, SLOT(slotRefreshOutputPorts()));
    connect(pushButton_refreshInputPorts, SIGNAL(clicked()),
            this, SLOT(slotRefreshInputPorts()));

    connect(pushButton_addLV2Device, SIGNAL(clicked()),
            this, SLOT(slotAddLV2Device()));

    // dialog box buttons
    QDialogButtonBox *bbox;
    // connect help button
    bbox = findChild < QDialogButtonBox * >("buttonBox");
    connect(bbox, SIGNAL(helpRequested()), this,
            SLOT(slotHelpRequested()));
    // connect close button
    QPushButton *pbClose;
    pbClose = bbox->button(QDialogButtonBox::Close);
    connect(pbClose, SIGNAL(clicked()), this, SLOT(slotClose()));

    // buttons
    connect(pushButton_newPlaybackDevice, SIGNAL(clicked()), this,
            SLOT(slotAddPlaybackDevice()));
    connect(pushButton_newRecordDevice, SIGNAL(clicked()), this,
            SLOT(slotAddRecordDevice()));

    connect(pushButton_deletePlaybackDevice, SIGNAL(clicked()), this,
            SLOT(slotDeletePlaybackDevice()));
    connect(pushButton_deleteRecordDevice, SIGNAL(clicked()), this,
            SLOT(slotDeleteRecordDevice()));

    connect(pushButton_manageBanksOfPlaybackDevice, SIGNAL(clicked()),
            this, SLOT(slotManageBanksOfPlaybackDevice()));
    connect(pushButton_editControllerDefinitions, SIGNAL(clicked()),
            this, SLOT(slotEditControllerDefinitions()));


} 


} // namespace Rosegarden

#include "DevicesManagerNew.moc"
