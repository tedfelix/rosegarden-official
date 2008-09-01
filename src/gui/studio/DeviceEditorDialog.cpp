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


#include "DeviceEditorDialog.h"

#include <klocale.h>
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Device.h"
#include "base/MidiDevice.h"
#include "base/Studio.h"
#include "commands/studio/CreateOrDeleteDeviceCommand.h"
#include "commands/studio/ReconnectDeviceCommand.h"
#include "commands/studio/RenameDeviceCommand.h"
#include "document/RosegardenGUIDoc.h"
#include "document/MultiViewCommandHistory.h"
#include "sequencer/RosegardenSequencer.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <kmessagebox.h>
#include <QByteArray>
#include <QDataStream>
#include <QPushButton>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <qtable.h>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <algorithm>


namespace Rosegarden
{

DeviceEditorDialog::DeviceEditorDialog(QDialogButtonBox::QWidget *parent,
                                       RosegardenGUIDoc *document) :
        KDialogBase(parent, "deviceeditordialog", true,
                    i18n("Manage MIDI Devices"), Ok | Apply | Close, Ok, true),
        m_document(document),
        m_studio(&document->getStudio()),
        m_modified(false)
{
    QVBox *mainBox = makeVBoxMainWidget();

    m_table = new QTable(0, 4, mainBox);
    m_table->setSorting(false);
    m_table->setRowMovingEnabled(false);
    m_table->setColumnMovingEnabled(false);
    m_table->setShowGrid(false);
    m_table->horizontalHeader()->setLabel(0, i18n("Device"));
    m_table->horizontalHeader()->setLabel(1, i18n("Name"));
    m_table->horizontalHeader()->setLabel(2, i18n("Type"));
    m_table->horizontalHeader()->setLabel(3, i18n("Connection"));
    m_table->horizontalHeader()->show();
    m_table->verticalHeader()->hide();
    m_table->setLeftMargin(0);
    m_table->setSelectionMode(QTable::SingleRow);
    m_table->setColumnReadOnly(0, true);
    m_table->setColumnReadOnly(2, true);

    makeConnectionList(MidiDevice::Play, m_playConnections);
    makeConnectionList(MidiDevice::Record, m_recordConnections);

    populate();

    QWidget *hbox = new QWidget(mainBox);
    QHBoxLayout hboxLayout = new QHBoxLayout;
    QPushButton *addButton = new QPushButton(i18n("Add Play Device"), hbox );
    hboxLayout->addWidget(addButton);
    QPushButton *addRButton = new QPushButton(i18n("Add Record Device"), hbox );
    hboxLayout->addWidget(addRButton);
    QPushButton *deleteButton = new QPushButton(i18n("Delete Device"), hbox );
    hboxLayout->addWidget(deleteButton);
    hbox->setLayout(hboxLayout);
    connect(addButton, SIGNAL(clicked()), this, SLOT(slotAddPlayDevice()));
    connect(addRButton, SIGNAL(clicked()), this, SLOT(slotAddRecordDevice()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(slotDeleteDevice()));
    connect(m_table, SIGNAL(valueChanged(int, int)),
            this, SLOT(slotValueChanged (int, int)));

    setMinimumHeight(250);

    enableButtonOK(false);
    enableButtonApply(false);
}

DeviceEditorDialog::~DeviceEditorDialog()
{
    // nothing -- don't need to clear device list (the devices were
    // just aliases for those in the studio)
}

void
DeviceEditorDialog::populate()
{
    DeviceList *devices = m_studio->getDevices();
    DeviceListIterator it;
    m_devices.clear();

    for (it = devices->begin(); it != devices->end(); ++it) {
        if ((*it)->getType() == Device::Midi) {
            MidiDevice *md =
                dynamic_cast<MidiDevice *>(*it);
            if (md)
                m_devices.push_back(md);
        }
    }

    while (m_table->numRows() > 0) {
        m_table->removeRow(m_table->numRows() - 1);
    }

    int deviceCount = 0;

#define NAME_COL 0
#define LABEL_COL 1
#define DIRECTION_COL 2
#define CONNECTION_COL 3

    for (it = m_devices.begin(); it != m_devices.end(); ++it) {

        m_table->insertRows(deviceCount, 1);

        // we know we only put MidiDevices in m_devices
        MidiDevice *md = static_cast<MidiDevice *>(*it);

        // if you change this string ("Device %1"), change test in slotApply
        QString deviceName = i18n("Device %1").arg(md->getId() + 1);
        QString deviceLabel = strtoqstr(md->getName());
        QString connectionName = strtoqstr(md->getConnection());

        m_table->setText(deviceCount, NAME_COL, deviceName);
        m_table->setText(deviceCount, LABEL_COL, deviceLabel);
        m_table->setText(deviceCount, DIRECTION_COL,
                         (md->getDirection() == MidiDevice::Play ?
                          i18n("Play") : i18n("Record")));

        QStringList &list(md->getDirection() == MidiDevice::Play ?
                          m_playConnections : m_recordConnections);
        int currentConnectionIndex = list.size() - 1;
        for (unsigned int i = 0; i < list.size(); ++i) {
            if (list[i] == connectionName)
                currentConnectionIndex = i;
        }

        QComboTableItem *item = new QComboTableItem(m_table, list, false);
        item->setCurrentIndex(currentConnectionIndex);
        m_table->setItem(deviceCount, CONNECTION_COL, item);

        m_table->adjustRow(deviceCount);
        ++deviceCount;
    }

    int minColumnWidths[] = { 80, 120, 100, 250 };
    for (int i = 0; i < 4; ++i) {
        m_table->adjustColumn(i);
        if (m_table->columnWidth(i) < minColumnWidths[i])
            m_table->setColumnWidth(i, minColumnWidths[i]);
    }
}

void
DeviceEditorDialog::makeConnectionList(MidiDevice::DeviceDirection direction,
                                       QStringList &list)
{
    list.clear();

    unsigned int connections = RosegardenSequencer::getInstance()->
        getConnections(Device::Midi, direction);

    for (unsigned int i = 0; i < connections; ++i) {
        list.append(RosegardenSequencer::getInstance()->
                    getConnection(Device::Midi, direction, i));
    }

    list.append(i18n("No connection"));
}

void
DeviceEditorDialog::setModified(bool m)
{
    if (m_modified == m) return;
    enableButtonOK(m);
    enableButtonApply(m);
    m_modified = m;
}

void
DeviceEditorDialog::slotOk()
{
    slotApply();
    accept();
}

void
DeviceEditorDialog::slotClose()
{
    if (m_modified) {

        int reply = KMessageBox::questionYesNo(this,
                                               i18n("Apply pending changes?"));

        if (reply == KMessageBox::Yes)
            slotApply();
    }

    reject();
}

void
DeviceEditorDialog::slotApply()
{
    MacroCommand *command = new MacroCommand("Edit Devices");

    // first delete deleted devices, in reverse order of id (so that
    // if we undo this command we'll get the original ids back... probably)

    std::vector<DeviceId> ids;

    for (DeviceListIterator i = m_devices.begin();
            i != m_devices.end(); ++i) {
        if (m_deletedDevices.find((*i)->getId()) != m_deletedDevices.end()) {
            ids.push_back((*i)->getId());
        }
    }

    std::sort(ids.begin(), ids.end());

    for (int i = ids.size() - 1; i >= 0; --i) {
        command->addCommand(new CreateOrDeleteDeviceCommand(m_studio, ids[i]));
    }

    // create the new devices, and rename and/or set connections for
    // any others that have changed

    for (int i = 0; i < m_table->numRows(); ++i) {
        int deviceId = getDeviceIdAt(i);
        if (deviceId < 0) { // new device
            command->addCommand(new CreateOrDeleteDeviceCommand
                                (m_studio,
                                 qstrtostr(m_table->text(i, LABEL_COL)),
                                 Device::Midi,
                                 m_table->text(i, DIRECTION_COL) == "Play" ?
                                 MidiDevice::Play :
                                 MidiDevice::Record,
                                 qstrtostr(m_table->text(i, CONNECTION_COL))));
        } else { // existing device
            Device *device = m_studio->getDevice(deviceId);
            if (!device) {
                /*
                std::cerr <<
                "WARNING: DeviceEditorDialog::slotApply(): device at row "
                << i << " (id " << deviceId
                << ") claims not to be new, but isn't in the studio"
                << std::endl;
                          */
            } else {
                std::string name = qstrtostr(m_table->text(i, LABEL_COL));
                std::string conn = qstrtostr(m_table->text(i, CONNECTION_COL));
                if (device->getName() != name) {
                    command->addCommand(new RenameDeviceCommand
                                        (m_studio, deviceId, name));
                }
                if (device->getConnection() != conn) {
                    command->addCommand(new ReconnectDeviceCommand
                                        (m_studio, deviceId, conn));
                }
            }
        }
    }

    m_document->getCommandHistory()->addCommand(command);

    m_deletedDevices.clear();

    populate();
    setModified(false);
}

int
DeviceEditorDialog::getDeviceIdAt(int row) // -1 for new device w/o an id yet
{
    QString t(m_table->text(row, 0));

    QRegExp re("^.*(\\d+).*$");
    re.search(t);

    QString number = re.cap(1);
    int id = -1;

    if (number && number != "")
    {
        id = number.toInt() - 1; // displayed device numbers are 1-based
    }

    return id;
}

void
DeviceEditorDialog::slotAddPlayDevice()
{
    int n = m_table->numRows();
    m_table->insertRows(n, 1);
    m_table->setText(n, 0, i18n("<new device>"));
    m_table->setText(n, 1, i18n("New Device"));
    m_table->setText(n, 2, i18n("Play"));

    QComboTableItem *item =
        new QComboTableItem(m_table, m_playConnections, false);
    item->setCurrentIndex(m_playConnections.size() - 1);
    m_table->setItem(n, 3, item);
    m_table->adjustRow(n);

    setModified(true);
}

void
DeviceEditorDialog::slotAddRecordDevice()
{
    int n = m_table->numRows();
    m_table->insertRows(n, 1);
    m_table->setText(n, 0, i18n("<new device>"));
    m_table->setText(n, 1, i18n("New Device"));
    m_table->setText(n, 2, i18n("Record"));

    QComboTableItem *item =
        new QComboTableItem(m_table, m_recordConnections, false);
    item->setCurrentIndex(m_recordConnections.size() - 1);
    m_table->setItem(n, 3, item);
    m_table->adjustRow(n);

    setModified(true);
}

void
DeviceEditorDialog::slotDeleteDevice()
{
    int n = m_table->currentRow();
    m_deletedDevices.insert(getDeviceIdAt(n));
    m_table->removeRow(n);
    setModified(true);
}

void
DeviceEditorDialog::slotValueChanged(int, int)
{
    setModified(true);
}

}
#include "DeviceEditorDialog.moc"
