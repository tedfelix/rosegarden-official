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


#include "ImportDeviceDialog.h"
#include <QLayout>
#include <QApplication>

#include <klocale.h>
#include "misc/Strings.h"
#include "document/ConfigGroups.h"
#include "base/MidiDevice.h"
#include "base/MidiProgram.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/application/RosegardenGUIApp.h"
#include "sound/SF2PatchExtractor.h"
#include "gui/general/FileSource.h"
#include <QComboBox>
#include <QSettings>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QGroupBox>
#include <QCheckBox>
#include <QLabel>
#include <QRadioButton>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QButtonGroup>


namespace Rosegarden
{

ImportDeviceDialog::ImportDeviceDialog(QWidget *parent, QUrl url) :
        QDialog(parent),
        m_url(url),
        m_fileDoc(0),
        m_device(0)
{}

ImportDeviceDialog::~ImportDeviceDialog()
{
    if (m_fileDoc) {
        delete m_fileDoc;
    } else {
        delete m_device;
    }
}

bool
ImportDeviceDialog::doImport()
{
    setModal(true);
    setWindowTitle(i18n("Import from Device..."));
    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *mainFrame = new QWidget(this);
    QVBoxLayout *mainFrameLayout = new QVBoxLayout;
    metagrid->addWidget(mainFrame, 0, 0);

    if (m_url.isEmpty()) {
        reject();
        return false;
    }

    QString target;
    FileSource source(m_url);
    if (!source.isAvailable()) {
        QMessageBox::critical(this, "", i18n("Cannot download file %1", m_url.toString()));
        return false;
    }

    source.waitForData();
    target = source.getLocalFilename();
    std::string filename = qStrToCharPtrLocal8(target);

    bool fileRead = false;
    if (SF2PatchExtractor::isSF2File(filename)) {
        fileRead = importFromSF2(target);
    } else {
        fileRead = importFromRG(target);
    }
    if (!fileRead) {
        QMessageBox::critical
            (this, i18n("Cannot open file %1", m_url.toString()));
        reject();
        close();
        return false;
    }
    if (m_devices.size() == 0) {
        /* was sorry */ QMessageBox::warning
            (this, i18n("No devices found in file %1", m_url.toString()));
        reject();
        close();
        return false;
    }

    QGroupBox *groupBox = new QGroupBox(i18n("Source device"));
    QHBoxLayout *groupBoxLayout = new QHBoxLayout;
    mainFrameLayout->addWidget(groupBox);

    QWidget *deviceBox = new QWidget(groupBox);
    QHBoxLayout *deviceBoxLayout = new QHBoxLayout( deviceBox );
    groupBoxLayout->addWidget(deviceBox);

    deviceBoxLayout->addWidget(new QLabel(i18n("Import from: "), deviceBox));

    bool showRenameOption = false;

    if (m_devices.size() > 1) {
        m_deviceLabel = 0;
        m_deviceCombo = new QComboBox( deviceBox );
        deviceBoxLayout->addWidget(m_deviceCombo);
    } else {
        m_deviceCombo = 0;
        m_deviceLabel = new QLabel( deviceBox );
        deviceBoxLayout->addWidget(m_deviceLabel);
    }

    deviceBoxLayout->addStretch(10);

    int count = 1;
    for (std::vector<MidiDevice *>::iterator i = m_devices.begin();
            i != m_devices.end(); ++i) {
        if ((*i)->getName() != "") {
            showRenameOption = true;
        } else {
            (*i)->setName(qstrtostr(i18n("Device %1", count)));
        }
        if (m_devices.size() > 1) {
            m_deviceCombo->addItem(strtoqstr((*i)->getName()));
        } else {
            m_deviceLabel->setText(strtoqstr((*i)->getName()));
        }
        ++count;
    }

    QWidget *optionsBox = new QWidget(mainFrame);
    QHBoxLayout *optionsBoxLayout = new QHBoxLayout;
    mainFrameLayout->addWidget(optionsBox);

    QGroupBox *gb = new QGroupBox(i18n("Options"));
    QHBoxLayout *gbLayout = new QHBoxLayout;
    optionsBoxLayout->addWidget(gb);

    m_importBanks = new QCheckBox(i18n("Import banks"), gb);
    gbLayout->addWidget(m_importBanks);
    m_importKeyMappings = new QCheckBox(i18n("Import key mappings"), gb);
    gbLayout->addWidget(m_importKeyMappings);
    m_importControllers = new QCheckBox(i18n("Import controllers"), gb);
    gbLayout->addWidget(m_importControllers);

    if (showRenameOption) {
        m_rename = new QCheckBox(i18n("Import device name"), gb);
        gbLayout->addWidget(m_rename);
    } else {
        m_rename = 0;
    }

    QGroupBox *buttonGroupBox = new QGroupBox(i18n("Bank import behavior"));
    QHBoxLayout *buttonGroupBoxLayout = new QHBoxLayout;
    optionsBoxLayout->addWidget(buttonGroupBox);
    m_buttonGroup = new QButtonGroup(buttonGroupBox);

    m_mergeBanks = new QRadioButton(i18n("Merge banks"));
    buttonGroupBoxLayout->addWidget(m_mergeBanks);
    m_buttonGroup->addButton(m_mergeBanks, 0);

    m_overwriteBanks = new QRadioButton(i18n("Overwrite banks"));
    buttonGroupBoxLayout->addWidget(m_overwriteBanks);
    m_buttonGroup->addButton(m_overwriteBanks, 1);

    gb->setLayout(gbLayout);
    buttonGroupBox->setLayout(buttonGroupBoxLayout);
    optionsBox->setLayout(optionsBoxLayout);
    deviceBox->setLayout(deviceBoxLayout);
    groupBox->setLayout(groupBoxLayout);
    mainFrame->setLayout(mainFrameLayout);

    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    m_importBanks->setChecked( qStrToBool( settings.value("importbanks", "true" ) ) );
    m_importKeyMappings->setChecked( qStrToBool( settings.value("importkeymappings", "true" ) ) );
    m_importControllers->setChecked( qStrToBool( settings.value("importcontrollers", "true" ) ) );

    bool rename = qStrToBool( settings.value("importbanksrename", "true" ) ) ;
    if (m_rename)
        m_rename->setChecked(rename);

    bool overwrite = qStrToBool( settings.value("importbanksoverwrite", "true" ) ) ;
    if (overwrite)
        m_buttonGroup->button(1)->setChecked(true);
    else
        m_buttonGroup->button(0)->setChecked(true);

    settings.endGroup();

    QDialogButtonBox *buttonBox
        = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(slotOk()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(slotCancel()));

    return true;
}

void
ImportDeviceDialog::slotOk()
{
    int index = 0;
    if (m_deviceCombo)
        index = m_deviceCombo->currentIndex();
    m_device = m_devices[index];

    int v = m_buttonGroup->checkedId();
    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    settings.setValue("importbanksoverwrite", v == 1);
    if (m_rename)
        settings.setValue("importbanksrename", m_rename->isChecked());
    accept();

    settings.endGroup();
}

void
ImportDeviceDialog::slotCancel()
{
    reject();
}

std::string  ImportDeviceDialog::getDeviceName() const
{
    return m_device->getName();
}

const BankList& ImportDeviceDialog::getBanks() const
{
    return m_device->getBanks();
}

const ProgramList& ImportDeviceDialog::getPrograms() const
{
    return m_device->getPrograms();
}

const KeyMappingList& ImportDeviceDialog::getKeyMappings() const
{
    return m_device->getKeyMappings();
}

const ControlList& ImportDeviceDialog::getControllers() const
{
    return m_device->getControlParameters();
}

std::string ImportDeviceDialog::getLibrarianName() const
{
    return m_device->getLibrarianName();
}

std::string ImportDeviceDialog::getLibrarianEmail() const
{
    return m_device->getLibrarianEmail();
}

MidiDevice::VariationType
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
ImportDeviceDialog::shouldImportKeyMappings() const
{
    return m_importKeyMappings->isChecked();
}

bool
ImportDeviceDialog::shouldImportControllers() const
{
    return m_importControllers->isChecked();
}

bool
ImportDeviceDialog::shouldOverwriteBanks() const
{
    return m_buttonGroup->checkedId() != 0;
}

bool
ImportDeviceDialog::shouldRename() const
{
    return m_rename ? m_rename->isChecked() : false;
}

bool
ImportDeviceDialog::importFromRG(QString fileName)
{
    m_fileDoc = new RosegardenGUIDoc(RosegardenGUIApp::self(), 0, true); // skipAutoload

    // Add some dummy devices for bank population when we open the document.
    // We guess that the file won't have more than 32 devices.
    //
    //    for (unsigned int i = 0; i < 32; i++) {
    //        m_fileDoc->getStudio().addDevice("", i, Device::Midi);
    //    }

    if (!m_fileDoc->openDocument(fileName, false)) {
        return false;
    }

    m_devices.clear();

    DeviceList *list = m_fileDoc->getStudio().getDevices();
    if (list->size() == 0) {
        return true; // true because we successfully read the document
    }

    for (DeviceListIterator it = list->begin();
            it != list->end(); ++it) {

        MidiDevice *device =
            dynamic_cast<MidiDevice*>(*it);

        if (device) {
            std::vector<MidiBank> banks =
                device->getBanks();

            // DMM - check for controllers too, because some users have
            // created .rgd files that contain only controllers
            // see bug #1183522
            //
            std::vector<ControlParameter> controllers =
                device->getControlParameters();

            // We've got a bank on a Device fom this file
            // (or a device that contains controllers or key mappings)
            //
            if (banks.size() ||
                    controllers.size() ||
                    device->getKeyMappings().size())
                m_devices.push_back(device);
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

    std::vector<MidiBank> banks;
    std::vector<MidiProgram> programs;

    for (SF2PatchExtractor::Device::const_iterator i = sf2device.begin();
            i != sf2device.end(); ++i) {

        int bankNumber = i->first;
        const SF2PatchExtractor::Bank &sf2bank = i->second;

        int msb = bankNumber / 128;
        int lsb = bankNumber % 128;

        MidiBank bank
        (msb == 1, msb, lsb,
         qstrtostr(i18n("Bank %1:%2", msb, lsb)));

        banks.push_back(bank);

        for (SF2PatchExtractor::Bank::const_iterator j = sf2bank.begin();
                j != sf2bank.end(); ++j) {

            MidiProgram program(bank, j->first, j->second);
            programs.push_back(program);
        }
    }

    MidiDevice *device = new MidiDevice
                         (0, "", MidiDevice::Play);
    device->replaceBankList(banks);
    device->replaceProgramList(programs);
    m_devices.push_back(device);

    return true;
}

}
#include "ImportDeviceDialog.moc"
