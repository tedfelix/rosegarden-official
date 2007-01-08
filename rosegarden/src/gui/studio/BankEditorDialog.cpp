/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
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


#include "BankEditorDialog.h"
#include <qlayout.h>
#include <kapplication.h>

#include <klocale.h>
#include <kstddirs.h>
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Device.h"
#include "base/MidiDevice.h"
#include "base/MidiProgram.h"
#include "base/NotationTypes.h"
#include "base/Studio.h"
#include "commands/studio/ModifyDeviceCommand.h"
#include "document/MultiViewCommandHistory.h"
#include "document/RosegardenGUIDoc.h"
#include "document/ConfigGroups.h"
#include "gui/dialogs/ExportDeviceDialog.h"
#include "gui/dialogs/ImportDeviceDialog.h"
#include "MidiBankListViewItem.h"
#include "MidiDeviceListViewItem.h"
#include "MidiKeyMapListViewItem.h"
#include "MidiKeyMappingEditor.h"
#include "MidiProgramsEditor.h"
#include <kaction.h>
#include <kcombobox.h>
#include <kcommand.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <klistview.h>
#include <kmainwindow.h>
#include <kmessagebox.h>
#include <kstdaccel.h>
#include <kstdaction.h>
#include <kxmlguiclient.h>
#include <qcheckbox.h>
#include <qdialog.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qframe.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qpushbutton.h>
#include <qsizepolicy.h>
#include <qsplitter.h>
#include <qstring.h>
#include <qtooltip.h>
#include <qvbox.h>
#include <qvgroupbox.h>
#include <qwidget.h>


namespace Rosegarden
{

BankEditorDialog::BankEditorDialog(QWidget *parent,
                                   RosegardenGUIDoc *doc,
                                   DeviceId defaultDevice):
        KMainWindow(parent, "bankeditordialog"),
        m_studio(&doc->getStudio()),
        m_doc(doc),
        m_copyBank(Device::NO_DEVICE, -1),
        m_modified(false),
        m_keepBankList(false),
        m_deleteAllReally(false),
        m_lastDevice(Device::NO_DEVICE),
        m_updateDeviceList(false)
{
    QVBox* mainFrame = new QVBox(this);
    setCentralWidget(mainFrame);

    setCaption(i18n("Manage MIDI Banks and Programs"));

    QSplitter* splitter = new QSplitter(mainFrame);

    QFrame* btnBox = new QFrame(mainFrame);

    btnBox->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));

    QHBoxLayout* layout = new QHBoxLayout(btnBox, 4, 10);

    m_closeButton = new QPushButton(btnBox);
    m_applyButton = new QPushButton(i18n("Apply"), btnBox);
    m_resetButton = new QPushButton(i18n("Reset"), btnBox);

    layout->addStretch(10);
    layout->addWidget(m_applyButton);
    layout->addWidget(m_resetButton);
    layout->addSpacing(15);
    layout->addWidget(m_closeButton);
    layout->addSpacing(5);

    connect(m_applyButton, SIGNAL(clicked()),
            this, SLOT(slotApply()));
    connect(m_resetButton, SIGNAL(clicked()),
            this, SLOT(slotReset()));

    //
    // Left-side list view
    //
    QVBox* leftPart = new QVBox(splitter);
    m_listView = new KListView(leftPart);
    m_listView->addColumn(i18n("MIDI Device"));
    m_listView->addColumn(i18n("Type"));
    m_listView->addColumn(i18n("MSB"));
    m_listView->addColumn(i18n("LSB"));
    m_listView->setRootIsDecorated(true);
    m_listView->setShowSortIndicator(true);
    m_listView->setItemsRenameable(true);
    m_listView->restoreLayout(kapp->config(), BankEditorConfigGroup);

    QFrame *bankBox = new QFrame(leftPart);
    QGridLayout *gridLayout = new QGridLayout(bankBox, 4, 2, 6, 6);

    m_addBank = new QPushButton(i18n("Add Bank"), bankBox);
    m_addKeyMapping = new QPushButton(i18n("Add Key Mapping"), bankBox);
    m_delete = new QPushButton(i18n("Delete"), bankBox);
    m_deleteAll = new QPushButton(i18n("Delete All"), bankBox);
    gridLayout->addWidget(m_addBank, 0, 0);
    gridLayout->addWidget(m_addKeyMapping, 0, 1);
    gridLayout->addWidget(m_delete, 1, 0);
    gridLayout->addWidget(m_deleteAll, 1, 1);

    // Tips
    //
    QToolTip::add
        (m_addBank,
                i18n("Add a Bank to the current device"));

    QToolTip::add
        (m_addKeyMapping,
                i18n("Add a Percussion Key Mapping to the current device"));

    QToolTip::add
        (m_delete,
                i18n("Delete the current Bank or Key Mapping"));

    QToolTip::add
        (m_deleteAll,
                i18n("Delete all Banks and Key Mappings from the current Device"));

    m_importBanks = new QPushButton(i18n("Import..."), bankBox);
    m_exportBanks = new QPushButton(i18n("Export..."), bankBox);
    gridLayout->addWidget(m_importBanks, 2, 0);
    gridLayout->addWidget(m_exportBanks, 2, 1);

    // Tips
    //
    QToolTip::add
        (m_importBanks,
                i18n("Import Bank and Program data from a Rosegarden file to the current Device"));
    QToolTip::add
        (m_exportBanks,
                i18n("Export all Device and Bank information to a Rosegarden format  interchange file"));

    m_copyPrograms = new QPushButton(i18n("Copy"), bankBox);
    m_pastePrograms = new QPushButton(i18n("Paste"), bankBox);
    gridLayout->addWidget(m_copyPrograms, 3, 0);
    gridLayout->addWidget(m_pastePrograms, 3, 1);

    // Tips
    //
    QToolTip::add
        (m_copyPrograms,
                i18n("Copy all Program names from current Bank to clipboard"));

    QToolTip::add
        (m_pastePrograms,
                i18n("Paste Program names from clipboard to current Bank"));

    connect(m_listView, SIGNAL(currentChanged(QListViewItem*)),
            this, SLOT(slotPopulateDevice(QListViewItem*)));

    QFrame *vbox = new QFrame(splitter);
    QVBoxLayout *vboxLayout = new QVBoxLayout(vbox, 8, 6);

    m_programEditor = new MidiProgramsEditor(this, vbox);
    vboxLayout->addWidget(m_programEditor);

    m_keyMappingEditor = new MidiKeyMappingEditor(this, vbox);
    vboxLayout->addWidget(m_keyMappingEditor);
    m_keyMappingEditor->hide();

    m_programEditor->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred));
    m_keyMappingEditor->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred));

    m_optionBox = new QVGroupBox(i18n("Options"), vbox);
    vboxLayout->addWidget(m_optionBox);

    QHBox *variationBox = new QHBox(m_optionBox);
    m_variationToggle = new QCheckBox(i18n("Show Variation list based on "), variationBox);
    m_variationCombo = new KComboBox(variationBox);
    m_variationCombo->insertItem(i18n("LSB"));
    m_variationCombo->insertItem(i18n("MSB"));

    // device/bank modification
    connect(m_listView, SIGNAL(itemRenamed (QListViewItem*, const QString&, int)),
            this, SLOT(slotModifyDeviceOrBankName(QListViewItem*, const QString&, int)));

    connect(m_addBank, SIGNAL(clicked()),
            this, SLOT(slotAddBank()));

    connect(m_addKeyMapping, SIGNAL(clicked()),
            this, SLOT(slotAddKeyMapping()));

    connect(m_delete, SIGNAL(clicked()),
            this, SLOT(slotDelete()));

    connect(m_deleteAll, SIGNAL(clicked()),
            this, SLOT(slotDeleteAll()));

    connect(m_importBanks, SIGNAL(clicked()),
            this, SLOT(slotImport()));

    connect(m_exportBanks, SIGNAL(clicked()),
            this, SLOT(slotExport()));

    connect(m_copyPrograms, SIGNAL(clicked()),
            this, SLOT(slotEditCopy()));

    connect(m_pastePrograms, SIGNAL(clicked()),
            this, SLOT(slotEditPaste()));

    connect(m_variationToggle, SIGNAL(clicked()),
            this, SLOT(slotVariationToggled()));

    connect(m_variationCombo, SIGNAL(activated(int)),
            this, SLOT(slotVariationChanged(int)));

    setupActions();

    m_doc->getCommandHistory()->attachView(actionCollection());
    connect(m_doc->getCommandHistory(), SIGNAL(commandExecuted()),
            this, SLOT(slotUpdate()));

    // Initialise the dialog
    //
    initDialog();
    setModified(false);

    // Check for no Midi devices and disable everything
    //
    DeviceList *devices = m_studio->getDevices();
    DeviceListIterator it;
    bool haveMidiPlayDevice = false;
    for (it = devices->begin(); it != devices->end(); ++it) {
        MidiDevice *md =
            dynamic_cast<MidiDevice *>(*it);
        if (md && md->getDirection() == MidiDevice::Play) {
            haveMidiPlayDevice = true;
            break;
        }
    }
    if (!haveMidiPlayDevice) {
        leftPart->setDisabled(true);
        m_programEditor->setDisabled(true);
        m_keyMappingEditor->setDisabled(true);
        m_optionBox->setDisabled(true);
    }

    if (defaultDevice != Device::NO_DEVICE) {
        setCurrentDevice(defaultDevice);
    }

    setAutoSaveSettings(BankEditorConfigGroup, true);
}

BankEditorDialog::~BankEditorDialog()
{
    RG_DEBUG << "~BankEditorDialog()\n";

    m_listView->saveLayout(kapp->config(), BankEditorConfigGroup);

    if (m_doc) // see slotFileClose() for an explanation on why we need to test m_doc
        m_doc->getCommandHistory()->detachView(actionCollection());
}

void
BankEditorDialog::setupActions()
{
    KAction* close = KStdAction::close (this, SLOT(slotFileClose()), actionCollection());

    m_closeButton->setText(close->text());
    connect(m_closeButton, SIGNAL(clicked()),
            this, SLOT(slotFileClose()));

    KStdAction::copy (this, SLOT(slotEditCopy()), actionCollection());
    KStdAction::paste (this, SLOT(slotEditPaste()), actionCollection());

    // some adjustments


    new KToolBarPopupAction(i18n("Und&o"),
                            "undo",
                            KStdAccel::key(KStdAccel::Undo),
                            actionCollection(),
                            KStdAction::stdName(KStdAction::Undo));

    new KToolBarPopupAction(i18n("Re&do"),
                            "redo",
                            KStdAccel::key(KStdAccel::Redo),
                            actionCollection(),
                            KStdAction::stdName(KStdAction::Redo));

    createGUI("bankeditor.rc");
}

void
BankEditorDialog::initDialog()
{
    // Clear down
    //
    m_deviceNameMap.clear();
    m_listView->clear();

    // Fill list view
    //
    DeviceList *devices = m_studio->getDevices();
    DeviceListIterator it;

    for (it = devices->begin(); it != devices->end(); ++it) {
        if ((*it)->getType() == Device::Midi) {
            MidiDevice* midiDevice =
                dynamic_cast<MidiDevice*>(*it);
            if (!midiDevice)
                continue;

            // skip read-only devices
            if (midiDevice->getDirection() == MidiDevice::Record)
                continue;

            m_deviceNameMap[midiDevice->getId()] = midiDevice->getName();
            QString itemName = strtoqstr(midiDevice->getName());

            RG_DEBUG << "BankEditorDialog::initDialog - adding "
            << itemName << endl;

            QListViewItem* deviceItem = new MidiDeviceListViewItem
                                        (midiDevice->getId(), m_listView, itemName);
            deviceItem->setOpen(true);

            populateDeviceItem(deviceItem, midiDevice);
        }
    }

    // Select the first Device
    //
    populateDevice(m_listView->firstChild());
    m_listView->setSelected(m_listView->firstChild(), true);

}

void
BankEditorDialog::updateDialog()
{
    // Update list view
    //
    DeviceList *devices = m_studio->getDevices();
    DeviceListIterator it;
    bool deviceLabelUpdate = false;

    for (it = devices->begin(); it != devices->end(); ++it) {

        if ((*it)->getType() != Device::Midi)
            continue;

        MidiDevice* midiDevice =
            dynamic_cast<MidiDevice*>(*it);
        if (!midiDevice)
            continue;

        // skip read-only devices
        if (midiDevice->getDirection() == MidiDevice::Record)
            continue;

        if (m_deviceNameMap.find(midiDevice->getId()) != m_deviceNameMap.end()) {
            // Device already displayed but make sure the label is up to date
            //
            QListViewItem* currentItem = m_listView->currentItem();

            if (currentItem) {
                MidiDeviceListViewItem* deviceItem =
                    getParentDeviceItem(currentItem);

                if (deviceItem &&
                        deviceItem->getDeviceId() == midiDevice->getId()) {
                    if (deviceItem->text(0) != strtoqstr(midiDevice->getName())) {
                        deviceItem->setText(0,
                                            strtoqstr(midiDevice->getName()));
                        m_deviceNameMap[midiDevice->getId()] =
                            midiDevice->getName();

                        /*
                        cout << "NEW TEXT FOR DEVICE " << midiDevice->getId()
                            << " IS " << midiDevice->getName() << endl;
                        cout << "LIST ITEM ID = "
                            << deviceItem->getDeviceId() << endl;
                            */

                        deviceLabelUpdate = true;
                    }

                    QListViewItem *child = deviceItem->firstChild();

                    while (child) {

                        MidiBankListViewItem *bankItem =
                            dynamic_cast<MidiBankListViewItem *>(child);

                        if (bankItem) {
                            bool percussion = bankItem->isPercussion();
                            int msb = bankItem->text(2).toInt();
                            int lsb = bankItem->text(3).toInt();
                            std::string bankName =
                                midiDevice->getBankName
                                (MidiBank(percussion, msb, lsb));
                            if (bankName != "" &&
                                    bankItem->text(0) != strtoqstr(bankName)) {
                                bankItem->setText(0, strtoqstr(bankName));
                            }
                        }

                        child = child->nextSibling();
                    }
                }
            }

            continue;
        }

        m_deviceNameMap[midiDevice->getId()] = midiDevice->getName();
        QString itemName = strtoqstr(midiDevice->getName());

        RG_DEBUG << "BankEditorDialog::updateDialog - adding "
        << itemName << endl;

        QListViewItem* deviceItem = new MidiDeviceListViewItem
                                    (midiDevice->getId(), m_listView, itemName);
        deviceItem->setOpen(true);

        populateDeviceItem(deviceItem, midiDevice);
    }

    // delete items whose corresponding devices are no longer present,
    // and update the other ones
    //
    std::vector<MidiDeviceListViewItem*> itemsToDelete;

    MidiDeviceListViewItem* sibling = dynamic_cast<MidiDeviceListViewItem*>
                                      (m_listView->firstChild());

    while (sibling) {

        if (m_deviceNameMap.find(sibling->getDeviceId()) == m_deviceNameMap.end())
            itemsToDelete.push_back(sibling);
        else
            updateDeviceItem(sibling);

        sibling = dynamic_cast<MidiDeviceListViewItem*>(sibling->nextSibling());
    }

    for (unsigned int i = 0; i < itemsToDelete.size(); ++i)
        delete itemsToDelete[i];

    m_listView->sort();

    if (deviceLabelUpdate)
        emit deviceNamesChanged();
}

void
BankEditorDialog::setCurrentDevice(DeviceId device)
{
    for (QListViewItem *item = m_listView->firstChild(); item;
            item = item->nextSibling()) {
        MidiDeviceListViewItem * deviceItem =
            dynamic_cast<MidiDeviceListViewItem *>(item);
        if (deviceItem && deviceItem->getDeviceId() == device) {
            m_listView->setSelected(item, true);
            break;
        }
    }
}

void
BankEditorDialog::populateDeviceItem(QListViewItem* deviceItem, MidiDevice* midiDevice)
{
    clearItemChildren(deviceItem);

    QString itemName = strtoqstr(midiDevice->getName());

    BankList banks = midiDevice->getBanks();
    // add banks for this device
    for (unsigned int i = 0; i < banks.size(); ++i) {
        RG_DEBUG << "BankEditorDialog::populateDeviceItem - adding "
        << itemName << " - " << strtoqstr(banks[i].getName())
        << endl;
        new MidiBankListViewItem(midiDevice->getId(), i, deviceItem,
                                 strtoqstr(banks[i].getName()),
                                 banks[i].isPercussion(),
                                 banks[i].getMSB(), banks[i].getLSB());
    }

    const KeyMappingList &mappings = midiDevice->getKeyMappings();
    for (unsigned int i = 0; i < mappings.size(); ++i) {
        RG_DEBUG << "BankEditorDialog::populateDeviceItem - adding key mapping "
        << itemName << " - " << strtoqstr(mappings[i].getName())
        << endl;
        new MidiKeyMapListViewItem(midiDevice->getId(), deviceItem,
                                   strtoqstr(mappings[i].getName()));
    }
}

void
BankEditorDialog::updateDeviceItem(MidiDeviceListViewItem* deviceItem)
{
    MidiDevice* midiDevice = getMidiDevice(deviceItem->getDeviceId());
    if (!midiDevice) {
        RG_DEBUG << "BankEditorDialog::updateDeviceItem : WARNING no midi device for this item\n";
        return ;
    }

    QString itemName = strtoqstr(midiDevice->getName());

    BankList banks = midiDevice->getBanks();
    KeyMappingList keymaps = midiDevice->getKeyMappings();

    // add missing banks for this device
    //
    for (unsigned int i = 0; i < banks.size(); ++i) {
        if (deviceItemHasBank(deviceItem, i))
            continue;

        RG_DEBUG << "BankEditorDialog::updateDeviceItem - adding "
        << itemName << " - " << strtoqstr(banks[i].getName())
        << endl;
        new MidiBankListViewItem(midiDevice->getId(), i, deviceItem,
                                 strtoqstr(banks[i].getName()),
                                 banks[i].isPercussion(),
                                 banks[i].getMSB(), banks[i].getLSB());
    }

    for (unsigned int i = 0; i < keymaps.size(); ++i) {

        QListViewItem *child = deviceItem->firstChild();
        bool have = false;

        while (child) {
            MidiKeyMapListViewItem *keyItem =
                dynamic_cast<MidiKeyMapListViewItem*>(child);
            if (keyItem) {
                if (keyItem->getName() == strtoqstr(keymaps[i].getName())) {
                    have = true;
                }
            }
            child = child->nextSibling();
        }

        if (have)
            continue;

        RG_DEBUG << "BankEditorDialog::updateDeviceItem - adding "
        << itemName << " - " << strtoqstr(keymaps[i].getName())
        << endl;
        new MidiKeyMapListViewItem(midiDevice->getId(), deviceItem,
                                   strtoqstr(keymaps[i].getName()));
    }

    // delete banks which are no longer present
    //
    std::vector<QListViewItem*> childrenToDelete;

    QListViewItem* child = deviceItem->firstChild();

    while (child) {

        MidiBankListViewItem *bankItem =
            dynamic_cast<MidiBankListViewItem *>(child);
        if (bankItem) {
            if (bankItem->getBank() >= int(banks.size()))
                childrenToDelete.push_back(child);
            else { // update the banks MSB/LSB which might have changed
                bankItem->setPercussion(banks[bankItem->getBank()].isPercussion());
                bankItem->setMSB(banks[bankItem->getBank()].getMSB());
                bankItem->setLSB(banks[bankItem->getBank()].getLSB());
            }
        }

        MidiKeyMapListViewItem *keyItem =
            dynamic_cast<MidiKeyMapListViewItem *>(child);
        if (keyItem) {
            if (!midiDevice->getKeyMappingByName(qstrtostr(keyItem->getName()))) {
                childrenToDelete.push_back(child);
            }
        }

        child = child->nextSibling();
    }

    for (unsigned int i = 0; i < childrenToDelete.size(); ++i)
        delete childrenToDelete[i];
}

bool
BankEditorDialog::deviceItemHasBank(MidiDeviceListViewItem* deviceItem, int bankNb)
{
    QListViewItem *child = deviceItem->firstChild();

    while (child) {
        MidiBankListViewItem *bankItem =
            dynamic_cast<MidiBankListViewItem*>(child);
        if (bankItem) {
            if (bankItem->getBank() == bankNb)
                return true;
        }
        child = child->nextSibling();
    }

    return false;
}

void
BankEditorDialog::clearItemChildren(QListViewItem* item)
{
    QListViewItem* child = 0;

    while ((child = item->firstChild()))
        delete child;
}

MidiDevice*
BankEditorDialog::getCurrentMidiDevice()
{
    return getMidiDevice(m_listView->currentItem());
}

void
BankEditorDialog::checkModified()
{
    if (!m_modified)
        return ;

    setModified(false);

    //     // then ask if we want to apply the changes

    //     int reply = KMessageBox::questionYesNo(this,
    //                                            i18n("Apply pending changes?"));

    ModifyDeviceCommand *command = 0;
    MidiDevice *device = getMidiDevice(m_lastDevice);
    if (!device) {
        RG_DEBUG << "%%% WARNING : BankEditorDialog::checkModified() - NO MIDI DEVICE for device "
        << m_lastDevice << endl;
        return ;
    }

    if (m_bankList.size() == 0 && m_programList.size() == 0) {

        command = new ModifyDeviceCommand(m_studio,
                                          m_lastDevice,
                                          m_deviceNameMap[m_lastDevice],
                                          device->getLibrarianName(),
                                          device->getLibrarianEmail()); // rename

        command->clearBankAndProgramList();

    } else {

        MidiDevice::VariationType variation =
            MidiDevice::NoVariations;
        if (m_variationToggle->isChecked()) {
            if (m_variationCombo->currentItem() == 0) {
                variation = MidiDevice::VariationFromLSB;
            } else {
                variation = MidiDevice::VariationFromMSB;
            }
        }

        command = new ModifyDeviceCommand(m_studio,
                                          m_lastDevice,
                                          m_deviceNameMap[m_lastDevice],
                                          device->getLibrarianName(),
                                          device->getLibrarianEmail());

        command->setVariation(variation);
        command->setBankList(m_bankList);
        command->setProgramList(m_programList);
    }

    addCommandToHistory(command);

    setModified(false);
}

void
BankEditorDialog::slotPopulateDevice(QListViewItem* item)
{
    RG_DEBUG << "BankEditorDialog::slotPopulateDevice" << endl;

    if (!item)
        return ;

    checkModified();

    populateDevice(item);
}

void
BankEditorDialog::populateDevice(QListViewItem* item)
{
    RG_DEBUG << "BankEditorDialog::populateDevice\n";

    if (!item)
        return ;

    MidiKeyMapListViewItem *keyItem = dynamic_cast<MidiKeyMapListViewItem *>(item);

    if (keyItem) {

        stateChanged("on_key_item");
        stateChanged("on_bank_item", KXMLGUIClient::StateReverse);

        m_delete->setEnabled(true);

        MidiDevice *device = getMidiDevice(keyItem->getDeviceId());
        if (!device)
            return ;

        setProgramList(device);

        m_keyMappingEditor->populate(item);

        m_programEditor->hide();
        m_keyMappingEditor->show();

        m_lastDevice = keyItem->getDeviceId();

        return ;
    }

    MidiBankListViewItem* bankItem = dynamic_cast<MidiBankListViewItem*>(item);

    if (bankItem) {

        stateChanged("on_bank_item");
        stateChanged("on_key_item", KXMLGUIClient::StateReverse);

        m_delete->setEnabled(true);
        m_copyPrograms->setEnabled(true);

        if (m_copyBank.first != Device::NO_DEVICE)
            m_pastePrograms->setEnabled(true);

        MidiDevice *device = getMidiDevice(bankItem->getDeviceId());
        if (!device)
            return ;

        if (!m_keepBankList || m_bankList.size() == 0)
            m_bankList = device->getBanks();
        else
            m_keepBankList = false;

        setProgramList(device);

        m_variationToggle->setChecked(device->getVariationType() !=
                                      MidiDevice::NoVariations);
        m_variationCombo->setEnabled(m_variationToggle->isChecked());
        m_variationCombo->setCurrentItem
        (device->getVariationType() ==
         MidiDevice::VariationFromLSB ? 0 : 1);

        m_lastBank = m_bankList[bankItem->getBank()];

        m_programEditor->populate(item);

        m_keyMappingEditor->hide();
        m_programEditor->show();

        m_lastDevice = bankItem->getDeviceId();

        return ;
    }

    // Device, not bank or key mapping
    // Ensure we fill these lists for the new device
    //
    MidiDeviceListViewItem* deviceItem = getParentDeviceItem(item);

    m_lastDevice = deviceItem->getDeviceId();

    MidiDevice *device = getMidiDevice(deviceItem);
    if (!device) {
        RG_DEBUG << "BankEditorDialog::populateDevice - no device for this item\n";
        return ;
    }

    m_bankList = device->getBanks();
    setProgramList(device);

    RG_DEBUG << "BankEditorDialog::populateDevice : not a bank item - disabling" << endl;
    m_delete->setEnabled(false);
    m_copyPrograms->setEnabled(false);
    m_pastePrograms->setEnabled(false);

    m_variationToggle->setChecked(device->getVariationType() !=
                                  MidiDevice::NoVariations);
    m_variationCombo->setEnabled(m_variationToggle->isChecked());
    m_variationCombo->setCurrentItem
    (device->getVariationType() ==
     MidiDevice::VariationFromLSB ? 0 : 1);

    stateChanged("on_bank_item", KXMLGUIClient::StateReverse);
    stateChanged("on_key_item", KXMLGUIClient::StateReverse);
    m_programEditor->clearAll();
    m_keyMappingEditor->clearAll();
}

void
BankEditorDialog::slotApply()
{
    RG_DEBUG << "BankEditorDialog::slotApply()\n";

    ModifyDeviceCommand *command = 0;

    MidiDevice *device = getMidiDevice(m_lastDevice);

    // Make sure that we don't delete all the banks and programs
    // if we've not populated them here yet.
    //
    if (m_bankList.size() == 0 && m_programList.size() == 0 &&
            m_deleteAllReally == false) {
        RG_DEBUG << "BankEditorDialog::slotApply() : m_bankList size = 0\n";

        command = new ModifyDeviceCommand(m_studio,
                                          m_lastDevice,
                                          m_deviceNameMap[m_lastDevice],
                                          device->getLibrarianName(),
                                          device->getLibrarianEmail());

        command->clearBankAndProgramList();
    } else {
        MidiDevice::VariationType variation =
            MidiDevice::NoVariations;
        if (m_variationToggle->isChecked()) {
            if (m_variationCombo->currentItem() == 0) {
                variation = MidiDevice::VariationFromLSB;
            } else {
                variation = MidiDevice::VariationFromMSB;
            }
        }

        RG_DEBUG << "BankEditorDialog::slotApply() : m_bankList size = "
        << m_bankList.size() << endl;

        command = new ModifyDeviceCommand(m_studio,
                                          m_lastDevice,
                                          m_deviceNameMap[m_lastDevice],
                                          device->getLibrarianName(),
                                          device->getLibrarianEmail());

        MidiKeyMapListViewItem *keyItem = dynamic_cast<MidiKeyMapListViewItem*>
                                          (m_listView->currentItem());
        if (keyItem) {
            KeyMappingList kml(device->getKeyMappings());
            for (int i = 0; i < kml.size(); ++i) {
                if (kml[i].getName() == qstrtostr(keyItem->getName())) {
                    kml[i] = m_keyMappingEditor->getMapping();
                    break;
                }
            }
            command->setKeyMappingList(kml);
        }

        command->setVariation(variation);
        command->setBankList(m_bankList);
        command->setProgramList(m_programList);
    }

    addCommandToHistory(command);

    // Our freaky fudge to update instrument/device names externally
    //
    if (m_updateDeviceList) {
        emit deviceNamesChanged();
        m_updateDeviceList = false;
    }

    setModified(false);
}

void
BankEditorDialog::slotReset()
{
    resetProgramList();

    m_programEditor->reset();
    m_programEditor->populate(m_listView->currentItem());
    m_keyMappingEditor->reset();
    m_keyMappingEditor->populate(m_listView->currentItem());

    MidiDeviceListViewItem* deviceItem = getParentDeviceItem
                                         (m_listView->currentItem());

    if (deviceItem) {
        MidiDevice *device = getMidiDevice(deviceItem);
        m_variationToggle->setChecked(device->getVariationType() !=
                                      MidiDevice::NoVariations);
        m_variationCombo->setEnabled(m_variationToggle->isChecked());
        m_variationCombo->setCurrentItem
        (device->getVariationType() ==
         MidiDevice::VariationFromLSB ? 0 : 1);
    }

    updateDialog();

    setModified(false);
}

void
BankEditorDialog::resetProgramList()
{
    m_programList = m_oldProgramList;
}

void
BankEditorDialog::setProgramList(MidiDevice *device)
{
    m_programList = device->getPrograms();
    m_oldProgramList = m_programList;
}

void
BankEditorDialog::slotUpdate()
{
    updateDialog();
}

MidiDeviceListViewItem*
BankEditorDialog::getParentDeviceItem(QListViewItem* item)
{
    if (!item)
        return 0;

    if (dynamic_cast<MidiBankListViewItem*>(item))
        // go up to the parent device item
        item = item->parent();

    if (dynamic_cast<MidiKeyMapListViewItem*>(item))
        // go up to the parent device item
        item = item->parent();

    if (!item) {
        RG_DEBUG << "BankEditorDialog::getParentDeviceItem : missing parent device item for bank item - this SHOULD NOT HAPPEN" << endl;
        return 0;
    }

    return dynamic_cast<MidiDeviceListViewItem*>(item);
}

void
BankEditorDialog::slotAddBank()
{
    if (!m_listView->currentItem())
        return ;

    QListViewItem* currentItem = m_listView->currentItem();

    MidiDeviceListViewItem* deviceItem = getParentDeviceItem(currentItem);
    MidiDevice *device = getMidiDevice(currentItem);

    if (device) {
        // If the bank and program lists are empty then try to
        // populate them.
        //
        if (m_bankList.size() == 0 && m_programList.size() == 0) {
            m_bankList = device->getBanks();
            setProgramList(device);
        }

        std::pair<int, int> bank = getFirstFreeBank(m_listView->currentItem());

        MidiBank newBank(false,
                         bank.first, bank.second,
                         qstrtostr(i18n("<new bank>")));
        m_bankList.push_back(newBank);

        QListViewItem* newBankItem =
            new MidiBankListViewItem(deviceItem->getDeviceId(),
                                     m_bankList.size() - 1,
                                     deviceItem,
                                     strtoqstr(newBank.getName()),
                                     newBank.isPercussion(),
                                     newBank.getMSB(), newBank.getLSB());
        keepBankListForNextPopulate();
        m_listView->setCurrentItem(newBankItem);

        slotApply();
        selectDeviceItem(device);
    }
}

void
BankEditorDialog::slotAddKeyMapping()
{
    if (!m_listView->currentItem())
        return ;

    QListViewItem* currentItem = m_listView->currentItem();

    MidiDeviceListViewItem* deviceItem = getParentDeviceItem(currentItem);
    MidiDevice *device = getMidiDevice(currentItem);

    if (device) {

        QString name = "";
        int n = 0;
        while (name == "" || device->getKeyMappingByName(qstrtostr(name)) != 0) {
            ++n;
            if (n == 1)
                name = i18n("<new mapping>");
            else
                name = i18n("<new mapping %1>").arg(n);
        }

        MidiKeyMapping newKeyMapping(qstrtostr(name));

        ModifyDeviceCommand *command = new ModifyDeviceCommand
                                       (m_studio,
                                        device->getId(),
                                        device->getName(),
                                        device->getLibrarianName(),
                                        device->getLibrarianEmail());

        KeyMappingList kml;
        kml.push_back(newKeyMapping);
        command->setKeyMappingList(kml);
        command->setOverwrite(false);
        command->setRename(false);

        addCommandToHistory(command);

        updateDialog();
        selectDeviceItem(device);
    }
}

void
BankEditorDialog::slotDelete()
{
    if (!m_listView->currentItem())
        return ;

    QListViewItem* currentItem = m_listView->currentItem();

    MidiBankListViewItem* bankItem = dynamic_cast<MidiBankListViewItem*>(currentItem);

    MidiDevice *device = getMidiDevice(currentItem);

    if (device && bankItem) {
        int currentBank = bankItem->getBank();

        int reply =
            KMessageBox::warningYesNo(this, i18n("Really delete this bank?"));

        if (reply == KMessageBox::Yes) {
            MidiBank bank = m_bankList[currentBank];

            // Copy across all programs that aren't in the doomed bank
            //
            ProgramList::iterator it;
            ProgramList tempList;
            for (it = m_programList.begin(); it != m_programList.end(); it++)
                if (!(it->getBank() == bank))
                    tempList.push_back(*it);

            // Erase the bank and repopulate
            //
            BankList::iterator er =
                m_bankList.begin();
            er += currentBank;
            m_bankList.erase(er);
            m_programList = tempList;
            keepBankListForNextPopulate();

            // the listview automatically selects a new current item
            m_listView->blockSignals(true);
            delete currentItem;
            m_listView->blockSignals(false);

            // Don't allow pasting from this defunct device
            //
            if (m_copyBank.first == bankItem->getDeviceId() &&
                    m_copyBank.second == bankItem->getBank()) {
                m_pastePrograms->setEnabled(false);
                m_copyBank = std::pair<DeviceId, int>
                             (Device::NO_DEVICE, -1);
            }

            slotApply();
            selectDeviceItem(device);
        }

        return ;
    }

    MidiKeyMapListViewItem* keyItem = dynamic_cast<MidiKeyMapListViewItem*>(currentItem);

    if (keyItem && device) {

        int reply =
            KMessageBox::warningYesNo(this, i18n("Really delete this key mapping?"));

        if (reply == KMessageBox::Yes) {

            std::string keyMappingName = qstrtostr(keyItem->getName());

            ModifyDeviceCommand *command = new ModifyDeviceCommand
                                           (m_studio,
                                            device->getId(),
                                            device->getName(),
                                            device->getLibrarianName(),
                                            device->getLibrarianEmail());

            KeyMappingList kml = device->getKeyMappings();

            for (KeyMappingList::iterator i = kml.begin();
                    i != kml.end(); ++i) {
                if (i->getName() == keyMappingName) {
                    RG_DEBUG << "erasing " << keyMappingName << endl;
                    kml.erase(i);
                    break;
                }
            }

            RG_DEBUG << " setting " << kml.size() << " key mappings to device " << endl;

            command->setKeyMappingList(kml);
            command->setOverwrite(true);

            addCommandToHistory(command);

            RG_DEBUG << " device has " << device->getKeyMappings().size() << " key mappings now " << endl;

            updateDialog();
        }

        return ;
    }
}

void
BankEditorDialog::slotDeleteAll()
{
    if (!m_listView->currentItem())
        return ;

    QListViewItem* currentItem = m_listView->currentItem();
    MidiDeviceListViewItem* deviceItem = getParentDeviceItem(currentItem);
    MidiDevice *device = getMidiDevice(deviceItem);

    QString question = i18n("Really delete all banks for ") +
                       strtoqstr(device->getName()) + QString(" ?");

    int reply = KMessageBox::warningYesNo(this, question);

    if (reply == KMessageBox::Yes) {

        // erase all bank items
        QListViewItem* child = 0;
        while ((child = deviceItem->firstChild()))
            delete child;

        m_bankList.clear();
        m_programList.clear();

        // Don't allow pasting from this defunct device
        //
        if (m_copyBank.first == deviceItem->getDeviceId()) {
            m_pastePrograms->setEnabled(false);
            m_copyBank = std::pair<DeviceId, int>
                         (Device::NO_DEVICE, -1);
        }

        // Urgh, we have this horrible flag that we're using to frig this.
        // (we might not need this anymore but I'm too scared to remove it
        // now).
        //
        m_deleteAllReally = true;
        slotApply();
        m_deleteAllReally = false;

        selectDeviceItem(device);

    }
}

MidiDevice*
BankEditorDialog::getMidiDevice(DeviceId id)
{
    Device *device = m_studio->getDevice(id);
    MidiDevice *midiDevice =
        dynamic_cast<MidiDevice *>(device);

    /*
    if (device) {
    if (!midiDevice) {
     std::cerr << "ERROR: BankEditorDialog::getMidiDevice: device "
        << id << " is not a MIDI device" << std::endl;
    }
    } else {
    std::cerr
     << "ERROR: BankEditorDialog::getMidiDevice: no such device as "
     << id << std::endl;
    }
    */

    return midiDevice;
}

MidiDevice*
BankEditorDialog::getMidiDevice(QListViewItem* item)
{
    MidiDeviceListViewItem* deviceItem =
        dynamic_cast<MidiDeviceListViewItem*>(item);
    if (!deviceItem)
        return 0;

    return getMidiDevice(deviceItem->getDeviceId());
}

std::pair<int, int>
BankEditorDialog::getFirstFreeBank(QListViewItem* item)
{
    //!!! percussion? this is actually only called in the expectation
    // that percussion==false at the moment

    for (int msb = 0; msb < 128; ++msb) {
        for (int lsb = 0; lsb < 128; ++lsb) {
            BankList::iterator i = m_bankList.begin();
            for ( ; i != m_bankList.end(); ++i) {
                if (i->getLSB() == lsb && i->getMSB() == msb) {
                    break;
                }
            }
            if (i == m_bankList.end())
                return std::pair<int, int>(msb, lsb);
        }
    }

    return std::pair<int, int>(0, 0);
}

void
BankEditorDialog::slotModifyDeviceOrBankName(QListViewItem* item, const QString &label, int)
{
    RG_DEBUG << "BankEditorDialog::slotModifyDeviceOrBankName" << endl;

    MidiDeviceListViewItem* deviceItem =
        dynamic_cast<MidiDeviceListViewItem*>(item);
    MidiBankListViewItem* bankItem =
        dynamic_cast<MidiBankListViewItem*>(item);
    MidiKeyMapListViewItem *keyItem =
        dynamic_cast<MidiKeyMapListViewItem*>(item);

    if (bankItem) {

        // renaming a bank item

        RG_DEBUG << "BankEditorDialog::slotModifyDeviceOrBankName - "
        << "modify bank name to " << label << endl;

        if (m_bankList[bankItem->getBank()].getName() != qstrtostr(label)) {
            m_bankList[bankItem->getBank()].setName(qstrtostr(label));
            setModified(true);
        }

    } else if (keyItem) {

        RG_DEBUG << "BankEditorDialog::slotModifyDeviceOrBankName - "
        << "modify key mapping name to " << label << endl;

        QString oldName = keyItem->getName();

        QListViewItem* currentItem = m_listView->currentItem();
        MidiDevice *device = getMidiDevice(currentItem);

        if (device) {

            ModifyDeviceCommand *command = new ModifyDeviceCommand
                                           (m_studio,
                                            device->getId(),
                                            device->getName(),
                                            device->getLibrarianName(),
                                            device->getLibrarianEmail());

            KeyMappingList kml = device->getKeyMappings();

            for (KeyMappingList::iterator i = kml.begin();
                    i != kml.end(); ++i) {
                if (i->getName() == qstrtostr(oldName)) {
                    i->setName(qstrtostr(label));
                    break;
                }
            }

            command->setKeyMappingList(kml);
            command->setOverwrite(true);

            addCommandToHistory(command);

            updateDialog();
        }

    } else if (deviceItem) { // must be last, as the others are subclasses

        // renaming a device item

        RG_DEBUG << "BankEditorDialog::slotModifyDeviceOrBankName - "
        << "modify device name to " << label << endl;

        if (m_deviceNameMap[deviceItem->getDeviceId()] != qstrtostr(label)) {
            m_deviceNameMap[deviceItem->getDeviceId()] = qstrtostr(label);
            setModified(true);

            m_updateDeviceList = true;
        }

    }

}

void
BankEditorDialog::selectDeviceItem(MidiDevice *device)
{
    QListViewItem *child = m_listView->firstChild();
    MidiDeviceListViewItem *midiDeviceItem;
    MidiDevice *midiDevice;

    do {
        midiDeviceItem = dynamic_cast<MidiDeviceListViewItem*>(child);

        if (midiDeviceItem) {
            midiDevice = getMidiDevice(midiDeviceItem);

            if (midiDevice == device) {
                m_listView->setSelected(child, true);
                return ;
            }
        }

    } while ((child = child->nextSibling()));
}

void
BankEditorDialog::selectDeviceBankItem(DeviceId deviceId,
                                       int bank)
{
    QListViewItem *deviceChild = m_listView->firstChild();
    QListViewItem *bankChild;
    int deviceCount = 0, bankCount = 0;

    do {
        bankChild = deviceChild->firstChild();

        MidiDeviceListViewItem *midiDeviceItem =
            dynamic_cast<MidiDeviceListViewItem*>(deviceChild);

        if (midiDeviceItem && bankChild) {
            do {
                if (deviceId == midiDeviceItem->getDeviceId() &
                        bank == bankCount) {
                    m_listView->setSelected(bankChild, true);
                    return ;
                }
                bankCount++;

            } while ((bankChild = bankChild->nextSibling()));
        }

        deviceCount++;
        bankCount = 0;
    } while ((deviceChild = deviceChild->nextSibling()));
}

void
BankEditorDialog::slotVariationToggled()
{
    setModified(true);
    m_variationCombo->setEnabled(m_variationToggle->isChecked());
}

void
BankEditorDialog::slotVariationChanged(int)
{
    setModified(true);
}

void
BankEditorDialog::setModified(bool modified)
{
    RG_DEBUG << "BankEditorDialog::setModified("
    << modified << ")" << endl;

    if (modified) {

        m_applyButton->setEnabled(true);
        m_resetButton->setEnabled(true);
        m_closeButton->setEnabled(false);
        m_listView->setEnabled(false);

    } else {

        m_applyButton->setEnabled(false);
        m_resetButton->setEnabled(false);
        m_closeButton->setEnabled(true);
        m_listView->setEnabled(true);

    }

    m_modified = modified;
}

void
BankEditorDialog::addCommandToHistory(KCommand *command)
{
    getCommandHistory()->addCommand(command);
    setModified(false);
}

MultiViewCommandHistory*
BankEditorDialog::getCommandHistory()
{
    return m_doc->getCommandHistory();
}

void
BankEditorDialog::slotImport()
{
    QString deviceDir = KGlobal::dirs()->findResource("appdata", "library/");
    QDir dir(deviceDir);
    if (!dir.exists()) {
        deviceDir = ":ROSEGARDENDEVICE";
    } else {
        deviceDir = "file://" + deviceDir;
    }

    KURL url = KFileDialog::getOpenURL
               (deviceDir,
                "audio/x-rosegarden-device audio/x-rosegarden audio/x-soundfont",
                this, i18n("Import Banks from Device in File"));

    if (url.isEmpty())
        return ;

    ImportDeviceDialog *dialog = new ImportDeviceDialog(this, url);
    if (dialog->doImport() && dialog->exec() == QDialog::Accepted) {

        MidiDeviceListViewItem* deviceItem =
            dynamic_cast<MidiDeviceListViewItem*>
            (m_listView->selectedItem());

        if (!deviceItem) {
            KMessageBox::error(this, "Some internal error: cannot locate selected device");
            return ;
        }

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
                                          deviceItem->getDeviceId(),
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

        addCommandToHistory(command);

        // No need to redraw the dialog, this is done by
        // slotUpdate, signalled by the MultiViewCommandHistory
        MidiDevice *device = getMidiDevice(deviceItem);
        if (device)
            selectDeviceItem(device);
    }

    delete dialog;
    updateDialog();
}

void
BankEditorDialog::slotEditCopy()
{
    MidiBankListViewItem* bankItem
    = dynamic_cast<MidiBankListViewItem*>(m_listView->currentItem());

    if (bankItem) {
        m_copyBank = std::pair<DeviceId, int>(bankItem->getDeviceId(),
                                              bankItem->getBank());
        m_pastePrograms->setEnabled(true);
    }
}

void
BankEditorDialog::slotEditPaste()
{
    MidiBankListViewItem* bankItem
    = dynamic_cast<MidiBankListViewItem*>(m_listView->currentItem());

    if (bankItem) {
        // Get the full program and bank list for the source device
        //
        MidiDevice *device = getMidiDevice(m_copyBank.first);
        std::vector<MidiBank> tempBank = device->getBanks();

        ProgramList::iterator it;
        std::vector<MidiProgram> tempProg;

        // Remove programs that will be overwritten
        //
        for (it = m_programList.begin(); it != m_programList.end(); it++) {
            if (!(it->getBank() == m_lastBank))
                tempProg.push_back(*it);
        }
        m_programList = tempProg;

        // Now get source list and msb/lsb
        //
        tempProg = device->getPrograms();
        MidiBank sourceBank = tempBank[m_copyBank.second];

        // Add the new programs
        //
        for (it = tempProg.begin(); it != tempProg.end(); it++) {
            if (it->getBank() == sourceBank) {
                // Insert with new MSB and LSB
                //
                MidiProgram copyProgram(m_lastBank,
                                        it->getProgram(),
                                        it->getName());

                m_programList.push_back(copyProgram);
            }
        }

        // Save these for post-apply
        //
        DeviceId devPos = bankItem->getDeviceId();
        int bankPos = bankItem->getBank();

        slotApply();

        // Select same bank
        //
        selectDeviceBankItem(devPos, bankPos);
    }
}

void
BankEditorDialog::slotExport()
{
    QString extension = "rgd";

    QString name =
        KFileDialog::getSaveFileName(":ROSEGARDEN",
                                     (extension.isEmpty() ? QString("*") : ("*." + extension)),
                                     this,
                                     i18n("Export Device as..."));

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
        KMessageBox::sorry(this, i18n("You have specified a directory"));
        return ;
    }

    if (info.exists()) {
        int overwrite = KMessageBox::questionYesNo
                        (this, i18n("The specified file exists.  Overwrite?"));

        if (overwrite != KMessageBox::Yes)
            return ;

    }

    MidiDeviceListViewItem* deviceItem =
        dynamic_cast<MidiDeviceListViewItem*>
        (m_listView->selectedItem());

    std::vector<DeviceId> devices;
    MidiDevice *md = getMidiDevice(deviceItem);

    if (md) {
        ExportDeviceDialog *ed = new ExportDeviceDialog
                                 (this, strtoqstr(md->getName()));
        if (ed->exec() != QDialog::Accepted)
            return ;
        if (ed->getExportType() == ExportDeviceDialog::ExportOne) {
            devices.push_back(md->getId());
        }
    }

    m_doc->exportStudio(name, devices);
}

void
BankEditorDialog::slotFileClose()
{
    RG_DEBUG << "BankEditorDialog::slotFileClose()\n";

    // We need to do this because we might be here due to a
    // documentAboutToChange signal, in which case the document won't
    // be valid by the time we reach the dtor, since it will be
    // triggered when the closeEvent is actually processed.
    //
    m_doc->getCommandHistory()->detachView(actionCollection());
    m_doc = 0;
    close();
}

void
BankEditorDialog::closeEvent(QCloseEvent *e)
{
    if (m_modified) {

        int res = KMessageBox::warningYesNoCancel(this,
                  i18n("There are unsaved changes.\n"
                       "Do you want to apply the changes before exiting "
                       "the Bank Editor or discard the changes ?"),
                  i18n("Unsaved Changes"),
                  i18n("&Apply"),
                  i18n("&Discard"));
        if (res == KMessageBox::Yes) {

            slotApply();

        } else if (res == KMessageBox::Cancel)
            return ;
    }

    emit closing();
    KMainWindow::closeEvent(e);
}

}
#include "BankEditorDialog.moc"
