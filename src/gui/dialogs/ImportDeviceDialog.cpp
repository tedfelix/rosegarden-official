/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ImportDeviceDialog.h"

#include "misc/Strings.h"
#include "misc/ConfigGroups.h"
#include "base/MidiDevice.h"
#include "base/MidiProgram.h"
#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenMainWindow.h"
#include "sound/SF2PatchExtractor.h"
#include "sound/LSCPPatchExtractor.h"
#include "gui/general/FileSource.h"

#include <QLayout>
#include <QApplication>
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

#include <string>

namespace Rosegarden
{

ImportDeviceDialog::ImportDeviceDialog(QWidget *parent, QUrl url) :
    QDialog(parent),
    m_url(url),
    m_device(0)
{}

ImportDeviceDialog::~ImportDeviceDialog()
{
    delete m_device;
    for (int i = 0; i < (int)m_devices.size(); ++i) delete m_devices[i];
}

bool
ImportDeviceDialog::doImport()
{
    setModal(true);
    setWindowTitle(tr("Import from Device..."));
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
        QMessageBox::critical(
          dynamic_cast<QWidget*>(this),
          "", /* no title */
          tr("Cannot download file %1").arg(m_url.toString()),
          QMessageBox::Ok,
          QMessageBox::Ok);
        return false;
    }

    source.waitForData();
    target = source.getLocalFilename();
    std::string filename = qStrToStrLocal8(target);

    bool fileRead = false;
    if (SF2PatchExtractor::isSF2File(filename)) {
        fileRead = importFromSF2(target);
    } else if (LSCPPatchExtractor::isLSCPFile(target)) {
        fileRead = importFromLSCP(target);
    } else {
        fileRead = importFromRG(target);
    }
    if (!fileRead) {
        QMessageBox::critical(
          dynamic_cast<QWidget*>(this),
          "", /* no title */
          tr("Cannot open file %1").arg(m_url.toString()),
          QMessageBox::Ok,
          QMessageBox::Ok);
        reject();
        close();
        return false;
    }
    if (m_devices.size() == 0) {
        QMessageBox::warning(
          dynamic_cast<QWidget*>(this),
          "", /* no title */
          tr("No devices found in file %1").arg(m_url.toString()),
          QMessageBox::Ok,
          QMessageBox::Ok);
        reject();
        close();
        return false;
    }

    QGroupBox *groupBox = new QGroupBox(tr("Source device"));
    QHBoxLayout *groupBoxLayout = new QHBoxLayout;
    mainFrameLayout->addWidget(groupBox);

    QWidget *deviceBox = new QWidget(groupBox);
    QHBoxLayout *deviceBoxLayout = new QHBoxLayout( deviceBox );
    groupBoxLayout->addWidget(deviceBox);

    deviceBoxLayout->addWidget(new QLabel(tr("Import from: "), deviceBox));

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
            (*i)->setName(qstrtostr(tr("Device %1").arg(count)));
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

    QGroupBox *gb = new QGroupBox(tr("Options"));
    QVBoxLayout *gbLayout = new QVBoxLayout;
    optionsBoxLayout->addWidget(gb);

    m_importBanks = new QCheckBox(tr("Import banks"), gb);
    gbLayout->addWidget(m_importBanks);
    m_importKeyMappings = new QCheckBox(tr("Import key mappings"), gb);
    gbLayout->addWidget(m_importKeyMappings);
    m_importControllers = new QCheckBox(tr("Import controllers"), gb);
    gbLayout->addWidget(m_importControllers);

    if (showRenameOption) {
        m_rename = new QCheckBox(tr("Import device name"), gb);
        gbLayout->addWidget(m_rename);
    } else {
        m_rename = 0;
    }

    QGroupBox *buttonGroupBox = new QGroupBox(tr("Bank import behavior"));
    QVBoxLayout *buttonGroupBoxLayout = new QVBoxLayout;
    optionsBoxLayout->addWidget(buttonGroupBox);
    m_buttonGroup = new QButtonGroup(buttonGroupBox);

    m_mergeBanks = new QRadioButton(tr("Merge banks"));
    buttonGroupBoxLayout->addWidget(m_mergeBanks);
    m_buttonGroup->addButton(m_mergeBanks, 0);

    m_overwriteBanks = new QRadioButton(tr("Overwrite banks"));
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
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(slotCancel()));

    return true;
}

void
ImportDeviceDialog::accept()
{
    int index = 0;
    if (m_deviceCombo) index = m_deviceCombo->currentIndex();
    if ((int)m_devices.size() > index) {
        m_device = new MidiDevice(*m_devices[index]);
    }

    int v = m_buttonGroup->checkedId();
    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    settings.setValue("importbanksoverwrite", v == 1);
    if (m_rename) settings.setValue("importbanksrename", m_rename->isChecked());
    settings.endGroup();

    QDialog::accept();
}

void
ImportDeviceDialog::slotCancel()
{
    reject();
}

bool ImportDeviceDialog::haveDevice() const
{
    return m_device;
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
    bool skipAutoload = true, clearCommandHistory = false;
    RosegardenDocument fileDoc(RosegardenMainWindow::self(), 0, 
                               skipAutoload, clearCommandHistory);

    if (!fileDoc.openDocument(fileName, false)) {
        return false;
    }

    for (int i = 0; i < (int)m_devices.size(); ++i) delete m_devices[i];
    m_devices.clear();

    DeviceList *list = fileDoc.getStudio().getDevices();
    if (list->size() == 0) {
        return true; // true because we successfully read the document
    }

    for (DeviceListIterator it = list->begin(); it != list->end(); ++it) {

        MidiDevice *device = dynamic_cast<MidiDevice*>(*it);

        if (device) {
            std::vector<MidiBank> banks = device->getBanks();

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
                device->getKeyMappings().size()) {
                m_devices.push_back(new MidiDevice(*device));
            }
        }
    }

    return true;
}

bool
ImportDeviceDialog::importFromSF2(QString filename)
{
    SF2PatchExtractor::Device sf2device;
    try {
        sf2device = SF2PatchExtractor::read( qstrtostr(filename) );

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
         qstrtostr(tr("Bank %1:%2").arg(msb).arg(lsb)));

        banks.push_back(bank);

        for (SF2PatchExtractor::Bank::const_iterator j = sf2bank.begin();
                j != sf2bank.end(); ++j) {

            MidiProgram program(bank, j->first, j->second);
            programs.push_back(program);
        }
    }

    // This is a temporary device, so we can use device and instrument
    // IDs that other devices in the Studio may also be using without
    // expecting any problems
    MidiDevice *device = new MidiDevice
        (0, MidiInstrumentBase, "", MidiDevice::Play);
    device->replaceBankList(banks);
    device->replaceProgramList(programs);
    m_devices.push_back(device);

    return true;
}

bool
ImportDeviceDialog::importFromLSCP(QString filename)
{
    LSCPPatchExtractor::Device lscpDevice;

    lscpDevice = LSCPPatchExtractor::extractContent(filename);
    std::vector<MidiBank> banks;
    std::vector<MidiProgram> programs;

    int comparableBankNumber = -1; //Make sure that first bank is read too by comparing to -1 first (invalid bank number)

    for (LSCPPatchExtractor::Device::const_iterator i = lscpDevice.begin();
    i != lscpDevice.end(); ++i) {

        int bankNumber = (*i).bankNumber; //Local variable bankNumber gets value from struct's member bankNumber

        std::string bankName = (*i).bankName; //Local variable bankName gets value from struct's member bankName
        int msb = bankNumber / 128;
        int lsb = bankNumber % 128;

        MidiBank bank (msb == 1, msb, lsb, bankName);

        if (comparableBankNumber != bankNumber) {
            banks.push_back(bank);
            comparableBankNumber = bankNumber;
        }

        MidiProgram program(bank, (*i).programNumber, (*i).programName);
        programs.push_back(program);
    }

    // This is a temporary device, so we can use device and instrument
    // IDs that other devices in the Studio may also be using without
    // expecting any problems
    MidiDevice *device = new MidiDevice
        (0, MidiInstrumentBase, "", MidiDevice::Play);
    device->replaceBankList(banks);
    device->replaceProgramList(programs);
    m_devices.push_back(device);

    return true;
}

}
