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

#include "devicemanager.h"
#include "constants.h"

#include <qtable.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qgroupbox.h>
#include <qpushbutton.h>
#include <qregexp.h>
#include <qgrid.h>
#include <qlayout.h>
#include <qtooltip.h>

#include <klocale.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <kstdaction.h>
#include <kaction.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <kfiledialog.h>
#include <kio/netaccess.h>

#include "rgapplication.h"
#include "rosegardenguidoc.h"
#include "rosestrings.h"
#include "rosedebug.h"
#include "rosegardendcop.h"
#include "studiocommands.h"
#include "studiocontrol.h"
#include "importdevicedialog.h"

#include "Studio.h"
#include "Device.h"
#include "MidiDevice.h"


static const int PLAY_NAME_COL = 0;
static const int PLAY_CONNECTION_COL = 1;

static const int RECORD_NAME_COL = 0;
static const int RECORD_CURRENT_COL = 1;
static const int RECORD_CONNECTION_COL = 2;

const char *const DeviceManagerDialog::DeviceManagerConfigGroup =
"Device Manager";

DeviceManagerDialog::DeviceManagerDialog(QWidget *parent,
					 RosegardenGUIDoc *document) :
    KMainWindow(parent, "deviceeditordialog"),
    m_document(document),
    m_studio(&document->getStudio())
{
    QFrame *mainBox = new QFrame(this);
    setCentralWidget(mainBox);
    QVBoxLayout *mainLayout = new QVBoxLayout(mainBox, 10, 10);

    setCaption(i18n("Manage MIDI Devices"));

    QGroupBox *groupBox = new QGroupBox(2, Horizontal, i18n("Play devices"), mainBox);

    m_playTable = new QTable(0, 2, groupBox);
    m_playTable->setSorting(false);
    m_playTable->setRowMovingEnabled(false);
    m_playTable->setColumnMovingEnabled(false);
    m_playTable->setShowGrid(false);
    m_playTable->horizontalHeader()->setLabel(PLAY_NAME_COL, i18n("Device"));
    m_playTable->horizontalHeader()->setLabel(PLAY_CONNECTION_COL, i18n("Connection"));
    m_playTable->horizontalHeader()->show();
    m_playTable->verticalHeader()->hide();
    m_playTable->setLeftMargin(0);
    m_playTable->setSelectionMode(QTable::SingleRow);

    QFrame *frame = new QFrame(groupBox);
    QVBoxLayout *vlayout = new QVBoxLayout(frame);
    QGrid *buttons = new QGrid(2, Horizontal, frame);
    QPushButton *addButton = new QPushButton(i18n("New"), buttons);
    m_deletePlayButton = new QPushButton(i18n("Delete"), buttons);
    m_importButton = new QPushButton(i18n("Import"), buttons);
    m_exportButton = new QPushButton(i18n("Export"), buttons);
    m_banksButton = new QPushButton(i18n("Banks..."), buttons);
    m_controllersButton = new QPushButton(i18n("Controllers..."), buttons);
    vlayout->addWidget(buttons);
    vlayout->addStretch(10);

    QToolTip::add(addButton,
		  i18n("Create a new Play device"));
    QToolTip::add(m_deletePlayButton,
		  i18n("Delete the selected device"));
    QToolTip::add(m_importButton,
		  i18n("Import Bank, Program and Controller data from a Rosegarden file to the selected device"));
    QToolTip::add(m_exportButton,
		  i18n("Export Bank and Controller data to a Rosegarden interchange file"));
    QToolTip::add(m_banksButton,
		  i18n("View and edit Banks and Programs for the selected device"));
    QToolTip::add(m_controllersButton,
		  i18n("View and edit Controllers for the selected device"));

    connect(addButton, SIGNAL(clicked()), this, SLOT(slotAddPlayDevice()));
    connect(m_deletePlayButton, SIGNAL(clicked()), this, SLOT(slotDeletePlayDevice()));
    connect(m_importButton, SIGNAL(clicked()), this, SLOT(slotImport()));
    connect(m_exportButton, SIGNAL(clicked()), this, SLOT(slotExport()));
    connect(m_banksButton, SIGNAL(clicked()), this, SLOT(slotSetBanks()));
    connect(m_controllersButton, SIGNAL(clicked()), this, SLOT(slotSetControllers()));

    connect(m_playTable, SIGNAL(valueChanged(int, int)),
	    this, SLOT(slotPlayValueChanged (int, int)));
    connect(m_playTable, SIGNAL(currentChanged(int, int)),
	    this, SLOT(slotPlayDeviceSelected (int, int)));

    mainLayout->addWidget(groupBox);
    groupBox = new QGroupBox(2, Horizontal, i18n("Record devices"), mainBox);

    m_recordTable = new QTable(0, 3, groupBox);
    m_recordTable->setSorting(false);
    m_recordTable->setRowMovingEnabled(false);
    m_recordTable->setColumnMovingEnabled(false);
    m_recordTable->setShowGrid(false);
    m_recordTable->horizontalHeader()->setLabel(RECORD_NAME_COL, i18n("Device"));
    m_recordTable->horizontalHeader()->setLabel(RECORD_CURRENT_COL, i18n("Current"));
    m_recordTable->horizontalHeader()->setLabel(RECORD_CONNECTION_COL, i18n("Connection"));
    m_recordTable->horizontalHeader()->show();
    m_recordTable->verticalHeader()->hide();
    m_recordTable->setLeftMargin(0);
    m_recordTable->setSelectionMode(QTable::SingleRow);

    frame = new QFrame(groupBox);
    vlayout = new QVBoxLayout(frame);
    buttons = new QGrid(2, Horizontal, frame);
    addButton = new QPushButton(i18n("New"), buttons);
    m_deleteRecordButton = new QPushButton(i18n("Delete"), buttons);
    vlayout->addWidget(buttons);
    vlayout->addStretch(10);

    QToolTip::add(addButton,
		  i18n("Create a new Record device"));
    QToolTip::add(m_deleteRecordButton,
		  i18n("Delete the selected device"));

    connect(addButton, SIGNAL(clicked()), this, SLOT(slotAddRecordDevice()));
    connect(m_deleteRecordButton, SIGNAL(clicked()), this, SLOT(slotDeleteRecordDevice()));

    connect(m_recordTable, SIGNAL(currentChanged(int, int)),
	    this, SLOT(slotRecordDeviceSelected (int, int)));
    connect(m_recordTable, SIGNAL(valueChanged(int, int)),
	    this, SLOT(slotRecordValueChanged (int, int)));

    connect(document, SIGNAL(devicesResyncd()), this, SLOT(slotDevicesResyncd()));

    slotDevicesResyncd();

    setMinimumHeight(400);
    setMinimumWidth(600);

    mainLayout->addWidget(groupBox);

    QFrame* btnBox = new QFrame(mainBox);

    btnBox->setSizePolicy(
            QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));

    QPushButton *closeButton = new QPushButton(i18n("Close"), btnBox);

    QHBoxLayout* layout = new QHBoxLayout(btnBox, 0, 10);
    layout->addStretch(10);
    layout->addWidget(closeButton);
    layout->addSpacing(5);

    KAction* close = KStdAction::close(this,
                                       SLOT(slotClose()),
                                       actionCollection());

    closeButton->setText(close->text());
    connect(closeButton, SIGNAL(released()), this, SLOT(slotClose()));

    mainLayout->addWidget(btnBox);

    // some adjustments
    new KToolBarPopupAction(i18n("Und&o"),
                            "undo",
                            KStdAccel::shortcut(KStdAccel::Undo),
                            actionCollection(),
                            KStdAction::stdName(KStdAction::Undo));

    new KToolBarPopupAction(i18n("Re&do"),
                            "redo",
                            KStdAccel::shortcut(KStdAccel::Redo),
                            actionCollection(),
                            KStdAction::stdName(KStdAction::Redo));
    /*
    new KToolBarPopupAction(i18n("&New"), "filenew",
			    KStdAccel::shortcut(KStdAccel::New),
			    this, SLOT(fileNew()),
			    actionCollection(), "new");
    */
    createGUI("devicemanager.rc");

    m_document->getCommandHistory()->attachView(actionCollection());
    connect(m_document->getCommandHistory(), SIGNAL(commandExecuted()),
            this, SLOT(populate()));

    m_playTable->setCurrentCell(-1, 0);
    m_recordTable->setCurrentCell(-1, 0);

    setAutoSaveSettings(DeviceManagerConfigGroup, true);

//    enableButtonOK(false);
//    enableButtonApply(false);

}

DeviceManagerDialog::~DeviceManagerDialog()
{
}

void
DeviceManagerDialog::slotClose()
{
    if (m_document) {
	m_document->getCommandHistory()->detachView(actionCollection());
	m_document = 0;
    }

    close();
}    

void
DeviceManagerDialog::slotDevicesResyncd()
{
    makeConnectionList((unsigned int)Rosegarden::MidiDevice::Play,
		       m_playConnections);
    makeConnectionList((unsigned int)Rosegarden::MidiDevice::Record,
		       m_recordConnections);

    populate();
}

void
DeviceManagerDialog::populate()
{
    Rosegarden::DeviceList *devices = m_studio->getDevices();

/* no, doesn't work for device renames

    // If everything is as it was last time around, then do no more
    bool changed = false;
    size_t playCount = 0, recordCount = 0;
    for (Rosegarden::DeviceList::iterator it = devices->begin();
	 it != devices->end(); ++it) {
	if ((*it)->getType() == Rosegarden::Device::Midi) {
	    Rosegarden::MidiDevice *md =
		dynamic_cast<Rosegarden::MidiDevice *>(*it);
	    if (md) {
		if (md->getDirection() == Rosegarden::MidiDevice::Play) {
		    if (playCount >= m_playDevices.size() ||
			m_playDevices[playCount]->getName() != md->getName() ||
			m_playDevices[playCount]->getConnection() != md->getConnection()) {
			changed = true;
			break;
		    }
		    ++playCount;
		} else {
		    if (recordCount >= m_recordDevices.size() ||
			m_recordDevices[recordCount]->getName() != md->getName() ||
			m_recordDevices[recordCount]->getConnection() != md->getConnection()) {
			changed = true;
			break;
		    }
		    ++recordCount;
		}
	    }
	}
    }
    if (!changed &&
	playCount == m_playDevices.size() &&
	recordCount == m_recordDevices.size()) return;
*/

    KConfig *config = kapp->config();
    config->setGroup(Rosegarden::SequencerOptionsConfigGroup);
    Rosegarden::DeviceId recordDevice =
	config->readUnsignedNumEntry("midirecorddevice");

    m_playDevices.clear();
    m_recordDevices.clear();

    for (Rosegarden::DeviceList::iterator it = devices->begin();
	 it != devices->end(); ++it) {
	if ((*it)->getType() == Rosegarden::Device::Midi) {
	    Rosegarden::MidiDevice *md =
		dynamic_cast<Rosegarden::MidiDevice *>(*it);
	    if (md) {
		if (md->getDirection() == Rosegarden::MidiDevice::Play) {
		    m_playDevices.push_back(md);
		} else {
		    m_recordDevices.push_back(md);
		}		    
	    }
	}
    }

    while (m_playTable->numRows() > 0) {
	m_playTable->removeRow(m_playTable->numRows()-1);
    }
    while (m_recordTable->numRows() > 0) {
	m_recordTable->removeRow(m_recordTable->numRows()-1);
    }

    int deviceCount = 0;

    for (MidiDeviceList::iterator it = m_playDevices.begin();
	 it != m_playDevices.end(); ++it) {

	m_playTable->insertRows(deviceCount, 1);

	QString deviceName = i18n("%1").arg(deviceCount + 1);
	QString connectionName = strtoqstr((*it)->getConnection());

	m_playTable->setText(deviceCount, PLAY_NAME_COL,
			     strtoqstr((*it)->getName()));
			 
	int currentConnectionIndex = m_playConnections.size() - 1;
	for (unsigned int i = 0; i < m_playConnections.size(); ++i) {
	    if (m_playConnections[i] == connectionName) currentConnectionIndex = i;
	}

	QComboTableItem *item = new QComboTableItem(m_playTable, m_playConnections, false);
	item->setCurrentItem(currentConnectionIndex);
	m_playTable->setItem(deviceCount, PLAY_CONNECTION_COL, item);

	m_playTable->adjustRow(deviceCount);
	++deviceCount;
    }

    int minPlayColumnWidths[] = { 250, 270 };
    for (int i = 0; i < 2; ++i) {
	m_playTable->adjustColumn(i);
	if (m_playTable->columnWidth(i) < minPlayColumnWidths[i])
	    m_playTable->setColumnWidth(i, minPlayColumnWidths[i]);
    }

    deviceCount = 0;

    for (MidiDeviceList::iterator it = m_recordDevices.begin();
	 it != m_recordDevices.end(); ++it) {

	m_recordTable->insertRows(deviceCount, 1);

	QString deviceName = i18n("%1").arg(deviceCount + 1);
	QString connectionName = strtoqstr((*it)->getConnection());

	m_recordTable->setText(deviceCount, RECORD_NAME_COL,
			       strtoqstr((*it)->getName()));

	int currentConnectionIndex = m_recordConnections.size() - 1;
	for (unsigned int i = 0; i < m_recordConnections.size(); ++i) {
	    if (m_recordConnections[i] == connectionName) currentConnectionIndex = i;
	}

	QComboTableItem *item = new QComboTableItem(m_recordTable, m_recordConnections, false);
	item->setCurrentItem(currentConnectionIndex);
	m_recordTable->setItem(deviceCount, RECORD_CONNECTION_COL, item);

	QCheckTableItem *check = new QCheckTableItem(m_recordTable, QString());
	check->setChecked((*it)->getId() == recordDevice);
	check->setText(((*it)->getId() == recordDevice) ?
		       i18n("Yes") : i18n("No"));
	m_recordTable->setItem(deviceCount, RECORD_CURRENT_COL, check);

	m_recordTable->adjustRow(deviceCount);
	++deviceCount;
    }

    int minRecordColumnWidths[] = { 180, 70, 270 };
    for (int i = 0; i < 3; ++i) {
	m_recordTable->adjustColumn(i);
	if (m_recordTable->columnWidth(i) < minRecordColumnWidths[i])
	    m_recordTable->setColumnWidth(i, minRecordColumnWidths[i]);
    }

    slotPlayDeviceSelected(m_playTable->currentRow(), m_playTable->currentColumn());
    slotRecordDeviceSelected(m_recordTable->currentRow(), m_recordTable->currentColumn());
}


void
DeviceManagerDialog::makeConnectionList(unsigned int direction,
					QStringList &list)
{
    list.clear();

    QByteArray data;
    QByteArray replyData;
    QCString replyType;
    QDataStream arg(data, IO_WriteOnly);
    arg << (int)Rosegarden::Device::Midi;
    arg << direction;

    if (!rgapp->sequencerCall("getConnections(int, unsigned int)", replyType, replyData, data)) {
	RG_DEBUG << "DeviceManagerDialog: can't call Sequencer" << endl;
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
	    RG_DEBUG << "DeviceManagerDialog: can't call Sequencer" << endl;
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
DeviceManagerDialog::closeEvent(QCloseEvent *e)
{
    emit closing();
    KMainWindow::closeEvent(e);
}


class ChangeRecordDeviceCommand : public KNamedCommand
{
public:
    ChangeRecordDeviceCommand(Rosegarden::DeviceId deviceId) :
	KNamedCommand(i18n("Change Record Device")),
	m_deviceId(deviceId) { }
    
    virtual void execute() { swap(); }
    virtual void unexecute() { swap(); }

private:
    Rosegarden::DeviceId m_deviceId;
    void swap() {

	KConfig *config = kapp->config();
	config->setGroup(Rosegarden::SequencerOptionsConfigGroup);
	Rosegarden::DeviceId oldDeviceId = config->readUnsignedNumEntry
	    ("midirecorddevice", 0);

	Rosegarden::DeviceId newDeviceId = m_deviceId;
	config->writeEntry("midirecorddevice", newDeviceId);

        // send the selected device to the sequencer
        Rosegarden::MappedEvent mEdevice
	    (Rosegarden::MidiInstrumentBase, // InstrumentId
	     Rosegarden::MappedEvent::SystemRecordDevice,
	     Rosegarden::MidiByte(newDeviceId));
        Rosegarden::StudioControl::sendMappedEvent(mEdevice);

	m_deviceId = oldDeviceId;
    }
};


Rosegarden::DeviceId
DeviceManagerDialog::getPlayDeviceIdAt(int row)
{
    if (row < 0 || row > (int)m_playDevices.size())
	return Rosegarden::Device::NO_DEVICE;
    return m_playDevices[row]->getId();
}


Rosegarden::DeviceId
DeviceManagerDialog::getRecordDeviceIdAt(int row)
{
    if (row < 0 || row > (int)m_recordDevices.size()) 
	return Rosegarden::Device::NO_DEVICE;
    return m_recordDevices[row]->getId();
}


void
DeviceManagerDialog::slotAddPlayDevice()
{
    QString connection = "";
    if (m_playConnections.size() > 0) 
	connection = m_playConnections[m_playConnections.size() - 1];
    CreateOrDeleteDeviceCommand *command = new CreateOrDeleteDeviceCommand
	(m_studio, 
	 qstrtostr(i18n("New Device")),
	 Rosegarden::Device::Midi,
	 Rosegarden::MidiDevice::Play,
	 qstrtostr(connection));
    m_document->getCommandHistory()->addCommand(command);
}

void
DeviceManagerDialog::slotAddRecordDevice()
{
    QString connection = "";
    if (m_recordConnections.size() > 0) 
	connection = m_recordConnections[m_recordConnections.size() - 1];
    CreateOrDeleteDeviceCommand *command = new CreateOrDeleteDeviceCommand
	(m_studio, 
	 qstrtostr(i18n("New Device")),
	 Rosegarden::Device::Midi,
	 Rosegarden::MidiDevice::Record,
	 qstrtostr(connection));
    m_document->getCommandHistory()->addCommand(command);
}

void
DeviceManagerDialog::slotDeletePlayDevice()
{
    // should really grey out if nothing current
    Rosegarden::DeviceId id = getPlayDeviceIdAt(m_playTable->currentRow());
    if (id == Rosegarden::Device::NO_DEVICE) return;
    CreateOrDeleteDeviceCommand *command = new CreateOrDeleteDeviceCommand
	(m_studio, id);
    m_document->getCommandHistory()->addCommand(command);
}

void
DeviceManagerDialog::slotDeleteRecordDevice()
{
    Rosegarden::DeviceId id = getRecordDeviceIdAt(m_recordTable->currentRow());
    if (id == Rosegarden::Device::NO_DEVICE) return;
    CreateOrDeleteDeviceCommand *command = new CreateOrDeleteDeviceCommand
	(m_studio, id);
    m_document->getCommandHistory()->addCommand(command);
}

void
DeviceManagerDialog::slotPlayValueChanged(int row, int col)
{
    Rosegarden::DeviceId id = getPlayDeviceIdAt(row);
    if (id == Rosegarden::Device::NO_DEVICE) return;

    Rosegarden::Device *device = m_studio->getDevice(id);
    if (!device) {
	std::cerr << "WARNING: DeviceManagerDialog::slotPlayValueChanged(): device at row "
		  << row << " (id " << id << ") not found in studio"
		  << std::endl;
	return;
    }

    switch (col) {
	
    case PLAY_NAME_COL:
    {
	std::string name = qstrtostr(m_playTable->text(row, col));
	if (device->getName() != name) {
	    m_document->getCommandHistory()->addCommand
		(new RenameDeviceCommand(m_studio, id, name));
	    emit deviceNamesChanged();
	}
    }
    break;

    case PLAY_CONNECTION_COL:
    {
	std::string connection = qstrtostr(m_playTable->text(row, col));
	if (device->getConnection() != connection) {
	    m_document->getCommandHistory()->addCommand
		(new ReconnectDeviceCommand(m_studio, id, connection));
	}
    }
    break;
    }
}

void
DeviceManagerDialog::slotRecordValueChanged(int row, int col)
{
    Rosegarden::DeviceId id = getRecordDeviceIdAt(row);
    if (id == Rosegarden::Device::NO_DEVICE) return;

    Rosegarden::Device *device = m_studio->getDevice(id);
    if (!device) {
	std::cerr << "WARNING: DeviceManagerDialog::slotRecordValueChanged(): device at row "
		  << row << " (id " << id << ") not found in studio"
		  << std::endl;
	return;
    }

    switch (col) {

    case RECORD_NAME_COL:
    {
	std::string name = qstrtostr(m_recordTable->text(row, col));
	if (device->getName() != name) {
	    m_document->getCommandHistory()->addCommand
		(new RenameDeviceCommand(m_studio, id, name));
	    emit deviceNamesChanged();
	}
    }
    break;

    case RECORD_CONNECTION_COL:
    {
	std::string connection = qstrtostr(m_recordTable->text(row, col));
	if (device->getConnection() != connection) {
	    m_document->getCommandHistory()->addCommand
		(new ReconnectDeviceCommand(m_studio, id, connection));
	}
    }
    break;

    case RECORD_CURRENT_COL:
    {
	m_recordTable->blockSignals(true);

	QCheckTableItem *check =
	    dynamic_cast<QCheckTableItem *>(m_recordTable->item(row, col));
	if (!check) return;

	check->setText(i18n("Yes"));
	check->setChecked(true);

	for (int i = 0; i < m_recordTable->numRows(); ++i) {
	    if (i == row) continue;
	    QCheckTableItem *othercheck =
		dynamic_cast<QCheckTableItem *>(m_recordTable->item(i, col));
	    if (othercheck) {
		othercheck->setChecked(false);
		othercheck->setText(i18n("No"));
	    }
	}

	m_recordTable->setCurrentCell(row, 0);

	KConfig *config = kapp->config();
	config->setGroup(Rosegarden::SequencerOptionsConfigGroup);
	Rosegarden::DeviceId oldRecordId = config->readUnsignedNumEntry
	    ("midirecorddevice", 0);

	if (oldRecordId != id) {
	    m_document->getCommandHistory()->addCommand
		(new ChangeRecordDeviceCommand(id));
	}

	m_recordTable->blockSignals(false);
    }
    break;
    }
}

void
DeviceManagerDialog::slotPlayDeviceSelected(int row, int col)
{
    RG_DEBUG << "slotPlayDeviceSelected(" << row << "," << col << ")" << endl;

    bool enable = (row >= 0 && row < (int)m_playDevices.size());
    m_deletePlayButton->setEnabled(enable);
    m_importButton->setEnabled(enable);
    m_exportButton->setEnabled(enable);
    m_banksButton->setEnabled(enable);
    m_controllersButton->setEnabled(enable);
}

void
DeviceManagerDialog::slotRecordDeviceSelected(int row, int col)
{
    RG_DEBUG << "slotRecordDeviceSelected(" << row << "," << col << ")" << endl;

    bool enable = (row >= 0 && row < (int)m_recordDevices.size());
    m_deleteRecordButton->setEnabled(enable);
}

void
DeviceManagerDialog::slotImport()
{
    Rosegarden::DeviceId id = getPlayDeviceIdAt(m_playTable->currentRow());
    if (id == Rosegarden::Device::NO_DEVICE) return;

    QString deviceDir = KGlobal::dirs()->findResource("appdata", "library/");
    QDir dir(deviceDir);
    if (!dir.exists()) {
        deviceDir = ":ROSEGARDENDEVICE";
    } else {
        deviceDir = "file://" + deviceDir;
    }

    KURL url = KFileDialog::getOpenURL
        (
#if KDE_VERSION >= 306
         deviceDir,
#else
         QString::null,
#endif
         "*.rgd *.rg *.sf2|Rosegarden and Soundfont files\n*.rgd|Rosegarden device file\n*.rg|Rosegarden file\n*.sf2|SoundFont file\n*|All files",
         this, i18n("Import Banks from Device in File"));

    if (url.isEmpty()) return;

    ImportDeviceDialog *dialog = new ImportDeviceDialog(this, url);
    if (dialog->exec() == QDialog::Accepted) {

	KCommand *command = 0;

	Rosegarden::BankList banks(dialog->getBanks());
	Rosegarden::ProgramList programs(dialog->getPrograms());
	Rosegarden::ControlList controls(dialog->getControllers());
	Rosegarden::MidiDevice::VariationType variation(dialog->getVariationType());
	std::string librarianName(dialog->getLibrarianName());
	std::string librarianEmail(dialog->getLibrarianEmail());

	// don't record the librarian when
	// merging banks -- it's misleading.
	// (also don't use variation type)
	if (!dialog->shouldOverwriteBanks()) {
	    librarianName = "";
	    librarianEmail = "";
	}

	command =
	    new ModifyDeviceCommand(
		m_studio,
		id,
		dialog->getDeviceName(),
		librarianName,
		librarianEmail,
		dialog->shouldOverwriteBanks() ? &variation : 0,
		dialog->shouldImportBanks() ? &banks : 0,
		dialog->shouldImportBanks() ? &programs : 0,
		dialog->shouldImportControllers() ? &controls : 0,
		dialog->shouldOverwriteBanks(),
		dialog->shouldRename());

	m_document->getCommandHistory()->addCommand(command);

	if (dialog->shouldRename()) emit deviceNamesChanged();
    }
}

void
DeviceManagerDialog::slotExport()
{
    QString extension = "rgd";

    QString name =
        KFileDialog::getSaveFileName(":ROSEGARDEN",
               (extension.isEmpty() ? QString("*") : ("*." + extension)),
               this,
               i18n("Export Device as..."));

    // Check for the existence of the name
    if (name.isEmpty()) return;

    // Append extension if we don't have one
    //
    if (!extension.isEmpty())
    {
        if (!name.endsWith("." + extension))
        {
            name += "." + extension;
        }
    }

    QFileInfo info(name);

    if (info.isDir())
    {
        KMessageBox::sorry(this, i18n("You have specified a directory"));
        return;
    }

    if (info.exists())
    {
        int overwrite = KMessageBox::questionYesNo
            (this, i18n("The specified file exists.  Overwrite?"));

        if (overwrite != KMessageBox::Yes) return;

    }

    m_document->exportStudio(name);
}

void
DeviceManagerDialog::slotSetBanks()
{
    Rosegarden::DeviceId id = getPlayDeviceIdAt(m_playTable->currentRow());
    emit editBanks(id);
}

void
DeviceManagerDialog::slotSetControllers()
{
    Rosegarden::DeviceId id = getPlayDeviceIdAt(m_playTable->currentRow());
    emit editControllers(id);
}

