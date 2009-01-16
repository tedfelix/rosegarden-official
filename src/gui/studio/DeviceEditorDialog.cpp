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
*/


#include "DeviceEditorDialog.h"


#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Device.h"
#include "base/MidiDevice.h"
#include "base/Studio.h"
#include "commands/studio/CreateOrDeleteDeviceCommand.h"
#include "commands/studio/ReconnectDeviceCommand.h"
#include "commands/studio/RenameDeviceCommand.h"
#include "document/RosegardenGUIDoc.h"
#include "document/CommandHistory.h"
#include "sequencer/RosegardenSequencer.h"
#include <algorithm>

#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QByteArray>
#include <QDataStream>
#include <QPushButton>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QComboBox>
// #include <QTreeWidget>
// #include <QTreeWidgetItem>


namespace Rosegarden
{

DeviceEditorDialog::DeviceEditorDialog(QWidget *parent,
                                       RosegardenGUIDoc *document) :
        QDialog(parent),
        m_document(document),
        m_studio(&document->getStudio()),
        m_modified(false)
{
    setModal(true);
    setWindowTitle(QObject::tr("Manage MIDI Devices"));
    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *mainBox = new QWidget(this);
    QVBoxLayout *mainBoxLayout = new QVBoxLayout;
    metagrid->addWidget(mainBox, 0, 0);

 	m_table = new QTableWidget(0, 4, mainBox);
// 	m_table = new QTableWidget(mainBox);
	
    m_table->setSortingEnabled(false);
	
	/*
    m_table->setRowMovingEnabled(false);	//&&&
    m_table->setColumnMovingEnabled(false);
	*/
    m_table->setShowGrid(false);
	
// 	m_table->setColumnCount( 4 );
	
	/*
	QStringList sl;
	sl << QObject::tr("Device") << QObject::tr("Name") << QObject::tr("Type") << QObject::tr("Connection");
	m_table->setHeaderLabels( sl );
	*/
	
	m_table->setHorizontalHeaderItem( 0, new QTableWidgetItem( QObject::tr("Device")));
	m_table->setHorizontalHeaderItem( 1, new QTableWidgetItem( QObject::tr("Name")));
	m_table->setHorizontalHeaderItem( 2, new QTableWidgetItem( QObject::tr("Type")));
	m_table->setHorizontalHeaderItem( 3, new QTableWidgetItem( QObject::tr("Connection")));
	
	
    m_table->horizontalHeader()->show();
    m_table->verticalHeader()->hide();
	//m_table->setLeftMargin(0);
	m_table->setContentsMargins( 0, 4, 0, 4 );	// left, top, right, bottom
	
	
//     m_table->setSelectionMode(QTableWidget::SingleRow);
	m_table->setSelectionBehavior( QAbstractItemView::SelectRows );
	
	/*
    m_table->setColumnReadOnly(0, true);	//&&&
    m_table->setColumnReadOnly(2, true);
	*/
    mainBox->layout()->addWidget(m_table);

    makeConnectionList(MidiDevice::Play, m_playConnections);
    makeConnectionList(MidiDevice::Record, m_recordConnections);

    populate();

    QWidget *hbox = new QWidget(mainBox);
    QHBoxLayout *hboxLayout = new QHBoxLayout;
    QPushButton *addButton = new QPushButton(QObject::tr("Add Play Device"), hbox );
    hboxLayout->addWidget(addButton);
    QPushButton *addRButton = new QPushButton(QObject::tr("Add Record Device"), hbox );
    hboxLayout->addWidget(addRButton);
    QPushButton *deleteButton = new QPushButton(QObject::tr("Delete Device"), hbox );
    hboxLayout->addWidget(deleteButton);
    hbox->setLayout(hboxLayout);
	

    connect(addButton, SIGNAL(clicked()), this, SLOT(slotAddPlayDevice()));
    connect(addRButton, SIGNAL(clicked()), this, SLOT(slotAddRecordDevice()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(slotDeleteDevice()));
    connect(m_table, SIGNAL(valueChanged(int, int)),
            this, SLOT(slotValueChanged (int, int)) );

    setMinimumHeight(250);

	
    //QDialogButtonBox *
	m_buttonBox = new QDialogButtonBox(  QDialogButtonBox::Ok
                                                       | QDialogButtonBox::Apply
                                                       | QDialogButtonBox::Close);
	
//     enableButtonOK(false);
//     enableButtonApply(false);
	m_buttonBox->button( QDialogButtonBox::Ok )->setEnabled( false );
	m_buttonBox->button( QDialogButtonBox::Apply )->setEnabled( false );
	
	metagrid->addWidget(m_buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
	connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
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

    while (m_table->rowCount() > 0) {
        m_table->removeRow(m_table->rowCount() - 1);
    }

    int deviceCount = 0;

#define NAME_COL 0
#define LABEL_COL 1
#define DIRECTION_COL 2
#define CONNECTION_COL 3

    for (it = m_devices.begin(); it != m_devices.end(); ++it) {

// 		m_table->insertRows(deviceCount, 1);
		m_table->insertRow( deviceCount );//m_devices.count() );

        // we know we only put MidiDevices in m_devices
        MidiDevice *md = static_cast<MidiDevice *>(*it);

        // if you change this string ("Device %1"), change test in slotApply
        QString deviceName = QObject::tr("Device %1").arg(md->getId() + 1);
        QString deviceLabel = strtoqstr(md->getName());
        QString connectionName = strtoqstr(md->getConnection());
	
			
		/*
        m_table->setText(deviceCount, NAME_COL, deviceName);
        m_table->setText(deviceCount, LABEL_COL, deviceLabel);
        m_table->setText(deviceCount, DIRECTION_COL,
                         (md->getDirection() == MidiDevice::Play ?
                          QObject::tr("Play") : QObject::tr("Record")));
		*/
		m_table->setItem(deviceCount, NAME_COL, new QTableWidgetItem(deviceName));
		m_table->setItem(deviceCount, LABEL_COL, new QTableWidgetItem(deviceLabel));
		m_table->setItem(deviceCount, DIRECTION_COL, new QTableWidgetItem(
						 (md->getDirection() == MidiDevice::Play ?
								 QObject::tr("Play") : QObject::tr("Record")) ));
		

        QStringList &list(md->getDirection() == MidiDevice::Play ?
                          m_playConnections : m_recordConnections);
        int currentConnectionIndex = list.size() - 1;
        for (unsigned int i = 0; i < list.size(); ++i) {
            if (list[i] == connectionName)
                currentConnectionIndex = i;
        }
		
		/*
 		QComboTableItem *item = new QComboTableItem(m_table, list, false);
		item->setCurrentIndex(currentConnectionIndex);
        m_table->setItem(deviceCount, CONNECTION_COL, item);
		*/
		QComboBox* combo = new QComboBox( this );
		combo->addItems( list );
		combo->setCurrentIndex( currentConnectionIndex );
		m_table->setCellWidget( deviceCount, CONNECTION_COL, combo );
		
		
		// m_table->adjustRow(deviceCount);	//&&& use setSizePolicy(QSizePolicy) ? // qt3 desc: Resizes row row so that the row height is tall enough to display the tallest item the row contains.
		
		
		
        ++deviceCount;
    }

    int minColumnWidths[] = { 80, 120, 100, 250 };
    for (int i = 0; i < 4; ++i) {
//         m_table->adjustColumn(i);	//&&&
		
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

    list.append(QObject::tr("No connection"));
}

void
DeviceEditorDialog::setModified(bool m)
{
    if (m_modified == m) return;
	
	m_buttonBox->button( QDialogButtonBox::Ok )->setEnabled( m );
	m_buttonBox->button( QDialogButtonBox::Apply )->setEnabled( m );
	
//     enableButtonOK(m);
//     enableButtonApply(m);
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

        int reply = QMessageBox::question(
                      dynamic_cast<QWidget*>(this),
                      "", /* no title */
                      QObject::tr("Apply pending changes?"),
                      QMessageBox::Yes | QMessageBox::No,
                      QMessageBox::No);

        if (reply == QMessageBox::Yes)
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

    for (int i = 0; i < m_table->rowCount(); ++i) {
        int deviceId = getDeviceIdAt(i);
        if (deviceId < 0) { // new device
            command->addCommand(new CreateOrDeleteDeviceCommand
                                (m_studio,
                                 qstrtostr( m_table->item(i, LABEL_COL)->text() ),
                                 Device::Midi,
								 m_table->item(i, DIRECTION_COL)->text() == "Play" ?
                                 MidiDevice::Play :
                                 MidiDevice::Record,
								 qstrtostr( m_table->item(i, CONNECTION_COL)->text() )  ));
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
				std::string name = qstrtostr(m_table->item(i, LABEL_COL)->text() );
				std::string conn = qstrtostr(m_table->item(i, CONNECTION_COL)->text() );
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

    CommandHistory::getInstance()->addCommand(command);

    m_deletedDevices.clear();

    populate();
    setModified(false);
}

int
DeviceEditorDialog::getDeviceIdAt(int row) // -1 for new device w/o an id yet
{
	QString t( m_table->item(row, 0)->text() );

    QRegExp re("^.*(\\d+).*$");
    re.search(t);

    QString number = re.cap(1);
    int id = -1;

    if ( ! number.isEmpty() )
    {
        id = number.toInt() - 1; // displayed device numbers are 1-based
    }

    return id;
}

void
DeviceEditorDialog::slotAddPlayDevice()
{
    int n = m_table->rowCount();
// 	m_table->insertRows(n, 1);
	m_table->insertRow( n );
	
    m_table->setItem(n, 0, new QTableWidgetItem( QObject::tr("<new device>")));
	m_table->setItem(n, 1, new QTableWidgetItem( QObject::tr("New Device")));
	m_table->setItem(n, 2, new QTableWidgetItem( QObject::tr("Play")));

	/*
    QComboTableItem *item =
        new QComboTableItem(m_table, m_playConnections, false);
    item->setCurrentIndex(m_playConnections.size() - 1);
    m_table->setItem(n, 3, item);
	*/
	QComboBox *combo;
	combo->addItems( m_playConnections );
	combo->setCurrentIndex( m_playConnections.size() - 1 );
	m_table->setCellWidget( n, 3, combo );
	
//     m_table->adjustRow(n);	//&&&

    setModified(true);
}

void
DeviceEditorDialog::slotAddRecordDevice()
{
    int n = m_table->rowCount();
// 	m_table->insertRows(n, 1);
	m_table->insertRow( n );
	
	m_table->setItem(n, 0, new QTableWidgetItem( QObject::tr("<new device>")));
	m_table->setItem(n, 1, new QTableWidgetItem( QObject::tr("New Device")));
	m_table->setItem(n, 2, new QTableWidgetItem( QObject::tr("Record")));

	/*
    QComboTableItem *item =
        new QComboTableItem(m_table, m_recordConnections, false);
    item->setCurrentIndex(m_recordConnections.size() - 1);
    m_table->setItem(n, 3, item);
	*/
	QComboBox *combo;
	combo->addItems( m_playConnections );
	combo->setCurrentIndex( m_playConnections.size() - 1 );
	m_table->setCellWidget( n, 3, combo );
// 	m_table->adjustRow(n);	//&&&
	
	
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
