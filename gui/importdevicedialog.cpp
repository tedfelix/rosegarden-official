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

#include "importdevicedialog.h"

#include <qvbox.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>

#include <kcombobox.h>
#include <kconfig.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <kmessagebox.h>
#include <kio/netaccess.h>

#include "Studio.h"
#include "MidiDevice.h"
#include "MidiProgram.h"

#include "SF2PatchExtractor.h"

#include "constants.h"
#include "rosestrings.h"
#include "rosegardenguidoc.h"

using Rosegarden::SF2PatchExtractor;

ImportDeviceDialog::ImportDeviceDialog(QWidget *parent, KURL url) :
    KDialogBase(parent, "importdevicedialog", true,
                i18n("Import from Device..."),
                Ok | Cancel, Ok),
    m_fileDoc(0),
    m_device(0)
{
    QVBox *mainFrame = makeVBoxMainWidget();

    if (url.isEmpty()) {
	reject();
	return;
    }

    QString target;
    if (KIO::NetAccess::download(url, target) == false) {
        KMessageBox::error(this, QString(i18n("Cannot download file %1"))
                           .arg(url.prettyURL()));
        return;
    }

    bool fileRead = false;
    if (SF2PatchExtractor::isSF2File(target.data())) {
	fileRead = importFromSF2(target);
    } else {
	fileRead = importFromRG(target);
    }
    if (!fileRead) {
	KMessageBox::error
	    (this, i18n("Cannot open file %1").arg(url.prettyURL()));
	reject();
	close();
	return;
    }
    if (m_devices.size() == 0) {
	KMessageBox::sorry
	    (this, i18n("No devices found in file %1").arg(url.prettyURL()));
	reject();
	close();
	return;
    }

    QGroupBox *groupBox = new QGroupBox(2, Qt::Horizontal,
                                        i18n("Source device"),
                                        mainFrame);

    QHBox *deviceBox = new QHBox(groupBox);
    QHBoxLayout *bl = new QHBoxLayout(deviceBox);
    bl->addWidget(new QLabel(i18n("Import from: "), deviceBox));

    bool showRenameOption = false;

    if (m_devices.size() > 1) {
        m_deviceCombo = new KComboBox(deviceBox);
	m_deviceLabel = 0;
	bl->addWidget(m_deviceCombo);
    } else {
	m_deviceCombo = 0;
	m_deviceLabel = new QLabel(deviceBox);
	bl->addWidget(m_deviceLabel);
    }

    bl->addStretch(10);

    int count = 1;
    for (std::vector<Rosegarden::MidiDevice *>::iterator i = m_devices.begin();
	 i != m_devices.end(); ++i) {
	if ((*i)->getName() != "") {
	    showRenameOption = true;
	} else {
	    (*i)->setName(qstrtostr(i18n("Device %1").arg(count)));
	}
	if (m_devices.size() > 1) {
	    m_deviceCombo->insertItem(strtoqstr((*i)->getName()));
	} else {
	    m_deviceLabel->setText(strtoqstr((*i)->getName()));
	}
	++count;
    }

    QHBox *optionsBox = new QHBox(mainFrame);

    QGroupBox *gb = new QGroupBox(1, Horizontal, i18n("Options"),
				  optionsBox);

    m_importBanks = new QCheckBox(i18n("Import banks"), gb);
    m_importControllers = new QCheckBox(i18n("Import controllers"), gb);

    if (showRenameOption) {
	m_rename = new QCheckBox(i18n("Import device name"), gb);
    } else {
	m_rename = 0;
    }

    m_buttonGroup = new QButtonGroup(1, Qt::Horizontal,
                                     i18n("Bank import behaviour"),
                                     optionsBox);
    m_mergeBanks = new QRadioButton(i18n("Merge banks"), m_buttonGroup);
    m_overwriteBanks = new QRadioButton(i18n("Overwrite banks"), m_buttonGroup);

    KConfig *config = kapp->config();
    config->setGroup(Rosegarden::GeneralOptionsConfigGroup);

    m_importBanks->setChecked(config->readBoolEntry("importbanks", true));
    m_importControllers->setChecked(config->readBoolEntry("importcontrollers", true));

    bool rename = config->readBoolEntry("importbanksrename", true);
    if (m_rename) m_rename->setChecked(rename);

    bool overwrite = config->readBoolEntry("importbanksoverwrite", false);
    if (overwrite) m_buttonGroup->setButton(1);
    else m_buttonGroup->setButton(0);
}

ImportDeviceDialog::~ImportDeviceDialog()
{
    if (m_fileDoc) {
	delete m_fileDoc;
    } else {
	delete m_device;
    }
}

void
ImportDeviceDialog::slotOk()
{
    int index = 0;
    if (m_deviceCombo) index = m_deviceCombo->currentItem();
    m_device = m_devices[index];

    int v = m_buttonGroup->id(m_buttonGroup->selected());
    KConfig *config = kapp->config();
    config->setGroup(Rosegarden::GeneralOptionsConfigGroup);
    config->writeEntry("importbanksoverwrite", v == 1);
    if (m_rename) config->writeEntry("importbanksrename", m_rename->isChecked());
    accept();
}

void
ImportDeviceDialog::slotCancel()
{
    reject();
}

std::string
ImportDeviceDialog::getDeviceName() const
{
    return m_device->getName();
}

const Rosegarden::BankList &
ImportDeviceDialog::getBanks() const
{
    return m_device->getBanks();
}

const Rosegarden::ProgramList &
ImportDeviceDialog::getPrograms() const
{
    return m_device->getPrograms();
}

const Rosegarden::ControlList &
ImportDeviceDialog::getControllers() const
{
    return m_device->getControlParameters();
}

std::string
ImportDeviceDialog::getLibrarianName() const
{
    return m_device->getLibrarianName();
}

std::string
ImportDeviceDialog::getLibrarianEmail() const
{
    return m_device->getLibrarianEmail();
}

Rosegarden::MidiDevice::VariationType
ImportDeviceDialog::getVariationType() const
{
    return m_device->getVariationType();
}

bool
ImportDeviceDialog::shouldImportBanks() const
{
    return m_importBanks->isChecked();
}

bool
ImportDeviceDialog::shouldImportControllers() const
{
    return m_importControllers->isChecked();
}

bool
ImportDeviceDialog::shouldOverwriteBanks() const
{
    return m_buttonGroup->id(m_buttonGroup->selected()) != 0;
}

bool
ImportDeviceDialog::shouldRename() const
{
    return m_rename ? m_rename->isChecked() : false;
}

bool
ImportDeviceDialog::importFromRG(QString fileName)
{
    m_fileDoc = new RosegardenGUIDoc(this, 0, true); // skipAutoload

    // Add some dummy devices for bank population when we open the document.
    // We guess that the file won't have more than 16 devices.
    //
    for (unsigned int i = 0; i < 16; i++) {
//        QString label = QString("MIDI Device %1").arg(i + 1);
        m_fileDoc->getStudio().addDevice("", i, Rosegarden::Device::Midi);
    }

    if (!m_fileDoc->openDocument(fileName, false)) {
	return false;
    }

    m_devices.clear();

    Rosegarden::DeviceList *list = m_fileDoc->getStudio().getDevices();
    if (list->size() == 0) {
	return true; // true because we successfully read the document
    }

    for (Rosegarden::DeviceListIterator it = list->begin();
	 it != list->end(); ++it) {

	Rosegarden::MidiDevice *device = 
	    dynamic_cast<Rosegarden::MidiDevice*>(*it);

	if (device) {
	    std::vector<Rosegarden::MidiBank> banks =
		device->getBanks();

	    // We've got a bank on a Device fom this file
	    //
	    if (banks.size()) m_devices.push_back(device);
	}
    }

    return true;
}

bool
ImportDeviceDialog::importFromSF2(QString filename)
{
    SF2PatchExtractor::Device sf2device;
    try {
        sf2device = SF2PatchExtractor::read(filename.data());

    // These exceptions shouldn't happen -- the isSF2File call before this
    // one should have weeded them out
    } catch (SF2PatchExtractor::FileNotFoundException e) {
        return false;
    } catch (SF2PatchExtractor::WrongFileFormatException e) {
        return false;
    }

    std::vector<Rosegarden::MidiBank> banks;
    std::vector<Rosegarden::MidiProgram> programs;

    for (SF2PatchExtractor::Device::const_iterator i = sf2device.begin();
         i != sf2device.end(); ++i) {

        int bankNumber = i->first;
        const SF2PatchExtractor::Bank &sf2bank = i->second;

	int msb = bankNumber / 128;
	int lsb = bankNumber % 128;

        Rosegarden::MidiBank bank
	    (false, msb, lsb, qstrtostr(i18n("Bank %1:%2").arg(msb).arg(lsb)));

        banks.push_back(bank);

        for (SF2PatchExtractor::Bank::const_iterator j = sf2bank.begin();
             j != sf2bank.end(); ++j) {

            Rosegarden::MidiProgram program(bank, j->first, j->second);
            programs.push_back(program);
        }
    }

    Rosegarden::MidiDevice *device = new Rosegarden::MidiDevice
	(0, "", Rosegarden::MidiDevice::Play);
    device->replaceBankList(banks);
    device->replaceProgramList(programs);
    m_devices.push_back(device);

    return true;
}

