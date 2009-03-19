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


//#include "gui/ui/DevicesManagerNewUi.h"
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
#include "document/RosegardenGUIDoc.h"
//#include "document/ConfigGroups.h"
#include "sequencer/RosegardenSequencer.h"

//#include "gui/dialogs/ExportDeviceDialog.h"
//#include "gui/dialogs/ImportDeviceDialog.h"

#include "gui/general/IconLoader.h"
//#include "gui/general/ResourceFinder.h"
//#include "gui/widgets/TmpStatusMsg.h"


#include <QWidget>
// #include <QDialog>
#include <QMainWindow>
#include <QObject>
#include <QFont>
#include <QMessageBox>
//#include <QThread>

// #include <QString>
// #include <QLocale>
// #include <QTranslator>
// #include <QCoreApplication>



namespace Rosegarden {





DevicesManagerNew::~DevicesManagerNew() {
    // destructor
}


DevicesManagerNew::DevicesManagerNew(QWidget * parent,
                                     RosegardenGUIDoc *
                                     doc) : QMainWindow(parent),
    Ui::DevicesManagerNewUi(), m_doc(doc) {
    // start constructor
    //setAttribute( Qt::WA_DeleteOnClose, true );
    setObjectName("DeviceManager");
    setWindowModality(Qt::NonModal);
    //setWindowFlags( Qt::Dialog );    //| Qt::WindowStaysOnTopHint );

    m_UserRole_DeviceId = Qt::UserRole + 1;
    m_noPortName = tr("[ No port ]");        // do not translate ?

    //m_doc = 0;    // RG document
    m_studio = &m_doc->getStudio();

    //setupUi ( dynamic_cast<QMainWindow*> ( this ) );
    setupUi(this);

    // gui adjustments
    //
    // adjust some column widths for better visibility
    m_treeWidget_playbackDevices->setColumnWidth(0, 200);   // column, width
    m_treeWidget_recordDevices->setColumnWidth(0, 200);     // column, width
    m_treeWidget_recordDevices->setColumnWidth(1, 60);      // column, width
    move(60, 40);
    resize(780, 420);


    connectSignalsToSlots();

    clearAllPortsLists();
}
// end constructor */



//slot
void DevicesManagerNew::show() {
    slotRefreshOutputPorts();
    slotRefreshInputPorts();

//         QDialog:: show();
    QMainWindow::show();
}



void DevicesManagerNew::slotClose() {
    /*
       if (m_doc) {
       //CommandHistory::getInstance()->detachView(actionCollection());    //&&&
       m_doc = 0;
       }
     */
    close(0);
}



// slot
void DevicesManagerNew::slotRefreshOutputPorts() {

    updatePortsList(m_treeWidget_outputPorts, MidiDevice::Play);

    updateDevicesList(0, m_treeWidget_playbackDevices,
                      MidiDevice::Play);

    updateCheckStatesOfPortsList(m_treeWidget_outputPorts,
                                 m_treeWidget_playbackDevices);
}

// slot
void DevicesManagerNew::slotRefreshInputPorts() {

    updatePortsList(m_treeWidget_inputPorts, MidiDevice::Record);

    updateDevicesList(0, m_treeWidget_recordDevices,
                      MidiDevice::Record);

    updateCheckStatesOfPortsList(m_treeWidget_inputPorts,
                                 m_treeWidget_recordDevices);
}



// slot
void DevicesManagerNew::slotPlaybackDeviceSelected() {
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

// slot
void DevicesManagerNew::slotRecordDeviceSelected() {
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


// slot
void DevicesManagerNew::slotOutputPortClicked(QTreeWidgetItem * twItem,
                                              int column) {
    //
    //### updating the devices should not happen here
    //### this is a bad fix, that makes it work 50% of time
    updateDevicesList(0, m_treeWidget_playbackDevices,
                      MidiDevice::Play);

    // prevents checking with single click
    updateCheckStatesOfPortsList(m_treeWidget_outputPorts,
                                 m_treeWidget_playbackDevices);
}

// slot
void DevicesManagerNew::slotInputPortClicked(QTreeWidgetItem * twItem,
                                             int column) {
    //
    //### updating the devices should not happen here
    //### this is a bad fix, that makes it work 50% of time
    updateDevicesList(0, m_treeWidget_recordDevices,
                      MidiDevice::Record);

    // prevents checking with single click
    updateCheckStatesOfPortsList(m_treeWidget_inputPorts,
                                 m_treeWidget_recordDevices);
}



// slot
void DevicesManagerNew::slotOutputPortDoubleClicked(QTreeWidgetItem *
                                                    twItem, int column)
{
    MidiDevice *mdev;
    QString portName;
    int OutputPortListColumn_Name = 0;

    portName = twItem->text(OutputPortListColumn_Name);
    mdev = getCurrentlySelectedDevice(m_treeWidget_playbackDevices);
    if (!mdev) {
        return;
    }
    connectMidiDeviceToPort(mdev, portName);

    // center selected item
    QTreeWidgetItem *twItemS;
    twItemS = searchItemWithPort(m_treeWidget_outputPorts, portName);
    if (twItemS) {
        m_treeWidget_outputPorts->scrollToItem(twItemS,
                                               QAbstractItemView::
                                               PositionAtCenter);
    }
    //### updating the playback-devices-list should happen here, but doesn't really work
    updateDevicesList(0, m_treeWidget_playbackDevices,
                      MidiDevice::Play);
    updateCheckStatesOfPortsList(m_treeWidget_outputPorts,
                                 m_treeWidget_playbackDevices);

}

// slot
void DevicesManagerNew::slotInputPortDoubleClicked(QTreeWidgetItem *
                                                   twItem, int column)
{
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
    //### updating the record-devices-list should happen here, but doesn't really work
    updateDevicesList(0, m_treeWidget_recordDevices,
                      MidiDevice::Record);
    updateCheckStatesOfPortsList(m_treeWidget_inputPorts,
                                 m_treeWidget_recordDevices);
}


QTreeWidgetItem *DevicesManagerNew::searchItemWithPort(QTreeWidget *
                                                       treeWid,
                                                       QString
                                                       portName) {
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

QTreeWidgetItem *DevicesManagerNew::searchItemWithDeviceId(QTreeWidget
                                                           * treeWid,
                                                           DeviceId
                                                           devId) {
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





void DevicesManagerNew::updateDevicesList(DeviceList * devices,
                                          QTreeWidget * treeWid,
                                          MidiDevice::
                                          DeviceDirection
                                          in_out_direction) {
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
    QList < DeviceId > listEntries;


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

        if (mdev->getDirection() == in_out_direction) {

            //m_playDevices.push_back ( mdev );
            devId = mdev->getId();
            outPort = strtoqstr(mdev->getConnection());

            if (!listEntries.contains(devId)) {
                // device is not listed
                // create new entry
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

            } else  // list contains mdev (midi-device)
            {
                // device is already listed
                twItem = searchItemWithDeviceId(treeWid, devId);
                if (twItem) {
                    devName = strtoqstr(mdev->getName());
                    if (devName != twItem->text(0)) {
                        twItem->setText(0, devName);
                    }
                    // update connection-name (port)
                    if (mdev->getDirection() == MidiDevice::Record) {
                        twItem->setText(1,
                                        mdev->isRecording() ? tr("Yes")
                                : tr("No"));
                        twItem->setText(2, outPort);
                    } else {
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






MidiDevice *DevicesManagerNew::getDeviceById(DeviceId devId) {
    MidiDevice *mdev;
    Device *dev;
    dev = m_studio->getDevice(devId);
    mdev = dynamic_cast < MidiDevice * >(dev);
    return mdev;
}

MidiDevice *DevicesManagerNew::getDeviceByName(QString deviceName) {
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

MidiDevice *DevicesManagerNew::getCurrentlySelectedDevice(QTreeWidget *
                                                          treeWid) {
    // return the device user-selected in the m_treeWidget_playbackDevices
    //
    QTreeWidgetItem *twItem;
    MidiDevice *mdev;
    DeviceId devId;
    QList < QTreeWidgetItem * >twItemsSelected;

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
    return mdev;
}


MidiDevice *DevicesManagerNew::getMidiDeviceOfItem(QTreeWidgetItem *
                                                   twItem) {
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




void DevicesManagerNew::updateCheckStatesOfPortsList(QTreeWidget *
                                                     treeWid_ports,
                                                     QTreeWidget *
                                                     treeWid_devices) {
    int i, cnt;
    QTreeWidgetItem *twItem;
//         QTreeWidgetItem* twItemSelected = 0;
    MidiDevice *mdev;
    QFont font;
    QString outPort;

    mdev = getCurrentlySelectedDevice(treeWid_devices);
    //if( !mdev ) return;

    IconLoader il;
    //:/pixmaps/DevicesManagerNew/

    cnt = treeWid_ports->topLevelItemCount();
    //
    for (i = 0; i < cnt; i++) {
        twItem = treeWid_ports->topLevelItem(i);

        twItem->setIcon(0,
                        il.load
                                      ("DevicesManagerNew/icon-plugged-out.png"));
        twItem->setSizeHint(0, QSize(24, 24));
//             twItem->setIconSize( 0, QSize(32,32) );
        //twItem->setCheckState( 0, Qt::Unchecked );
        font = twItem->font(0);
        font.setWeight(QFont::Normal);
        twItem->setFont(0, font); // 0=column
//             setStretch( QFont::SemiExpanded );

        // RG_DEBUG << mdev->getConnection() << " ==> " << qstrtostr(twItem->text(0)) << endl;

        outPort = "";
        if (mdev) {
            outPort = strtoqstr(mdev->getConnection());
        }

        if (outPort.isEmpty()) {
            outPort = m_noPortName; // nullPort
        }


        if (outPort == twItem->text(0)) {
            font.setWeight(QFont::Bold);
            twItem->setFont(0, font); // 0=column
            twItem->setIcon(0,
                            il.load
                                      ("DevicesManagerNew/icon-plugged-in.png"));
//                 twItem->setCheckState( 0, Qt::Checked );
//                 twItemSelected = twItem;
        }


    }                       // end for i

    treeWid_ports->update();        // update view

}                               // end updateCheckStatesOfPortsList()


void DevicesManagerNew::connectMidiDeviceToPort(MidiDevice * mdev,
                                                QString portName) {
    if (!mdev) {
        RG_DEBUG <<
        "Warning: mdev is NULL in DevicesManagerNew::connectPlaybackDeviceToOutputPort() "
                 << endl;
        return;
    }
    if (mdev->getType() != Device::Midi) {
        RG_DEBUG <<
        "Warning: Type of mdev is not Device::Midi in DevicesManagerNew::connectMidiDeviceToPort() "
                 << endl;
        //return;
    }
    QString outPort;
    outPort = strtoqstr(mdev->getConnection());

    DeviceId devId;
    devId = mdev->getId();

    if (outPort != portName) {
        if ((portName == "") || (portName == m_noPortName)) {
            CommandHistory::getInstance()->addCommand(new
                                                      ReconnectDeviceCommand
                                                                (m_studio, devId,
                                                                ""));
            mdev->setConnection("");
            //### FIXME!!!: This does not really disconnect the port !
            
        } else {
            CommandHistory::getInstance()->addCommand(new
                                                      ReconnectDeviceCommand
                                                                (m_studio, devId,
                                                                qstrtostr
                                                                 (portName)));
        }
    }
}


void DevicesManagerNew::clearAllPortsLists() {
//         m_playDevices.clear();
//         m_recordDevices.clear();

    m_treeWidget_playbackDevices->clear();
    m_treeWidget_recordDevices->clear();
    m_treeWidget_outputPorts->clear();
    m_treeWidget_inputPorts->clear();
}


void DevicesManagerNew::updatePortsList(QTreeWidget * treeWid,
                                        MidiDevice::
                                        DeviceDirection PlayRecDir) {
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





// slot
void DevicesManagerNew::slotAddPlaybackDevice() {
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

    //sleep( 1,5 );
    //     updatePlaybackDevicesList();
    slotRefreshOutputPorts();
}


// slot
void DevicesManagerNew::slotAddRecordDevice() {
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

void DevicesManagerNew::slotDeletePlaybackDevice() {
    MidiDevice *mdev;
    mdev = getCurrentlySelectedDevice(m_treeWidget_playbackDevices);
    if (!mdev)
        return;
    DeviceId id = mdev->getId();
    //
    if (id == Device::NO_DEVICE)
        return;
    CreateOrDeleteDeviceCommand *command =
            new CreateOrDeleteDeviceCommand(m_studio, id);
    CommandHistory::getInstance()->addCommand(command);

    RosegardenSequencer::getInstance()->removeDevice(id);

    slotRefreshOutputPorts();
}

void DevicesManagerNew::slotDeleteRecordDevice() {
    MidiDevice *mdev;
    mdev = getCurrentlySelectedDevice(m_treeWidget_recordDevices);
    if (!mdev)
        return;
    DeviceId id = mdev->getId();
    //
    if (id == Device::NO_DEVICE)
        return;
    CreateOrDeleteDeviceCommand *command =
            new CreateOrDeleteDeviceCommand(m_studio, id);
    CommandHistory::getInstance()->addCommand(command);

    slotRefreshInputPorts();
}


void DevicesManagerNew::slotManageBanksOfPlaybackDevice() {
    MidiDevice *mdev;
    mdev = getCurrentlySelectedDevice(m_treeWidget_playbackDevices);
    if (!mdev)
        return;
    DeviceId devId = mdev->getId();
    if (devId == Device::NO_DEVICE)
        return;
    //
    emit editBanks(devId);
}

void DevicesManagerNew::slotEditControllerDefinitions() {
    MidiDevice *mdev;
    mdev = getCurrentlySelectedDevice(m_treeWidget_playbackDevices);
    if (!mdev)
        return;
    DeviceId devId = mdev->getId();
    if (devId == Device::NO_DEVICE)
        return;
    //
    emit editControllers(devId);
}


void DevicesManagerNew::slotAddLV2Device() {
    /*
       QMessageBox:: information(
       this, "Add LV2 Plugin Device",
       "Just an idea at the moment. \nNot implemented. ",
       QMessageBox::Ok,
       QMessageBox::Ok
       );
     */
}


void DevicesManagerNew::slotHelpRequested() {
    QMessageBox::information(this,
                             tr
                                               ("Help for the Midi Devices-Manager Dialog"),
                             tr
                                               ("This is Rosegardens central connection station. Create and connect your Midi Devices here! "),
                             QMessageBox::Ok, QMessageBox::Ok);
}


void DevicesManagerNew::slotDeviceItemChanged(QTreeWidgetItem * twItem,
                                              int column) {
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
        CommandHistory::getInstance()->addCommand(new
                                                  RenameDeviceCommand
                                                                (m_studio,
                                                                mdev->getId(),
                                                                qstrtostr
                                                                 (devName)));
        emit sigDeviceNameChanged(mdev->getId());
    }

}

void DevicesManagerNew::
slotRecordDevicesListItemDoubleClicked(QTreeWidgetItem * twItem,
                                       int col) {

    MidiDevice *mdev;
    mdev = getMidiDeviceOfItem(twItem);
    if (!mdev)
        return;

    if (col == 1) {         // column: enable recording

        if (mdev->isRecording()) {

            mdev->setRecording(false);
//                 CommandHistory::getInstance()
//                         ->addCommand( new ChangeRecordDeviceCommand(mdev->getId(), false) );
        } else {
            mdev->setRecording(true);
//                 CommandHistory::getInstance()
//                         ->addCommand( new ChangeRecordDeviceCommand(mdev->getId(), true) );
        }


        // update list entry
        twItem->setText(1, mdev->isRecording() ? tr("Yes") : tr("No"));
    }
}


void DevicesManagerNew::connectSignalsToSlots() {
    //

    // playback devices
    connect(m_treeWidget_outputPorts,
            SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this,
            SLOT(slotOutputPortDoubleClicked(QTreeWidgetItem *, int)));
    connect(m_treeWidget_outputPorts,
            SIGNAL(itemClicked(QTreeWidgetItem *, int)), this,
            SLOT(slotOutputPortClicked(QTreeWidgetItem *, int)));
    connect(m_treeWidget_playbackDevices,
            SIGNAL(itemSelectionChanged()), this,
            SLOT(slotPlaybackDeviceSelected()));

//         connect( m_treeWidget_playbackDevices, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),
//                 this, SLOT(slotPlaybackDevicesListItemDoubleClicked( QTreeWidgetItem *, int )) );
    connect(m_treeWidget_playbackDevices,
            SIGNAL(itemChanged(QTreeWidgetItem *, int)), this,
            SLOT(slotDeviceItemChanged(QTreeWidgetItem *, int)));


    // record devices
    connect(m_treeWidget_inputPorts,
            SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this,
            SLOT(slotInputPortDoubleClicked(QTreeWidgetItem *, int)));
    connect(m_treeWidget_inputPorts,
            SIGNAL(itemClicked(QTreeWidgetItem *, int)), this,
            SLOT(slotInputPortClicked(QTreeWidgetItem *, int)));
    connect(m_treeWidget_recordDevices, SIGNAL(itemSelectionChanged()),
            this, SLOT(slotRecordDeviceSelected()));

    connect(m_treeWidget_recordDevices,
            SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this,
            SLOT(slotRecordDevicesListItemDoubleClicked
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


}                               // end connectSignalsToSlots()







/*
   void
   makeConnectionList ( MidiDevice::DeviceDirection direction,
   QStringList &list )
   {
   list.clear();

   unsigned int connections = RosegardenSequencer::getInstance()->
   getConnections ( Device::Midi, direction );

   for ( unsigned int i = 0; i < connections; ++i )
   {

   list.append ( RosegardenSequencer::getInstance()->
   getConnection ( Device::Midi, direction, i ) );
   }

   list.append ( QObject::tr( "No connection" ) );
   }
 */




}                               // end namespace Rosegarden



#include "DevicesManagerNew.moc"
