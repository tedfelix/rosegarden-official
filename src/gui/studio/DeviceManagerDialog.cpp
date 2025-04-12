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

    Written February 2009 by Emanuel Rumpf.

 */

#define RG_MODULE_STRING "[DeviceManagerDialog]"
#define RG_NO_DEBUG_PRINT

#include "DeviceManagerDialog.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Device.h"
#include "base/Event.h"
#include "base/Instrument.h"
#include "base/MidiDevice.h"
#include "base/MidiTypes.h"
#include "base/Studio.h"
#include "base/Composition.h"
#include "commands/studio/CreateOrDeleteDeviceCommand.h"
#include "commands/studio/ReconnectDeviceCommand.h"
#include "commands/studio/RenameDeviceCommand.h"
#include "document/CommandHistory.h"
#include "document/RosegardenDocument.h"
#include "sequencer/RosegardenSequencer.h"
#include "gui/general/IconLoader.h"
#include "gui/general/ThornStyle.h"

#include <QWidget>
#include <QMainWindow>
#include <QObject>
#include <QFont>
#include <QMessageBox>
#include <QUrl>
#include <QDesktopServices>


namespace Rosegarden
{


DeviceManagerDialog::~DeviceManagerDialog()
{
    RG_DEBUG << "dtor";
    if (m_observingStudio) {
        m_observingStudio = false;
        m_studio->removeObserver(this);
    }

    unobserveAllDevices();
}

DeviceManagerDialog::DeviceManagerDialog(QWidget *parent) :
    QMainWindow(parent),
    Ui::DeviceManagerDialogUi(),
    m_isClosing(false)
{
    RG_DEBUG << "DeviceManagerDialog::ctor";

    // start constructor
    setObjectName("DeviceManager");
    setWindowModality(Qt::NonModal);

    m_UserRole_DeviceId = Qt::UserRole + 1;
    m_noPortName = tr("[ No port ]");

    //m_doc = 0;    // RG document
    m_studio = &RosegardenDocument::currentDocument->getStudio();
    m_studio->addObserver(this);
    m_observingStudio = true;

    setupUi(this);

    // gui adjustments
    //
    // adjust some column widths for better visibility
    m_treeWidget_playbackDevices->setColumnWidth(0, 200);   // column, width
    m_treeWidget_recordDevices->setColumnWidth(0, 150);
    m_treeWidget_recordDevices->setColumnWidth(1, 50);
    m_treeWidget_recordDevices->setColumnWidth(3, 150);

    // enable sorting
    m_treeWidget_playbackDevices->setSortingEnabled(true);
    m_treeWidget_recordDevices->setSortingEnabled(true);

    m_treeWidget_inputPorts->setRootIsDecorated(false);
    m_treeWidget_outputPorts->setRootIsDecorated(false);

    connectSignalsToSlots();

    clearAllPortsLists();

    setAttribute(Qt::WA_DeleteOnClose);
}

void
DeviceManagerDialog::show()
{
    RG_DEBUG << "DeviceManagerDialog::show()";
    // ignore if we are in the process of closing
    if (m_isClosing) return;

    slotRefreshOutputPorts();
    slotRefreshInputPorts();

    // select the top level item by default, if one exists, just as the bank
    // editor does
    if (m_treeWidget_playbackDevices->topLevelItem(0)) {
        // This ensures ports are updated correctly to reflect selection.
        QTreeWidgetItem * topItem = m_treeWidget_playbackDevices->topLevelItem(0);
        m_treeWidget_playbackDevices->setCurrentItem(topItem);

    }
    if (m_treeWidget_recordDevices->topLevelItem(0)) {
        // This ensures ports are updated correctly to reflect selection.
        QTreeWidgetItem * topItem = m_treeWidget_recordDevices->topLevelItem(0);
        m_treeWidget_recordDevices->setCurrentItem(topItem);
    }

    QMainWindow::show();
}

void
DeviceManagerDialog::slotCloseButtonPress()
{
    RG_DEBUG << "slotCloseButtonPress";
    m_isClosing = true;

    // remove observers here to avoid crash on studio deletion

    if (m_observingStudio) {
        m_observingStudio = false;
        m_studio->removeObserver(this);
    }

    unobserveAllDevices();

    /*
       if (m_doc) {
       //CommandHistory::getInstance()->detachView(actionCollection());    //&&&
       m_doc = 0;
       }
     */
    close();
}


void
DeviceManagerDialog::slotRefreshOutputPorts()
{
    RG_DEBUG << "DeviceManagerDialog::slotRefreshOutputPorts()";

    updatePortsList(m_treeWidget_outputPorts, MidiDevice::Play);

    updateDevicesList(m_treeWidget_playbackDevices,
                      MidiDevice::Play);

    updateCheckStatesOfPortsList(m_treeWidget_outputPorts,
                                 m_treeWidget_playbackDevices);
}


void
DeviceManagerDialog::slotRefreshInputPorts()
{
    RG_DEBUG << "DeviceManagerDialog::slotRefreshInputPorts()";

    updatePortsList(m_treeWidget_inputPorts, MidiDevice::Record);

    updateDevicesList(m_treeWidget_recordDevices,
                      MidiDevice::Record);

    updateCheckStatesOfPortsList(m_treeWidget_inputPorts,
                                 m_treeWidget_recordDevices);
}

void
DeviceManagerDialog::slotPlaybackDeviceSelected()
{
    RG_DEBUG << "DeviceManagerDialog::slotPlaybackDeviceSelected()";
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
    QString conn = RosegardenSequencer::getInstance()->getConnection(mdev->getId());
    twItemS = searchItemWithPort(m_treeWidget_outputPorts, conn);
    if (twItemS) {
        m_treeWidget_outputPorts->scrollToItem(twItemS,
                                               QAbstractItemView::
                                               PositionAtCenter);
    }
}


void
DeviceManagerDialog::slotRecordDeviceSelected()
{
    RG_DEBUG << "DeviceManagerDialog::slotRecordDevicesSelected()";
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
    QString conn = RosegardenSequencer::getInstance()->getConnection(mdev->getId());
    twItemS = searchItemWithPort(m_treeWidget_inputPorts, conn);
    if (twItemS) {
        m_treeWidget_inputPorts->scrollToItem(twItemS,
                                              QAbstractItemView::
                                              PositionAtCenter);
    }
}



void
DeviceManagerDialog::slotOutputPortClicked(QTreeWidgetItem *twItem, int /* column */)
{
    RG_DEBUG << "DeviceManagerDialog::slotOutputPortClicked(...)";

    MidiDevice *mdev;
    QString portName;
    int OutputPortListColumn_Name = 0;

    portName = twItem->text(OutputPortListColumn_Name);

    RG_DEBUG << "DeviceManagerDialog: portName: " << portName;

    mdev = getCurrentlySelectedDevice(m_treeWidget_playbackDevices);
    if (!mdev) {
        return;
    }
    connectMidiDeviceToPort(mdev, portName);

    /*
    // center selected item
    QTreeWidgetItem *twItemS;
    twItemS = searchItemWithPort(m_treeWidget_outputPorts, portName);
    if (twItemS) {
       m_treeWidget_outputPorts->scrollToItem(twItemS,
                                               QAbstractItemView::
                                               PositionAtCenter);
    }
    */

    // update the playback-devices-list
    updateDevicesList(m_treeWidget_playbackDevices,
                      MidiDevice::Play);

    updateCheckStatesOfPortsList(m_treeWidget_outputPorts,
                                 m_treeWidget_playbackDevices);
}


void
DeviceManagerDialog::slotInputPortClicked(QTreeWidgetItem *twItem,
                                          int /* column */)
{
    RG_DEBUG << "DeviceManagerDialog::slotInputPortClicked(...)";

    MidiDevice *mdev;
    QString portName;
    int InputPortListColumn_Name = 0;

    portName = twItem->text(InputPortListColumn_Name);
    mdev = getCurrentlySelectedDevice(m_treeWidget_recordDevices);
    if (!mdev) {
        return;
    }
    connectMidiDeviceToPort(mdev, portName);

    /*
    // center selected item
    QTreeWidgetItem *twItemS;
    twItemS = searchItemWithPort(m_treeWidget_inputPorts, portName);
    if (twItemS) {
        m_treeWidget_inputPorts->scrollToItem(twItemS,
                                              QAbstractItemView::
                                              PositionAtCenter);
    }
    */

    // update the record-devices-list
    updateDevicesList(m_treeWidget_recordDevices,
                      MidiDevice::Record);

    updateCheckStatesOfPortsList(m_treeWidget_inputPorts,
                                 m_treeWidget_recordDevices);
}


QTreeWidgetItem
*DeviceManagerDialog::searchItemWithPort(QTreeWidget *treeWid,
                                                    QString
                                                    portName)
{
    RG_DEBUG << "DeviceManagerDialog::searchItemWithPort(...)";

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
    return nullptr;               //twItem;
}


QTreeWidgetItem
*DeviceManagerDialog::searchItemWithDeviceId(QTreeWidget *treeWid,
                                           DeviceId devId)
{
    RG_DEBUG << "DeviceManagerDialog::searchItemWithDeviceId(..., devId = "
             << devId << ")";

    // find the TreeWidgetItem, that is associated with the device with devId
    // searches Items of treeWid
    int i, cnt;
    QTreeWidgetItem *twItem;

    cnt = treeWid->topLevelItemCount();
    for (i = 0; i < cnt; i++) {
        twItem = treeWid->topLevelItem(i);

        DeviceId devIdx = twItem->data(0, m_UserRole_DeviceId).toInt();
        if (devIdx == devId) {
            return twItem;
        }
    }
    return nullptr;               //twItem;
}


void
DeviceManagerDialog::updateDevicesList
(QTreeWidget * treeWid,
 MidiDevice::DeviceDirection in_out_direction)
{
    RG_DEBUG << "DeviceManagerDialog::updateDevicesList(...)" <<
        in_out_direction;

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
    DeviceId devId = Device::NO_DEVICE;
    MidiDevice *mdev;
    QString outPort;
    QList < MidiDevice * >midiDevices;

    DeviceList *devices = m_studio->getDevices();

    // observe any devices we are not yet observing
    for(Device* device : *devices) {
        observeDevice(device);
    }

//         QStringList listEntries;
    QList <DeviceId> listEntries;


//         cnt = m_treeWidget_playbackDevices->topLevelItemCount();
    int cnt = treeWid->topLevelItemCount();
    treeWid->blockSignals(true);
    // create a list of IDs of all listed devices
    //
    //for( i=0; i < cnt; i++ ){
    int i = 0;
    while (i < cnt) {
        QTreeWidgetItem *twItem = treeWid->topLevelItem(i);

        devId = twItem->data(0, m_UserRole_DeviceId).toInt();
        mdev = getDeviceById(devId);

        // if the device does not exist (anymore),
        // auto-remove the device from the list
        // or if the direction has changed
        if (!mdev || mdev->getDirection() != in_out_direction) {
            twItem = treeWid->takeTopLevelItem(i); // remove list entry
            //
            delete(twItem);
            cnt = treeWid->topLevelItemCount(); // update count
            continue;
        }
        listEntries << static_cast < int >(devId); // append to list
        //
        i += 1;
    }


    // fill the midiDevices list with in_out_direction matches
    cnt = int(devices->size());

    for (i = 0; i < cnt; i++) {
        Device *device = devices->at(i);

        if (device->getType() == Device::Midi) {
            mdev = dynamic_cast < MidiDevice * >(device);
            if (mdev && mdev->getDirection() == in_out_direction) {
                midiDevices << mdev; // append
                RG_DEBUG <<
                    "DeviceManagerDialog: direction matches in_out_direction" <<
                    in_out_direction << mdev->getName();
            } else {
                //RG_DEBUG << "ERROR: mdev is nullptr in updateDevicesList() ";
                //continue;
            }
        }               // end if MidiDevice
    }

    cnt = int(midiDevices.size());
    //
    for (i = 0; i < cnt; i++) {

        mdev = midiDevices.at(i);

        RG_DEBUG << "DeviceManagerDialog: mdev = midiDevices.at(" << i << ") == id: " << mdev->getId();

        //m_playDevices.push_back ( mdev );
        devId = mdev->getId();
        outPort = RosegardenSequencer::getInstance()->getConnection(devId);

        if (!listEntries.contains(devId)) {
            // device is not listed
            // create new entry
            RG_DEBUG << "DeviceManagerDialog: listEntries does not contain devId "
                     << devId;

            // translate the name string, if translation is available (ie.
            // "General MIDI Device")
            std::string name = mdev->getName();
            QString nameStr = QObject::tr("%1").arg(strtoqstr(name));
            nameStr = QObject::tr(nameStr.toStdString().c_str());
            // ??? LEAK
            RG_DEBUG << "create item" << nameStr;
            QTreeWidgetItem *twItem = new QTreeWidgetItem(treeWid, QStringList() << nameStr);
            // set port text
            twItem->setText(1, outPort);

            // save deviceId in data field
            twItem->setData(0, m_UserRole_DeviceId,
                            QVariant((int) mdev->getId()));
            twItem->setFlags(twItem->flags() | Qt::ItemIsEditable);
            twItem->setSizeHint(0, QSize(24, 24));
            treeWid->addTopLevelItem(twItem);
        } else {  // list contains mdev (midi-device)
            RG_DEBUG << "DeviceManagerDialog: device is already listed";

            // device is already listed
            QTreeWidgetItem *twItem = searchItemWithDeviceId(treeWid, devId);
            if (twItem) {
                RG_DEBUG << "DeviceManagerDialog: twItem is non-zero";

                QString devName = strtoqstr(mdev->getName());

                RG_DEBUG << "DeviceManagerDialog: devName: " << devName;

                if (devName != twItem->text(0)) {
                    RG_DEBUG << "DeviceManagerDialog: devName != twItem->text(0)";

                    twItem->setText(0, devName);
                }
                // update connection-name (port)
                twItem->setText(1, outPort);
            } else {
                RG_DEBUG <<
                "Warning: twItem is nullptr in DeviceManagerDialog::updateDevicesList() "
                        ;
            }
        }
    }                   // end for device
    treeWid->blockSignals(false);

}                       // end function updateDevicesList()


MidiDevice
*DeviceManagerDialog::getDeviceById(DeviceId devId)
{
    RG_DEBUG << "DeviceManagerDialog::getDeviceById(...)";

    MidiDevice *mdev;
    Device *dev;
    dev = m_studio->getDevice(devId);
    mdev = dynamic_cast < MidiDevice * >(dev);
    return mdev;
}

/* unused
MidiDevice
*DeviceManagerDialog::getDeviceByName(QString deviceName)
{
    RG_DEBUG << "DeviceManagerDialog::getDeviceByName(...)";

    MidiDevice *mdev;
    int i, cnt;
    DeviceList *devices;

    devices = m_studio->getDevices();
    cnt = int(devices->size());

    // search in the device list for deviceName
    for (i = 0; i < cnt; i++) {
        Device *dev = devices->at(i);

        if (dev->getType() == Device::Midi) {
            mdev = dynamic_cast < MidiDevice * >(dev);
            if (mdev->getName() == qstrtostr(deviceName)) {
                return mdev;
            }
            //if ( mdev->getDirection() == MidiDevice::Play )

        }
    }

    return nullptr;
}
*/

MidiDevice
*DeviceManagerDialog::getCurrentlySelectedDevice(QTreeWidget *treeWid)
{
    RG_DEBUG << "DeviceManagerDialog::getCurrentlySelectedDevice(...)";

    // return the device user-selected in the treeWid
    //
    QTreeWidgetItem *twItem;
    MidiDevice *mdev;
    DeviceId devId;

    twItem = treeWid->currentItem();


    if (!twItem)
        return nullptr;

    devId = twItem->data(0, m_UserRole_DeviceId).toInt();
    mdev = getDeviceById(devId);
//        mdev = getDeviceByName( twItem->text( 0 ) );    // item->text(column)
    RG_DEBUG << "DeviceManagerDialog: returning non-zero mdev";
    return mdev;
}


MidiDevice
*DeviceManagerDialog::getMidiDeviceOfItem(QTreeWidgetItem *twItem)
{
    RG_DEBUG << "DeviceManagerDialog::getMidiDeviceOfItem(...)";

    // return the device represented by twItem
    //
    MidiDevice *mdev;
    DeviceId devId;

    if (!twItem)
        return nullptr;
    devId = twItem->data(0, m_UserRole_DeviceId).toInt();
    mdev = getDeviceById(devId);
//        mdev = getDeviceByName( twItem->text( 0 ) );    // item->text(column)
    return mdev;
}


void
DeviceManagerDialog::updateCheckStatesOfPortsList(QTreeWidget *treeWid_ports,
                                                QTreeWidget *treeWid_devices)
{
    RG_DEBUG << "DeviceManagerDialog::updateCheckStatesOfPortsList(...)";

    QTreeWidgetItem *twItem;
    QFont font;
    QString outPort;
    MidiDevice *mdev = getCurrentlySelectedDevice(treeWid_devices);

    // Let the popualtion of the ports happen even if we don't have a device.
    if (!mdev) {
        outPort = m_noPortName; // nullPort
    } else {
        outPort = RosegardenSequencer::getInstance()->getConnection(mdev->getId());
        if (outPort.isEmpty()) {
            outPort = m_noPortName; // nullPort
        }
    }

//    RG_DEBUG << "DeviceManagerDialog: outPort: " << outPort
//             << " id: " << mdev->getId();

    int cnt = treeWid_ports->topLevelItemCount();

    for (int i = 0; i < cnt; i++) {
        twItem = treeWid_ports->topLevelItem(i);

        twItem->setSizeHint(0, QSize(24, 24));

        if (outPort == twItem->text(0) && mdev) {
            // Make it appear selected
            treeWid_ports->setCurrentItem(twItem);
            font.setWeight(QFont::Bold);
            twItem->setFont(0, font); // 0=column
            twItem->setIcon(0, IconLoader::load("icon-plugged-in.png"));
        } else {
            // Make it appear not selected
            twItem->setIcon(0, IconLoader::load("icon-plugged-out.png"));
            font = twItem->font(0);
            font.setWeight(QFont::Normal);
            twItem->setFont(0, font); // 0=column
            twItem->setSelected(false);
        }


    } // end for i

    treeWid_ports->update();        // update view

}  // end updateCheckStatesOfPortsList()


void
DeviceManagerDialog::connectMidiDeviceToPort(MidiDevice *device,
                                             QString newPort)
{
    //RG_DEBUG << "connectMidiDeviceToPort(" << device->getId() << ", " << newPort << ")";

    if (!device) {
        RG_WARNING << "connectMidiDeviceToPort() WARNING: Device is nullptr";
        return;
    }

    if (device->getType() != Device::Midi) {
        RG_WARNING << "connectMidiDeviceToPort() WARNING: Device is not MIDI";
    }

    QString oldPort = RosegardenSequencer::getInstance()->getConnection(device->getId());

    DeviceId deviceId = device->getId();

    if (oldPort != newPort) {  // then: port changed
        // Disconnect (connect to nothing)
        if ((newPort == "") || (newPort == m_noPortName)) {
            CommandHistory::getInstance()->addCommand(
                    new ReconnectDeviceCommand(m_studio, deviceId, ""));
        } else {  // Connect
            CommandHistory::getInstance()->addCommand(
                    new ReconnectDeviceCommand(m_studio, deviceId,
                                               qstrtostr(newPort)));
        }
    }
}


void
DeviceManagerDialog::clearAllPortsLists()
{
    RG_DEBUG << "DeviceManagerDialog::clearAllPortsLists()";

//         m_playDevices.clear();
//         m_recordDevices.clear();

    m_treeWidget_playbackDevices->clear();
    m_treeWidget_recordDevices->clear();
    m_treeWidget_outputPorts->clear();
    m_treeWidget_inputPorts->clear();
}


void
DeviceManagerDialog::updatePortsList(QTreeWidget * treeWid,
                                   MidiDevice::DeviceDirection PlayRecDir)
{
    RG_DEBUG << "DeviceManagerDialog::updatePortsList(...)";

    /**
     *    updates (adds/removes) the ports listed in the TreeWidget
     *    this does not update/set the checkStates !
     **/
    int portsCount;
    QTreeWidgetItem *twItem;
    QString portName;
    QStringList portNamesAvail;     // available ports names (connections)
    QStringList portNamesListed;

    RosegardenSequencer *seq;
    seq = RosegardenSequencer::getInstance();

    treeWid->blockSignals(true);
    portsCount = seq->getConnections(Device::Midi, PlayRecDir);     // MidiDevice::Play or MidiDevice::Record
    for (int i = 0; i < portsCount; ++i) {
        portName = seq->getConnection(Device::Midi, PlayRecDir, i); // last: int connectionNr
        portNamesAvail << portName; // append
    }


    // create a list of all listed portNames
    int cnt = treeWid->topLevelItemCount();
    int i = 0;
    while (i < cnt) {
        twItem = treeWid->topLevelItem(i);

        portName = twItem->text(0);

        if (!portNamesAvail.contains(portName)) {
            // port disappeared, remove entry

            twItem = treeWid->takeTopLevelItem(i); // remove list entry
            //
            delete(twItem);
            cnt = treeWid->topLevelItemCount(); // update count
            continue;
        }

        portNamesListed << portName;    // append to list
        //
        i += 1;
    }

    portNamesAvail << m_noPortName; // add nullPort
    portsCount += 1;        // inc count

    const int itemHeight = 24; // ?
    QLinearGradient gradient(0, itemHeight, 0, 0);
    gradient.setColorAt(0, QColor(0x99, 0x99, 0x99));
    gradient.setColorAt(1, QColor(0xDD, 0xDD, 0xDD));
    QBrush bgBrush(gradient);

    for (int i = 0; i < portsCount; ++i) {

        //portName = seq->getConnection( Device::Midi, PlayRecDir, i );    // last: int connectionNr
        portName = portNamesAvail.at(i);

        if (!portNamesListed.contains(portName)) {
            // item is not in list
            // create new entry
            // ??? LEAK
            twItem =
                    new QTreeWidgetItem(treeWid,
                                        QStringList() << portName);

            //twItem->setCheckState( 0, Qt::Unchecked );

            if (ThornStyle::isEnabled()) {
                twItem->setBackground(0, bgBrush);
                twItem->setBackground(1, bgBrush);
            }

            treeWid->addTopLevelItem(twItem);
        }

    }                       // end for i
    treeWid->blockSignals(false);
}                               // end updatePortsList()


void
DeviceManagerDialog::slotAddPlaybackDevice()
{
    RG_DEBUG << "DeviceManagerDialog::slotAddPlaybackDevice()";

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

    // Try to find the new one and select it.
    QList<QTreeWidgetItem *> newDeviceItems =
            m_treeWidget_playbackDevices->findItems(tr("New Device"), Qt::MatchExactly);
    if (newDeviceItems.size() == 1)
        m_treeWidget_playbackDevices->setCurrentItem(newDeviceItems.first());
}

void
DeviceManagerDialog::slotAddRecordDevice()
{
    RG_DEBUG << "DeviceManagerDialog::slotAddRecordDevice()";

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
DeviceManagerDialog::slotDeletePlaybackDevice()
{
    //RG_DEBUG << "slotDeletePlaybackDevice()";

    const MidiDevice *mdev =
            getCurrentlySelectedDevice(m_treeWidget_playbackDevices);
    if (!mdev)
        return;

    const DeviceId deviceID = mdev->getId();
    if (deviceID == Device::NO_DEVICE)
        return;

    // Make sure the Device is not being used by a Track.

    // Track positions using the Device.
    std::vector<int> trackPositions;

    Composition &composition =
            RosegardenDocument::currentDocument->getComposition();
    const Composition::TrackMap &tracks = composition.getTracks();

    // For each Track in the Composition...
    for (const Composition::TrackMap::value_type &pair : tracks) {
        const Track *track = pair.second;
        if (!track)
            continue;

        const InstrumentId instrumentID = track->getInstrument();
        const Instrument *instrument = m_studio->getInstrumentById(instrumentID);
        if (!instrument)
            continue;
        if (instrument->getType() != Instrument::Midi)
            continue;

        const Device *device = instrument->getDevice();
        if (!device)
            continue;

        // Found a Track using this device?
        if (deviceID == device->getId())
            trackPositions.push_back(track->getPosition());
    }

    // If there are Tracks using this Device, issue a message and abort.
    if (!trackPositions.empty()) {
        QString msg{tr("The following tracks are using this device:")};
        msg += '\n';
        for (const int &trackPos : trackPositions) {
            msg += QString::number(trackPos + 1) + " ";
        }
        msg += '\n';
        msg += tr("The device cannot be deleted.");
        QMessageBox::warning(
                this,
                tr("Rosegarden"),
                msg);
        return;
    }

    // Delete the Device.

    CreateOrDeleteDeviceCommand *command =
            new CreateOrDeleteDeviceCommand(m_studio, deviceID);
    CommandHistory::getInstance()->addCommand(command);

    RosegardenSequencer::getInstance()->removeDevice(deviceID);

    slotRefreshOutputPorts();
}


void
DeviceManagerDialog::slotDeleteRecordDevice()
{
    RG_DEBUG << "DeviceManagerDialog::slotDeleteRecordDevice()";

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
DeviceManagerDialog::slotManageBanksOfPlaybackDevice()
{
    RG_DEBUG << "DeviceManagerDialog::slotManageBanksOfPlaybackDevice()";

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
DeviceManagerDialog::slotEditControllerDefinitions()
{
    RG_DEBUG << "DeviceManagerDialog::slotEditControllerDefinitions()";

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
DeviceManagerDialog::slotHelpRequested()
{
    // TRANSLATORS: We recommend pointing help URLs to a wiki page in your native language.
    // We suggest changing the two-letter language code in the following URL,
    // and establishing a wiki page there directing users to the English version
    // of the page, and inviting readers to translate the manual into your
    // language.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:device-manager-en");
    QDesktopServices::openUrl(QUrl(helpURL));

}

void
DeviceManagerDialog::slotEdit(QTreeWidgetItem * item, int)
{
    item->treeWidget()->editItem(item);
    RG_DEBUG << "DeviceManagerDialog::slotEdit()";
}

void
DeviceManagerDialog::slotDeviceItemChanged(QTreeWidgetItem * twItem,
                                           int /* column */)
{
    RG_DEBUG << "DeviceManagerDialog::slotDeviceItemChanged(...)";

    /** called, if an item of the playback or record devices list (treeWidgetItem) changed
        if the device-name column has been changed, the device will be renamed.
     **/
    MidiDevice *mdev;
    QString devName;

    mdev = getMidiDeviceOfItem(twItem);
    if (!mdev) {
        RG_DEBUG <<
        "Warning: mdev is nullptr in DeviceManagerDialog::slotPlaybackDeviceItemChanged() "
                ;
        return;
    }
    devName = twItem->text(0);

    if (devName != strtoqstr(mdev->getName())) {
        CommandHistory::getInstance()->addCommand
            (new RenameDeviceCommand(m_studio, mdev->getId(),
                                     qstrtostr(devName)));
        emit deviceNameChanged(mdev->getId());
        RG_DEBUG << "DeviceManagerDialog::slotDeviceItemChanged emitting deviceNamesChanged()";
        emit deviceNamesChanged();
    }
}

//!DEVPUSH Need to do this when a new document is loaded
void DeviceManagerDialog::slotResyncDevicesReceived(){
    /**
    // devicesResyncd is emitted by RosegardenDocument::syncDevices(),
    // which is called by SequenceManager::processAsynchronousMidi()
    // on the event MappedEvent::SystemUpdateInstruments
    //              ####################################
    // which is send by the AlsaDriver, when devices have been added or removed,
    // or if AlsaDriver::checkForNewClients() found any news
    **/
    RG_DEBUG << "DeviceManagerDialog::slotResyncDevicesReceived() -  refreshing listboxes ";

    slotRefreshOutputPorts();
    slotRefreshInputPorts();

}


void
DeviceManagerDialog::connectSignalsToSlots()
{
    RG_DEBUG << "DeviceManagerDialog::connectSignalsToSlots()";


    // connect devicesResyncd signal (updates the devices and ports lists)
    //
    // it's emitted by RosegardenDocument::syncDevices(),
    // which is called by SequenceManager::processAsynchronousMidi()
    // on the event MappedEvent::SystemUpdateInstruments
    //              ####################################
    // which is send by the AlsaDriver, when devices have been added or removed,
    // or if AlsaDriver::checkForNewClients() found any news
    //
    connect(
        RosegardenDocument::currentDocument,
            &RosegardenDocument::devicesResyncd,
        this,
            &DeviceManagerDialog::slotResyncDevicesReceived );

//     //
//     connect( m_doc,
//         SIGNAL(signalAlsaSeqPortConnectionChanged()), this,
//         SLOT(slotResyncDevicesReceived()) );


    // playback devices
    connect(m_treeWidget_outputPorts,
            &QTreeWidget::itemClicked, this,
            &DeviceManagerDialog::slotOutputPortClicked);

    connect(m_treeWidget_playbackDevices,
            &QTreeWidget::itemSelectionChanged, this,
            &DeviceManagerDialog::slotPlaybackDeviceSelected);

    connect(m_treeWidget_playbackDevices,
            &QTreeWidget::itemChanged, this,
            &DeviceManagerDialog::slotDeviceItemChanged);

    connect(m_treeWidget_playbackDevices,
            &QTreeWidget::itemDoubleClicked,
            this, &DeviceManagerDialog::slotEdit);

    // record devices
    connect(m_treeWidget_inputPorts,
            &QTreeWidget::itemClicked, this,
            &DeviceManagerDialog::slotInputPortClicked);

    connect(m_treeWidget_recordDevices, &QTreeWidget::itemSelectionChanged,
            this, &DeviceManagerDialog::slotRecordDeviceSelected);

    connect(m_treeWidget_recordDevices,
            &QTreeWidget::itemChanged, this,
            &DeviceManagerDialog::slotDeviceItemChanged);

    connect(m_treeWidget_recordDevices,
            &QTreeWidget::itemDoubleClicked,
            this, &DeviceManagerDialog::slotEdit);

    // refresh buttons
    connect(pushButton_refreshOutputPorts, &QAbstractButton::clicked,
            this, &DeviceManagerDialog::slotRefreshOutputPorts);
    connect(pushButton_refreshInputPorts, &QAbstractButton::clicked,
            this, &DeviceManagerDialog::slotRefreshInputPorts);

    // dialog box buttons
    QDialogButtonBox *bbox;
    // connect help button
    bbox = findChild < QDialogButtonBox * >("buttonBox");
    connect(bbox, &QDialogButtonBox::helpRequested, this,
            &DeviceManagerDialog::slotHelpRequested);
    // connect close button
    QPushButton *pbClose;
    pbClose = bbox->button(QDialogButtonBox::Close);
    connect(pbClose, &QAbstractButton::clicked, this, &DeviceManagerDialog::slotCloseButtonPress);

    // buttons
    connect(pushButton_newPlaybackDevice, &QAbstractButton::clicked, this,
            &DeviceManagerDialog::slotAddPlaybackDevice);
    connect(pushButton_newRecordDevice, &QAbstractButton::clicked, this,
            &DeviceManagerDialog::slotAddRecordDevice);

    connect(pushButton_deletePlaybackDevice, &QAbstractButton::clicked, this,
            &DeviceManagerDialog::slotDeletePlaybackDevice);
    connect(pushButton_deleteRecordDevice, &QAbstractButton::clicked, this,
            &DeviceManagerDialog::slotDeleteRecordDevice);

    connect(pushButton_manageBanksOfPlaybackDevice, &QAbstractButton::clicked,
            this, &DeviceManagerDialog::slotManageBanksOfPlaybackDevice);
    connect(pushButton_editControllerDefinitions, &QAbstractButton::clicked,
            this, &DeviceManagerDialog::slotEditControllerDefinitions);
}

void DeviceManagerDialog::deviceAdded(Device* device)
{
    RG_DEBUG << "deviceAdded" << device;
    observeDevice(device);
    slotRefreshOutputPorts();
    slotRefreshInputPorts();
}

void DeviceManagerDialog::deviceRemoved(Device* device)
{
    RG_DEBUG << "deviceRemoved" << device;

    // Not found?  Bail.
    if (m_observedDevices.find(device) == m_observedDevices.end())
        return;

    m_observedDevices.erase(device);
    device->removeObserver(this);

    slotRefreshOutputPorts();
    slotRefreshInputPorts();
}

void DeviceManagerDialog::deviceModified(Device* device)
{
    RG_DEBUG << "deviceModified" << device;
    slotRefreshOutputPorts();
    slotRefreshInputPorts();
}

void DeviceManagerDialog::observeDevice(Device* device)
{
    RG_DEBUG << "observeDevice" << device;
    if (m_observedDevices.find(device) != m_observedDevices.end()) return;
    m_observedDevices.insert(device);
    device->addObserver(this);
}

void DeviceManagerDialog::unobserveAllDevices()
{
    for (Device *device : m_observedDevices) {
        device->removeObserver(this);
    }
    m_observedDevices.clear();
}


}
