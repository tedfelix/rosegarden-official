/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "ImportDeviceDialog.h"

#include <klocale.h>
#include "misc/Strings.h"
#include "document/ConfigGroups.h"
#include "base/MidiDevice.h"
#include "base/MidiProgram.h"
#include "document/RosegardenGUIDoc.h"
#include <kcombobox.h>
#include <kconfig.h>
#include <kdialogbase.h>
#include <kmessagebox.h>
#include <kurl.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qstring.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

ImportDeviceDialog::ImportDeviceDialog(QWidget *parent, KURL url) :
        KDialogBase(parent, "importdevicedialog", true,
                    i18n("Import from Device..."),
                    Ok | Cancel, Ok),
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
    QVBox *mainFrame = makeVBoxMainWidget();

    if (m_url.isEmpty()) {
        reject();
        return false;
    }

    QString target;
    if (KIO::NetAccess::download(m_url, target) == false) {
        KMessageBox::error(this, i18n("Cannot download file %1").arg(m_url.prettyURL()));
        return false;
    }

    bool fileRead = false;
    if (SF2PatchExtractor::isSF2File(target.data())) {
        fileRead = importFromSF2(target);
    } else {
        fileRead = importFromRG(target);
    }
    if (!fileRead) {
        KMessageBox::error
        (this, i18n("Cannot open file %1").arg(m_url.prettyURL()));
        reject();
        close();
        return false;
    }
    if (m_devices.size() == 0) {
        KMessageBox::sorry
        (this, i18n("No devices found in file %1").arg(m_url.prettyURL()));
        reject();
        close();
        return false;
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
    for (std::vector<MidiDevice *>::iterator i = m_devices.begin();
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
    m_importKeyMappings = new QCheckBox(i18n("Import key mappings"), gb);
    m_importControllers = new QCheckBox(i18n("Import controllers"), gb);

    if (showRenameOption) {
        m_rename = new QCheckBox(i18n("Import device name"), gb);
    } else {
        m_rename = 0;
    }

    m_buttonGroup = new QButtonGroup(1, Qt::Horizontal,
                                     i18n("Bank import behavior"),
                                     optionsBox);
    m_mergeBanks = new QRadioButton(i18n("Merge banks"), m_buttonGroup);
    m_overwriteBanks = new QRadioButton(i18n("Overwrite banks"), m_buttonGroup);

    KConfig *config = kapp->config();
    config->setGroup(GeneralOptionsConfigGroup);

    m_importBanks->setChecked(config->readBoolEntry("importbanks", true));
    m_importKeyMappings->setChecked(config->readBoolEntry("importkeymappings", true));
    m_importControllers->setChecked(config->readBoolEntry("importcontrollers", true));

    bool rename = config->readBoolEntry("importbanksrename", true);
    if (m_rename)
        m_rename->setChecked(rename);

    bool overwrite = config->readBoolEntry("importbanksoverwrite", true);
    if (overwrite)
        m_buttonGroup->setButton(1);
    else
        m_buttonGroup->setButton(0);

    return true;
}

void
ImportDeviceDialog::slotOk()
{
    int index = 0;
    if (m_deviceCombo)
        index = m_deviceCombo->currentItem();
    m_device = m_devices[index];

    int v = m_buttonGroup->id(m_buttonGroup->selected());
    KConfig *config = kapp->config();
    config->setGroup(GeneralOptionsConfigGroup);
    config->writeEntry("importbanksoverwrite", v == 1);
    if (m_rename)
        config->writeEntry("importbanksrename", m_rename->isChecked());
    accept();
}

void
ImportDeviceDialog::slotCancel()
{
    reject();
}

ImportDeviceDialog::getDeviceName() const
{
    return m_device->getName();
}

ImportDeviceDialog::getBanks() const
{
    return m_device->getBanks();
}

ImportDeviceDialog::getPrograms() const
{
    return m_device->getPrograms();
}

ImportDeviceDialog::getKeyMappings() const
{
    return m_device->getKeyMappings();
}

ImportDeviceDialog::getControllers() const
{
    return m_device->getControlParameters();
}

ImportDeviceDialog::getLibrarianName() const
{
    return m_device->getLibrarianName();
}

ImportDeviceDialog::getLibrarianEmail() const
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
         qstrtostr(i18n("Bank %1:%2").arg(msb).arg(lsb)));

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
