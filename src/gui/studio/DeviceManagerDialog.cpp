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


#include "DeviceManagerDialog.h"

#include "ChangeRecordDeviceCommand.h"
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
#include "document/MultiViewCommandHistory.h"
#include "document/RosegardenGUIDoc.h"
#include "document/ConfigGroups.h"
#include "sequencer/RosegardenSequencer.h"
#include "gui/dialogs/ExportDeviceDialog.h"
#include "gui/dialogs/ImportDeviceDialog.h"
#include "gui/general/IconLoader.h"
#include "gui/general/EditViewBase.h"
#include "gui/kdeext/KTmpStatusMsg.h"

#include <QApplication>
#include <QAction>
#include <QFileDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <QByteArray>
#include <QDataStream>
#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QLayout>
#include <QPushButton>
#include <QSizePolicy>
#include <QString>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QToolTip>
#include <QWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QShortcut>

#include <klocale.h>
// #include <kglobal.h>
// #include <kstandardshortcut.h>
// #include <kstandardaction.h>

namespace Rosegarden
{

static const int PLAY_NAME_COL = 0;
static const int PLAY_CONNECTION_COL = 1;

static const int RECORD_NAME_COL = 0;
static const int RECORD_CURRENT_COL = 1;
static const int RECORD_CONNECTION_COL = 2;


DeviceManagerDialog::DeviceManagerDialog(QWidget *parent,
            RosegardenGUIDoc *document) :
            QMainWindow(parent),
            m_document(document),
            m_studio(&document->getStudio())
{
    QFrame * mainBox = new QFrame(this);
    setCentralWidget(mainBox);
    mainBox->setContentsMargins(10, 10, 10, 10);
    QVBoxLayout *mainLayout = new QVBoxLayout(mainBox);
    mainLayout->setSpacing(10);

    setWindowTitle(i18n("Manage MIDI Devices"));

    QGroupBox *groupBox = new QGroupBox(i18n("Play devices"), mainBox);
    QHBoxLayout *groupBoxLayout = new QHBoxLayout;
    mainLayout->addWidget(groupBox);

    m_playTable = new QTableWidget(0, 2, groupBox);
//     m_playTable->setSorting(false);
	m_playTable->setSortingEnabled(false);
	/*
    m_playTable->setRowMovingEnabled(false);	//&&&
    m_playTable->setColumnMovingEnabled(false);
	*/
	m_playTable->setShowGrid(false);
	
	m_playTable->setHorizontalHeaderItem( PLAY_NAME_COL, new QTableWidgetItem( i18n("Device")));
	m_playTable->setHorizontalHeaderItem( PLAY_CONNECTION_COL, new QTableWidgetItem( i18n("Connection")));
	
    m_playTable->horizontalHeader()->show();
    m_playTable->verticalHeader()->hide();
	
// 	m_playTable->setLeftMargin(0);
	m_playTable->setContentsMargins(0, 4, 0, 4);	// left, top, right, bottom
	
	//     m_playTable->setSelectionMode(QTableWidget::SingleRow);
	m_playTable->setSelectionBehavior( QAbstractItemView::SelectRows );
    groupBoxLayout->addWidget(m_playTable);

    QFrame *frame = new QFrame(groupBox);
    QGridLayout *vlayout = new QGridLayout(frame);
    groupBoxLayout->addWidget(frame);

    QPushButton *addButton = new QPushButton(i18n("New"));
    m_deletePlayButton = new QPushButton(i18n("Delete"));
    m_importButton = new QPushButton(i18n("Import..."));
    m_exportButton = new QPushButton(i18n("Export..."));
    m_banksButton = new QPushButton(i18n("Banks..."));
    m_controllersButton = new QPushButton(i18n("Control Events..."));

    vlayout->addWidget(addButton, 0, 0);
    vlayout->addWidget(m_deletePlayButton, 0, 1);
    vlayout->addWidget(m_importButton, 1, 0);
    vlayout->addWidget(m_exportButton, 1, 1);
    vlayout->addWidget(m_banksButton, 2, 0);
    vlayout->addWidget(m_controllersButton, 2, 1);
    vlayout->addWidget(new QLabel(" "), 3, 0, 1, 2);
    vlayout->setRowStretch(3, 10);

    frame->setLayout(vlayout);
    groupBox->setLayout(groupBoxLayout);

    addButton->setToolTip(i18n("Create a new Play device"));
    m_deletePlayButton->setToolTip(i18n("Delete the selected device"));
    m_importButton->setToolTip(i18n("Import Bank, Program and Controller data from a Rosegarden file to the selected device"));
    m_exportButton->setToolTip(i18n("Export Bank and Controller data to a Rosegarden interchange file"));
    m_banksButton->setToolTip(i18n("View and edit Banks and Programs for the selected device"));
    m_controllersButton->setToolTip(i18n("View and edit Control Events for the selected device - these are special Event types that you can define against your device and control through Control Rulers or the Instrument Parameter Box "));

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

    groupBox = new QGroupBox(i18n("Record devices"), mainBox);
    groupBoxLayout = new QHBoxLayout;
    mainLayout->addWidget(groupBox);

    m_recordTable = new QTableWidget(0, 3, groupBox);
//     m_recordTable->setSorting(false);
	m_playTable->setSortingEnabled(false);
	m_recordTable->setShowGrid(false);
	/*
    m_recordTable->setRowMovingEnabled(false);		//&&&
    m_recordTable->setColumnMovingEnabled(false);
	*/
	
    m_recordTable->setHorizontalHeaderItem( RECORD_NAME_COL, new QTableWidgetItem( i18n("Device")));
	m_recordTable->setHorizontalHeaderItem( RECORD_CURRENT_COL, new QTableWidgetItem( i18n("Current")));
	m_recordTable->setHorizontalHeaderItem( RECORD_CONNECTION_COL, new QTableWidgetItem( i18n("Connection")));
	
    m_recordTable->horizontalHeader()->show();
    m_recordTable->verticalHeader()->hide();
	
// 	m_recordTable->setLeftMargin(0);
	m_recordTable->setContentsMargins(0, 4, 0, 4);	// left, top, right, bottom
	
//     m_recordTable->setSelectionMode(QTableWidget::SingleRow);
	m_recordTable->setSelectionBehavior( QAbstractItemView::SelectRows );
// 	m_recordTable->setSelectionMode( QAbstractItemView::ExtendedSelection );
	
    groupBoxLayout->addWidget(m_recordTable);

    frame = new QFrame(groupBox);
    vlayout = new QGridLayout(frame);
    groupBoxLayout->addWidget(frame);

    addButton = new QPushButton(i18n("New"), frame);
    m_deleteRecordButton = new QPushButton(i18n("Delete"), frame);

    vlayout->addWidget(addButton, 0, 0);
    vlayout->addWidget(m_deleteRecordButton, 0, 1);
    vlayout->addWidget(new QLabel(" "), 1, 0, 1, 2);
    vlayout->setRowStretch(1, 10);

    frame->setLayout(vlayout);
    groupBox->setLayout(groupBoxLayout);

    addButton->setToolTip(i18n("Create a new Record device"));
    m_deleteRecordButton->setToolTip(i18n("Delete the selected device"));

    connect(addButton, SIGNAL(clicked()), this, SLOT(slotAddRecordDevice()));
    connect(m_deleteRecordButton, SIGNAL(clicked()), this, SLOT(slotDeleteRecordDevice()));

    connect(m_recordTable, SIGNAL(currentChanged(int, int)),
            this, SLOT(slotRecordDeviceSelected (int, int)));
    connect(m_recordTable, SIGNAL(valueChanged(int, int)),
            this, SLOT(slotRecordValueChanged (int, int)));

    connect(document, SIGNAL(devicesResyncd()), this, SLOT(slotDevicesResyncd()));

    m_noConnectionString = i18n("No connection");

    slotDevicesResyncd();

    setMinimumHeight(400);
    setMinimumWidth(600);

    QFrame* btnBox = new QFrame(mainBox);
    btnBox->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout* layout = new QHBoxLayout(btnBox);
    layout->setSpacing(10);
    mainLayout->addWidget(btnBox);

    btnBox->setSizePolicy(
        QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));

    QPushButton *closeButton = new QPushButton(i18n("Close"), btnBox);

    layout->addStretch(10);
    layout->addWidget(closeButton);
    layout->addSpacing(5);

	/*
    KAction* close = KStandardAction::close(this,
                                       SLOT(slotClose()),
                                       actionCollection());
	*/
	QAction *close = createAction("file_close", SLOT(slotClose()) );

    closeButton->setText(close->text());
    connect(closeButton, SIGNAL(clicked()), this, SLOT(slotClose()));

    mainBox->setLayout(mainLayout);

    // some adjustments
	/*
    new KToolBarPopupAction(i18n("Und&o"),
                            "undo",
                            KStandardShortcut::shortcut(KStandardShortcut::Undo),
                            actionCollection(),
                            KStandardAction::stdName(KStandardAction::Undo));

    new KToolBarPopupAction(i18n("Re&do"),
                            "redo",
                            KStandardShortcut::shortcut(KStandardShortcut::Redo),
                            actionCollection(),
                            KStandardAction::stdName(KStandardAction::Redo));
	*/
	createAction( "edit_undo" );
	createAction( "edit_redo" );
	
        createGUI("devicemanager.rc"); //@@@JAS orig. 0

//     m_document->getCommandHistory()->attachView(actionCollection());	//&&&
//     connect(m_document->getCommandHistory(), SIGNAL(commandExecuted()),//&&&
//             this, SLOT(populate()));

    m_playTable->setCurrentCell( -1, 0);
    m_recordTable->setCurrentCell( -1, 0);

//     setAutoSaveSettings(DeviceManagerConfigGroup, true);		//&&&

    //    enableButtonOK(false);
    //    enableButtonApply(false);

}

DeviceManagerDialog::~DeviceManagerDialog()
{
    if (m_document) {
//         m_document->getCommandHistory()->detachView(actionCollection());		//&&&
        m_document = 0;
    }
    
    RG_DEBUG << "\n*** DeviceManagerDialog::~DeviceManagerDialog\n" << endl;
}

void
DeviceManagerDialog::slotClose()
{
    if (m_document) {
//         m_document->getCommandHistory()->detachView(actionCollection());	//&&&
        m_document = 0;
    }

    close();
}

void
DeviceManagerDialog::slotDevicesResyncd()
{
    makeConnectionList(MidiDevice::Play, m_playConnections);
    makeConnectionList(MidiDevice::Record, m_recordConnections);

    populate();
}

void
DeviceManagerDialog::populate()
{
    DeviceList *devices = m_studio->getDevices();

    //QSettings settings;
    //settings.beginGroup( SequencerOptionsConfigGroup );

    //DeviceId recordDevice = settings.value("midirecorddevice").toUInt();
    //settings.endGroup();
    m_playDevices.clear();
    m_recordDevices.clear();

    for (DeviceList::iterator it = devices->begin();
         it != devices->end(); ++it) {
        if ((*it)->getType() == Device::Midi) {
            MidiDevice *md =
                dynamic_cast<MidiDevice *>(*it);
            if (md) {
                if (md->getDirection() == MidiDevice::Play) {
                    m_playDevices.push_back(md);
                } else {
                    m_recordDevices.push_back(md);
                }
            }
        }
    }

	while (m_playTable->rowCount() > 0) {
        m_playTable->removeRow(m_playTable->rowCount() - 1);
    }
	while (m_recordTable->rowCount() > 0) {
		m_recordTable->removeRow(m_recordTable->rowCount() - 1);
    }

    int deviceCount = 0;

    for (MidiDeviceList::iterator it = m_playDevices.begin();
         it != m_playDevices.end(); ++it) {

// 		 m_playTable->insertRows(deviceCount, 1);
		 m_playTable->insertRow(deviceCount);
		
        QString deviceName = i18n("%1", deviceCount + 1);
        QString connectionName = strtoqstr((*it)->getConnection());

// 		m_playTable->setText(deviceCount, PLAY_NAME_COL, strtoqstr((*it)->getName()));
		m_playTable->item(deviceCount, PLAY_NAME_COL)->setText( strtoqstr((*it)->getName()) );
		// or:
		//m_playTable->setItem(deviceCount, PLAY_NAME_COL, new QTableWidgetItem(strtoqstr((*it)->getName())) );
		
        int currentConnectionIndex = m_playConnections.size() - 1;
        for (unsigned int i = 0; i < m_playConnections.size(); ++i) {
            if (m_playConnections[i] == connectionName)
                currentConnectionIndex = i;
        }

		/*
        Q3ComboTableItem *item = new Q3ComboTableItem(m_playTable, m_playConnections, false);
        item->setCurrentIndex(currentConnectionIndex);
        m_playTable->setItem(deviceCount, PLAY_CONNECTION_COL, item);
		*/
		QComboBox *combo = new QComboBox( m_playTable );
		combo->addItems( m_playConnections );
		combo->setCurrentIndex( currentConnectionIndex );
		m_playTable->setCellWidget( deviceCount, PLAY_CONNECTION_COL, combo );

//         m_playTable->adjustRow(deviceCount);	//&&&
        ++deviceCount;
    }

    int minPlayColumnWidths[] = { 250, 270 };
    for (int i = 0; i < 2; ++i) {
//         m_playTable->adjustColumn(i);	//&&&
		
        if (m_playTable->columnWidth(i) < minPlayColumnWidths[i])
            m_playTable->setColumnWidth(i, minPlayColumnWidths[i]);
    }

    deviceCount = 0;

    for (MidiDeviceList::iterator it = m_recordDevices.begin();
         it != m_recordDevices.end(); ++it) {

// 		 m_recordTable->insertRows(deviceCount, 1);
		 m_recordTable->insertRow(deviceCount);

        QString deviceName = i18n("%1", deviceCount + 1);
        QString connectionName = strtoqstr((*it)->getConnection());

//         m_recordTable->setText(deviceCount, RECORD_NAME_COL, strtoqstr((*it)->getName()));
		m_recordTable->item(deviceCount, RECORD_NAME_COL)->setText( strtoqstr((*it)->getName()) );
		// or:
		//m_recordTable->setItem(deviceCount, RECORD_NAME_COL, new QTableWidgetItem(strtoqstr((*it)->getName())) );
		

        int currentConnectionIndex = m_recordConnections.size() - 1;
        for (unsigned int i = 0; i < m_recordConnections.size(); ++i) {
            if (m_recordConnections[i] == connectionName)
                currentConnectionIndex = i;
        }
		
		/*
        Q3ComboTableItem *item = new Q3ComboTableItem(m_recordTable, m_recordConnections, false);
        item->setCurrentIndex(currentConnectionIndex);
        m_recordTable->setItem(deviceCount, RECORD_CONNECTION_COL, item);
		*/
		
		QComboBox *	combo = new QComboBox( m_recordTable );
		combo->addItems( m_recordConnections );
		combo->setCurrentIndex( currentConnectionIndex );
		m_playTable->setCellWidget( deviceCount, RECORD_CONNECTION_COL, combo );
		
		/*
        Q3CheckTableItem *check = new Q3CheckTableItem(m_recordTable, QString());
        //check->setChecked((*it)->getId() == recordDevice);
        //check->setText(((*it)->getId() == recordDevice) ?
        //	       i18n("Yes") : i18n("No"));
        check->setChecked((*it)->isRecording());
        check->setText((*it)->isRecording() ? i18n("Yes") : i18n("No"));
        m_recordTable->setItem(deviceCount, RECORD_CURRENT_COL, check);
		*/

		QCheckBox *check = new QCheckBox( m_recordTable );
		check->setChecked( (*it)->isRecording() );
		check->setText( (*it)->isRecording() ? i18n("Yes") : i18n("No") );
		m_recordTable->setCellWidget( deviceCount, RECORD_CURRENT_COL, check );		

//         m_recordTable->adjustRow(deviceCount);	//&&&
        ++deviceCount;
    }

    int minRecordColumnWidths[] = { 180, 70, 270 };
    for (int i = 0; i < 3; ++i) {
//         m_recordTable->adjustColumn(i);	//&&&
		
        if (m_recordTable->columnWidth(i) < minRecordColumnWidths[i])
            m_recordTable->setColumnWidth(i, minRecordColumnWidths[i]);
    }

    slotPlayDeviceSelected(m_playTable->currentRow(), m_playTable->currentColumn());
    slotRecordDeviceSelected(m_recordTable->currentRow(), m_recordTable->currentColumn());
}

void
DeviceManagerDialog::makeConnectionList(MidiDevice::DeviceDirection direction,
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
DeviceManagerDialog::closeEvent(QCloseEvent *e)
{
    emit closing();
    QMainWindow::closeEvent(e);
}

DeviceId
DeviceManagerDialog::getPlayDeviceIdAt(int row)
{
    if (row < 0 || row > (int)m_playDevices.size())
        return Device::NO_DEVICE;
    return m_playDevices[row]->getId();
}

DeviceId
DeviceManagerDialog::getRecordDeviceIdAt(int row)
{
    if (row < 0 || row > (int)m_recordDevices.size())
        return Device::NO_DEVICE;
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
         Device::Midi,
         MidiDevice::Play,
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
         Device::Midi,
         MidiDevice::Record,
         qstrtostr(connection));
    m_document->getCommandHistory()->addCommand(command);
}

void
DeviceManagerDialog::slotDeletePlayDevice()
{
    // should really grey out if nothing current
    DeviceId id = getPlayDeviceIdAt(m_playTable->currentRow());
    if (id == Device::NO_DEVICE)
        return ;
    CreateOrDeleteDeviceCommand *command = new CreateOrDeleteDeviceCommand
        (m_studio, id);
    m_document->getCommandHistory()->addCommand(command);

    RosegardenSequencer::getInstance()->removeDevice(id);
}

void
DeviceManagerDialog::slotDeleteRecordDevice()
{
    DeviceId id = getRecordDeviceIdAt(m_recordTable->currentRow());
    if (id == Device::NO_DEVICE)
        return ;
    CreateOrDeleteDeviceCommand *command = new CreateOrDeleteDeviceCommand
        (m_studio, id);
    m_document->getCommandHistory()->addCommand(command);
}

void
DeviceManagerDialog::slotPlayValueChanged(int row, int col)
{
    if (!m_document)
        return ; // closing
    DeviceId id = getPlayDeviceIdAt(row);
    if (id == Device::NO_DEVICE)
        return ;

    Device *device = m_studio->getDevice(id);
    if (!device) {
        std::cerr << "WARNING: DeviceManagerDialog::slotPlayValueChanged(): device at row "
                  << row << " (id " << id << ") not found in studio"
                  << std::endl;
        return ;
    }

    switch (col) {

    case PLAY_NAME_COL: {
        std::string name = qstrtostr(m_playTable->item(row, col)->text() );
        if (device->getName() != name) {

            m_document->getCommandHistory()->addCommand
                (new RenameDeviceCommand(m_studio, id, name));
            emit deviceNamesChanged();

            RosegardenSequencer::getInstance()->
					renameDevice(id, m_playTable->item(row, col)->text() );
        }
    }
        break;

    case PLAY_CONNECTION_COL: {
		std::string connection = qstrtostr(m_playTable->item(row, col)->text() );
        if (connection == qstrtostr(m_noConnectionString))
            connection = "";
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
    if (!m_document)
        return ; // closing
    DeviceId id = getRecordDeviceIdAt(row);
    if (id == Device::NO_DEVICE)
        return ;

    Device *device = m_studio->getDevice(id);
    if (!device) {
        std::cerr << "WARNING: DeviceManagerDialog::slotRecordValueChanged(): device at row "
                  << row << " (id " << id << ") not found in studio"
                  << std::endl;
        return ;
    }

    switch (col) {

    case RECORD_NAME_COL: {
		std::string name = qstrtostr(m_recordTable->item(row, col)->text() );
        if (device->getName() != name) {

            m_document->getCommandHistory()->addCommand
                (new RenameDeviceCommand(m_studio, id, name));
            emit deviceNamesChanged();

            RosegardenSequencer::getInstance()->
					renameDevice(id, m_recordTable->item(row, col)->text() );
        }
    }
        break;

    case RECORD_CONNECTION_COL: {
		std::string connection = qstrtostr(m_recordTable->item(row, col)->text() );
        if (device->getConnection() != connection) {
            m_document->getCommandHistory()->addCommand
                (new ReconnectDeviceCommand(m_studio, id, connection));
        }
    }
        break;

    case RECORD_CURRENT_COL: {
        m_recordTable->blockSignals(true);
		
		
        QCheckBox *check =
            dynamic_cast<QCheckBox *>(m_recordTable->item(row, col));
		
        if (!check)
            return ;

        bool actionConnect = check->isChecked();

        // The following lines are not strictly needed, but give the checkboxes
        // a smoother behavior while waiting a confirmation from the sequencer.
        //
        check->setText(actionConnect ? i18n("Yes") : i18n("No"));
        MidiDevice *device =
            dynamic_cast<MidiDevice*>(m_studio->getDevice(id));
        device->setRecording(actionConnect);

        m_recordTable->setCurrentCell(row, 0);

        m_document->getCommandHistory()->addCommand
            (new ChangeRecordDeviceCommand(id, actionConnect));

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
    DeviceId id = getPlayDeviceIdAt(m_playTable->currentRow());
    if (id == Device::NO_DEVICE)
        return ;

//     QString deviceDir = KGlobal::dirs()->findResource("appdata", "library/");
	IconLoader il;
	QString deviceDir = il.getResourcePath( "library" );
	
    QDir dir(deviceDir);
    if (!dir.exists()) {
        deviceDir = ":ROSEGARDENDEVICE";
    } else {
        deviceDir = "file://" + deviceDir;
    }

	/*
    KURL url = KFileDialog::getOpenURL
        (deviceDir,
         "audio/x-rosegarden-device audio/x-rosegarden audio/x-soundfont",
         this, i18n("Import from Device in File"));
	*/
	//### use simple file dialog for now:
	QString url_str = QFileDialog::getOpenFileName( this, i18n("Import from Device in File"), deviceDir );

	QUrl url( url_str );
	
    if (url.isEmpty())
        return ;
	
	
    ImportDeviceDialog *dialog = new ImportDeviceDialog(this, url);
    if (dialog->doImport() && dialog->exec() == QDialog::Accepted) {

        ModifyDeviceCommand *command = 0;

        BankList banks(dialog->getBanks());
        ProgramList programs(dialog->getPrograms());
        ControlList controls(dialog->getControllers());
        KeyMappingList keyMappings(dialog->getKeyMappings());
        MidiDevice::VariationType variation(dialog->getVariationType());
        std::string librarianName(dialog->getLibrarianName());
        std::string librarianEmail(dialog->getLibrarianEmail());

        // don't record the librarian when
        // merging banks -- it's misleading.
        // (also don't use variation type)
        if (!dialog->shouldOverwriteBanks()) {
            librarianName = "";
            librarianEmail = "";
        }

        command = new ModifyDeviceCommand(m_studio,
                                          id,
                                          dialog->getDeviceName(),
                                          librarianName,
                                          librarianEmail);

        if (dialog->shouldOverwriteBanks()) {
            command->setVariation(variation);
        }
        if (dialog->shouldImportBanks()) {
            command->setBankList(banks);
            command->setProgramList(programs);
        }
        if (dialog->shouldImportControllers()) {
            command->setControlList(controls);
        }
        if (dialog->shouldImportKeyMappings()) {
            command->setKeyMappingList(keyMappings);
        }

        command->setOverwrite(dialog->shouldOverwriteBanks());
        command->setRename(dialog->shouldRename());

        m_document->getCommandHistory()->addCommand(command);

        if (dialog->shouldRename())
            emit deviceNamesChanged();
    }

    delete dialog;
}

void
DeviceManagerDialog::slotExport()
{
    QString extension = "rgd";

	QString name = QFileDialog::getSaveFileName( this, i18n("Export Device as..."), QDir::currentPath(), 
			(extension.isEmpty() ? QString("*") : ("*." + extension)) );
			/*
        KFileDialog::getSaveFileName(":ROSEGARDEN",
                                     (extension.isEmpty() ? QString("*") : ("*." + extension)),
                                     this,
                                     i18n("Export Device as..."));
*/
    // Check for the existence of the name
    if (name.isEmpty())
        return ;

    // Append extension if we don't have one
    //
    if (!extension.isEmpty()) {
        if (!name.endsWith("." + extension)) {
            name += "." + extension;
        }
    }

    QFileInfo info(name);

    if (info.isDir()) {
        QMessageBox::warning
            (dynamic_cast<QWidget*>(this),
            "", /* stupid extra parameter because pure Qt sucks */
            i18n("You have specified a directory"),
            QMessageBox::Ok,
            QMessageBox::Ok);
        return ;
    }

    if (info.exists()) {
        int overwrite = QMessageBox::question
            (dynamic_cast<QWidget*>(this),
            "", /* stupid extra parameter because pure Qt sucks */
            i18n("The specified file exists.  Overwrite?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

        if (overwrite != QMessageBox::Yes)
            return ;

    }

    std::vector<DeviceId> devices;
    DeviceId id = getPlayDeviceIdAt(m_playTable->currentRow());
    MidiDevice *md = 0;
    if (id != Device::NO_DEVICE) {
        md = dynamic_cast<MidiDevice *>(m_studio->getDevice(id));
    }
    if (md) {
        ExportDeviceDialog ed(this, strtoqstr(md->getName()));
        if (ed.exec() != QDialog::Accepted)
            return ;
        if (ed.getExportType() == ExportDeviceDialog::ExportOne) {
            devices.push_back(id);
        }
    }

    m_document->exportStudio(name, devices);
}

void
DeviceManagerDialog::slotSetBanks()
{
    DeviceId id = getPlayDeviceIdAt(m_playTable->currentRow());
    emit editBanks(id);
}

void
DeviceManagerDialog::slotSetControllers()
{
    DeviceId id = getPlayDeviceIdAt(m_playTable->currentRow());
    emit editControllers(id);
}

}
#include "DeviceManagerDialog.moc"
