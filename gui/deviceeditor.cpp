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

#include "deviceeditor.h"

#include <qtable.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qpushbutton.h>
#include <qregexp.h>

#include <klocale.h>
#include <kmessagebox.h>

#include "rgapplication.h"
#include "rosegardenguidoc.h"
#include "rosestrings.h"
#include "rosedebug.h"
#include "rosegardendcop.h"
#include "studiocommands.h"

#include "Studio.h"
#include "Device.h"
#include "MidiDevice.h"


DeviceEditorDialog::DeviceEditorDialog(QWidget *parent,
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

    makeConnectionList((unsigned int)Rosegarden::MidiDevice::Play,
		       m_playConnections);
    makeConnectionList((unsigned int)Rosegarden::MidiDevice::Record,
		       m_recordConnections);

    populate();

    QHBox *hbox = new QHBox(mainBox);
    QPushButton *addButton = new QPushButton(i18n("Add Play Device"), hbox);
    QPushButton *addRButton = new QPushButton(i18n("Add Record Device"), hbox);
    QPushButton *deleteButton = new QPushButton(i18n("Delete Device"), hbox);
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
    Rosegarden::DeviceList *devices = m_studio->getDevices();
    Rosegarden::DeviceListIterator it;
    m_devices.clear();

    for (it = devices->begin(); it != devices->end(); ++it) {
	if ((*it)->getType() == Rosegarden::Device::Midi) {
	    Rosegarden::MidiDevice *md =
		dynamic_cast<Rosegarden::MidiDevice *>(*it);
	    if (md) m_devices.push_back(md);
	}
    }

    while (m_table->numRows() > 0) {
	m_table->removeRow(m_table->numRows()-1);
    }

    int deviceCount = 0;

#define NAME_COL 0
#define LABEL_COL 1
#define DIRECTION_COL 2
#define CONNECTION_COL 3

    for (it = m_devices.begin(); it != m_devices.end(); ++it) {

	m_table->insertRows(deviceCount, 1);

	// we know we only put MidiDevices in m_devices
	Rosegarden::MidiDevice *md = static_cast<Rosegarden::MidiDevice *>(*it);

	// if you change this string ("Device %1"), change test in slotApply
	QString deviceName = i18n("Device %1").arg(md->getId() + 1);
	QString deviceLabel = strtoqstr(md->getName());
	QString connectionName = strtoqstr(md->getConnection());

	m_table->setText(deviceCount, NAME_COL, deviceName);
	m_table->setText(deviceCount, LABEL_COL, deviceLabel);
	m_table->setText(deviceCount, DIRECTION_COL,
			 (md->getDirection() == Rosegarden::MidiDevice::Play ?
			  i18n("Play") : i18n("Record")));

	QStringList &list(md->getDirection() == Rosegarden::MidiDevice::Play ?
			  m_playConnections : m_recordConnections);
	int currentConnectionIndex = list.size() - 1;
	for (unsigned int i = 0; i < list.size(); ++i) {
	    if (list[i] == connectionName) currentConnectionIndex = i;
	}

	QComboTableItem *item = new QComboTableItem(m_table, list, false);
	item->setCurrentItem(currentConnectionIndex);
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
DeviceEditorDialog::makeConnectionList(unsigned int direction,
				       QStringList &list)
{
    QByteArray data;
    QByteArray replyData;
    QCString replyType;
    QDataStream arg(data, IO_WriteOnly);
    arg << (int)Rosegarden::Device::Midi;
    arg << direction;

    if (!rgapp->sequencerCall("getConnections(int, unsigned int)", replyType, replyData, data)) {
	RG_DEBUG << "DeviceEditorDialog: can't call Sequencer" << endl;
	list.append(i18n("No connection"));
	return;
    }

    QDataStream reply(replyData, IO_ReadOnly);
    unsigned int connections = 0;
    if (replyType == "unsigned int") reply >> connections;

    for (unsigned int i = 0; i < connections; ++i) {

	QByteArray data;
	QByteArray replyData;
	QCString replyType;
	QDataStream arg(data, IO_WriteOnly);
	arg << (int)Rosegarden::Device::Midi;
	arg << direction;
	arg << i;
	    
	
        if(!rgapp->sequencerCall("getConnection(int, unsigned int, unsigned int)",
                                 replyType, replyData, data)) {
	    RG_DEBUG << "DeviceEditorDialog: can't call Sequencer" << endl;
	    list.append(i18n("No connection"));
	    return;
	}
	    
	QDataStream reply(replyData, IO_ReadOnly);
	QString connection;
	if (replyType == "QString") {
	    reply >> connection;
	    list.append(connection);
	}
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
	
        if (reply == KMessageBox::Yes) slotApply();
    } 

    reject();
}


void
DeviceEditorDialog::slotApply()
{
    KMacroCommand *command = new KMacroCommand("Edit Devices");
    
    // first delete deleted devices, in reverse order of id (so that
    // if we undo this command we'll get the original ids back... probably)

    std::vector<Rosegarden::DeviceId> ids;

    for (Rosegarden::DeviceListIterator i = m_devices.begin();
	 i != m_devices.end(); ++i) {
	if (m_deletedDevices.find((*i)->getId()) != m_deletedDevices.end()) {
	    ids.push_back((*i)->getId());
	}
    }

    std::sort(ids.begin(), ids.end());

    for (int i = ids.size()-1; i >= 0; --i) {
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
				Rosegarden::Device::Midi,
				m_table->text(i, DIRECTION_COL) == "Play" ?
				Rosegarden::MidiDevice::Play :
				Rosegarden::MidiDevice::Record,
				qstrtostr(m_table->text(i, CONNECTION_COL))));
	} else { // existing device
	    Rosegarden::Device *device = m_studio->getDevice(deviceId);
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

    if (number && number != "") {
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
    item->setCurrentItem(m_playConnections.size() - 1);
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
    item->setCurrentItem(m_recordConnections.size() - 1);
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

