/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[BankEditorDialog]"
#define RG_NO_DEBUG_PRINT

#include "BankEditorDialog.h"
#include "MidiBankTreeWidgetItem.h"
#include "MidiDeviceTreeWidgetItem.h"
#include "MidiKeyMapTreeWidgetItem.h"
#include "MidiKeyMappingEditor.h"
#include "MidiProgramsEditor.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "misc/ConfigGroups.h"
#include "base/Device.h"
#include "base/MidiDevice.h"
#include "base/MidiProgram.h"
#include "base/NotationTypes.h"
#include "commands/studio/ModifyDeviceCommand.h"
#include "document/CommandHistory.h"
#include "document/RosegardenDocument.h"
#include "misc/ConfigGroups.h"
#include "gui/dialogs/ExportDeviceDialog.h"
#include "gui/dialogs/ImportDeviceDialog.h"
#include "gui/dialogs/LibrarianDialog.h"
#include "gui/widgets/FileDialog.h"
#include "gui/general/ResourceFinder.h"
#include "gui/general/ThornStyle.h"
#include "gui/dialogs/AboutDialog.h"
#include "document/Command.h"

#include <QLayout>
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QTreeWidget>
#include <QMainWindow>
#include <QMessageBox>
#include <QCheckBox>
#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QGroupBox>
#include <QPushButton>
#include <QSizePolicy>
#include <QString>
#include <QToolTip>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDesktopServices>
#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#endif
#include <QDialogButtonBox>


namespace Rosegarden
{

BankEditorDialog::BankEditorDialog(QWidget *parent,
                                   RosegardenDocument *doc,
                                   DeviceId defaultDevice):
        QMainWindow(parent),
        m_studio(&doc->getStudio()),
        m_doc(doc)
{
    m_clipboard.itemType = ItemType::NONE;
    m_clipboard.deviceId = Device::NO_DEVICE;
    m_clipboard.bank = -1;
    m_clipboard.keymapName = "";

    QWidget* mainFrame = new QWidget(this);
    mainFrame->setContentsMargins(1, 1, 1, 1);
    setCentralWidget(mainFrame);
    QVBoxLayout *mainFrameLayout = new QVBoxLayout;
    mainFrameLayout->setContentsMargins(0, 0, 0, 0);
    mainFrameLayout->setSpacing(2);
    mainFrame->setLayout(mainFrameLayout);

    setWindowTitle(tr("Manage MIDI Banks and Programs"));

    QWidget *splitter = new QWidget;
    QHBoxLayout *splitterLayout = new QHBoxLayout;
    splitterLayout->setContentsMargins(0, 0, 0, 0);
    splitter->setLayout(splitterLayout);

    mainFrameLayout->addWidget(splitter);

    //
    // Left-side list view
    //
    QWidget *leftPart = new QWidget;
    QVBoxLayout *leftPartLayout = new QVBoxLayout;
    leftPartLayout->setContentsMargins(2, 2, 2, 2);
    leftPart->setLayout(leftPartLayout);
    splitterLayout->addWidget(leftPart);

    m_treeWidget = new QTreeWidget;
    leftPartLayout->addWidget(m_treeWidget);

    m_treeWidget->setColumnCount(4);
    QStringList sl;
    sl << tr("Device and Banks")
       << tr("Type")
       << tr("MSB")
       << tr("LSB");
    m_treeWidget->setHeaderLabels(sl);
    m_treeWidget->setRootIsDecorated(true);
    m_treeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);    //qt4
    m_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);    //qt4
//    m_treeWidget->setAllColumnsShowFocus(true);
    m_treeWidget->setSortingEnabled(true);

    /*
    m_treeWidget->setShowSortIndicator(true);        //&&&
    m_treeWidget->setItemsRenameable(true);
    m_treeWidget->restoreLayout(BankEditorConfigGroup);
    */

    QFrame *bankBox = new QFrame(leftPart);
    leftPartLayout->addWidget(bankBox);
    bankBox->setContentsMargins(1, 1, 1, 1);
    QGridLayout *gridLayout = new QGridLayout(bankBox);
    gridLayout->setSpacing(4);

    m_addBank = new QPushButton(tr("Add Bank"), bankBox);
    m_addKeyMapping = new QPushButton(tr("Add Key Mapping"), bankBox);
    m_delete = new QPushButton(tr("Delete"), bankBox);
    m_deleteAll = new QPushButton(tr("Delete All"), bankBox);
    gridLayout->addWidget(m_addBank, 0, 0);
    gridLayout->addWidget(m_addKeyMapping, 0, 1);
    gridLayout->addWidget(m_delete, 1, 0);
    gridLayout->addWidget(m_deleteAll, 1, 1);

    // Tips
    //
    m_addBank->setToolTip(tr("Add a Bank to the current device"));

    m_addKeyMapping->setToolTip(tr("Add a Percussion Key Mapping to the current device"));

    m_delete->setToolTip(tr("Delete the current Bank or Key Mapping"));

    m_deleteAll->setToolTip(tr("Delete all Banks and Key Mappings from the current Device"));

    m_importBanks = new QPushButton(tr("Import..."), bankBox);
    m_exportBanks = new QPushButton(tr("Export..."), bankBox);
    gridLayout->addWidget(m_importBanks, 2, 0);
    gridLayout->addWidget(m_exportBanks, 2, 1);

    // Tips
    //
    m_importBanks->setToolTip(tr("Import Bank and Program data from a Rosegarden file to the current Device"));
    m_exportBanks->setToolTip(tr("Export all Device and Bank information to a Rosegarden format  interchange file"));

    m_copyPrograms = new QPushButton(tr("Copy"), bankBox);
    m_pastePrograms = new QPushButton(tr("Paste"), bankBox);
    gridLayout->addWidget(m_copyPrograms, 3, 0);
    gridLayout->addWidget(m_pastePrograms, 3, 1);

    bankBox->setLayout(gridLayout);

    // Tips
    //
    m_copyPrograms->setToolTip(tr("Copy all Program names from current Bank or Keymap to clipboard"));

    m_pastePrograms->setToolTip(tr("Paste Program names from clipboard to current Bank or Keymap"));

//     connect(m_treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*)),
//             this, SLOT(slotPopulateDeviceEditors(QTreeWidgetItem*)));
    // note: above connection moved to setupActions

    // Right side layout
    m_rightSide = new QFrame;

    m_rightSide->setContentsMargins(8, 8, 8, 8);
    QVBoxLayout *rightSideLayout = new QVBoxLayout;
    rightSideLayout->setContentsMargins(0, 0, 0, 0);
    rightSideLayout->setSpacing(6);
    m_rightSide->setLayout(rightSideLayout);

    splitterLayout->addWidget(m_rightSide);

    m_programEditor = new MidiProgramsEditor(this, m_rightSide);
    rightSideLayout->addWidget(m_programEditor);

    m_keyMappingEditor = new MidiKeyMappingEditor(this, m_rightSide);
    rightSideLayout->addWidget(m_keyMappingEditor);
    m_keyMappingEditor->hide();

    m_programEditor->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred));
    m_keyMappingEditor->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred));

    m_optionBox = new QGroupBox(tr("Options"), m_rightSide);
    QVBoxLayout *optionBoxLayout = new QVBoxLayout;
    optionBoxLayout->setContentsMargins(0, 0, 0, 0);
    rightSideLayout->addWidget(m_optionBox);

    QWidget *variationBox = new QWidget(m_optionBox);
    QHBoxLayout *variationBoxLayout = new QHBoxLayout;
    variationBoxLayout->setContentsMargins(4, 4, 4, 4);
    optionBoxLayout->addWidget(variationBox);

    m_variationToggle = new QCheckBox(tr("Show Variation list based on "), variationBox);
    variationBoxLayout->addWidget(m_variationToggle);
    m_variationCombo = new QComboBox(variationBox);
    variationBoxLayout->addWidget(m_variationCombo);
    variationBox->setLayout(variationBoxLayout);
    m_variationCombo->addItem(tr("LSB"));
    m_variationCombo->addItem(tr("MSB"));

    m_optionBox->setLayout(optionBoxLayout);

    // device/bank modification

    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked,
            this, &BankEditorDialog::slotEdit);

    connect(m_addBank, &QAbstractButton::clicked,
            this, &BankEditorDialog::slotAddBank);

    connect(m_addKeyMapping, &QAbstractButton::clicked,
            this, &BankEditorDialog::slotAddKeyMapping);

    connect(m_delete, &QAbstractButton::clicked,
            this, &BankEditorDialog::slotDelete);

    connect(m_deleteAll, &QAbstractButton::clicked,
            this, &BankEditorDialog::slotDeleteAll);

    connect(m_importBanks, &QAbstractButton::clicked,
            this, &BankEditorDialog::slotImport);

    connect(m_exportBanks, &QAbstractButton::clicked,
            this, &BankEditorDialog::slotExport);

    connect(m_copyPrograms, &QAbstractButton::clicked,
            this, &BankEditorDialog::slotEditCopy);

    connect(m_pastePrograms, &QAbstractButton::clicked,
            this, &BankEditorDialog::slotEditPaste);

    connect(m_variationToggle, &QAbstractButton::clicked,
            this, &BankEditorDialog::slotVariationToggled);

    connect(m_variationCombo,
                static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &BankEditorDialog::slotVariationChanged);

    // Button box
    QDialogButtonBox *btnBox = new QDialogButtonBox(/*QDialogButtonBox::Apply  | */
                                                    QDialogButtonBox::Close);

    mainFrameLayout->addWidget(btnBox);

    m_closeButton = btnBox->button(QDialogButtonBox::Close);

    m_studio->addObserver(this);
    m_observingStudio = true;

    // Initialize the dialog
    //
    initDialog();

    setupActions();

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

    setAttribute(Qt::WA_DeleteOnClose);
//     setAutoSaveSettings(BankEditorConfigGroup, true);    //&&&
}

BankEditorDialog::~BankEditorDialog()
{
    RG_DEBUG << "~BankEditorDialog()\n";

    if (m_observingStudio) {
        m_observingStudio = false;
        m_studio->removeObserver(this);
    }
    for(Device* device : m_observedDevices) {
        unobserveDevice(device);
    }

//     m_treeWidget->saveLayout(BankEditorConfigGroup);    //&&&

//     if (m_doc) // see slotFileClose() for an explanation on why we need to test m_doc
//         CommandHistory::getInstance()->detachView(actionCollection());
}

void
BankEditorDialog::setupActions()
{
//     KAction* close = KStandardAction::close (this, SLOT(slotFileClose()), actionCollection());

    createAction("file_close", SLOT(slotFileClose()));

    connect(m_closeButton, &QAbstractButton::clicked, this, &BankEditorDialog::slotFileClose);

    createAction("edit_copy", SLOT(slotEditCopy()));
    createAction("edit_paste", SLOT(slotEditPaste()));
    createAction("bank_help", SLOT(slotHelpRequested()));
    createAction("help_about_app", SLOT(slotHelpAbout()));

    connect(m_treeWidget, &QTreeWidget::currentItemChanged,
            this, &BankEditorDialog::slotPopulateDeviceEditors);

    connect(m_treeWidget, &QTreeWidget::itemChanged, this,
            &BankEditorDialog::slotModifyDeviceOrBankName);

    // some adjustments

/*
    new KToolBarPopupAction(tr("Und&o"),
                            "undo",
                            KStandardShortcut::key(KStandardShortcut::Undo),
                            actionCollection(),
                            KStandardAction::stdName(KStandardAction::Undo));

    new KToolBarPopupAction(tr("Re&do"),
                            "redo",
                            KStandardShortcut::key(KStandardShortcut::Redo),
                            actionCollection(),
                            KStandardAction::stdName(KStandardAction::Redo));
    */

    createMenusAndToolbars("bankeditor.rc"); //@@@ JAS orig. 0
}

void
BankEditorDialog::initDialog()
{
    // Clear down
    //
    m_treeWidget->clear();

    // Fill list view
    //
    MidiDevice* midiDevice = nullptr;
    QTreeWidgetItem* twItemDevice = nullptr;
    DeviceList *devices = m_studio->getDevices();
    DeviceListIterator it;
//     unsigned int i = 0;

    // iterates over devices and create device-TreeWidgetItems (level: topLevelItem)
    // then calls populateDeviceItem() to create bank-TreeWidgetItems (level: topLevelItem-child)
    for (it = devices->begin(); it != devices->end(); ++it) {
        Device *devx = *it;
//     for ( i=0; i < int(devices->size()); i++ ){
//         devx = devices->at( i );

        if (devx->getType() == Device::Midi) {

            midiDevice = dynamic_cast<MidiDevice*>(devx);

            if (!midiDevice) continue;
            // skip read-only devices
            if (midiDevice->getDirection() == MidiDevice::Record) continue;

            observeDevice(midiDevice);

            QString itemName = strtoqstr(midiDevice->getName());

            RG_DEBUG << "BankEditorDialog::initDialog - adding " << itemName;

            twItemDevice =
                new MidiDeviceTreeWidgetItem(midiDevice,
                                             m_treeWidget,
                                             itemName);
            //twItemDevice->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable);

            m_treeWidget->addTopLevelItem(twItemDevice);  //

            twItemDevice->setExpanded(true);

            populateDeviceItem(twItemDevice, midiDevice);
        }
    }

    populateDeviceEditors(m_treeWidget->topLevelItem(0));

    // select the first device item
    m_treeWidget->topLevelItem(0)->setSelected(true);
    m_treeWidget->resizeColumnToContents(0);
    m_treeWidget->setMinimumWidth(500);
}

void
BankEditorDialog::updateDialog()
{
    RG_DEBUG << "updateDialog";
    // Update list view
    //
    m_treeWidget->blockSignals(true);
    DeviceList *devices = m_studio->getDevices();

    MidiDevice* midiDevice = nullptr;

    // get selected Item
    enum class SelectedType {NONE, DEVICE, BANK, KEYMAP};
    SelectedType selectedType = SelectedType::NONE;
    QString selectedName;
    Device* parentDevice = nullptr;

    QTreeWidgetItem* item = m_treeWidget->currentItem();
    if (item) {
        MidiDeviceTreeWidgetItem* deviceItem =
            dynamic_cast<MidiDeviceTreeWidgetItem*>(item);
        if (deviceItem) {
            selectedType = SelectedType::DEVICE;
            MidiDevice *device = deviceItem->getDevice();
            if (device) {
                selectedName = strtoqstr(device->getName());
                parentDevice = device;
            } else {
                selectedType = SelectedType::NONE;
            }
        }
        MidiKeyMapTreeWidgetItem *keyItem =
            dynamic_cast<MidiKeyMapTreeWidgetItem*>(item);
        if (keyItem) {
            selectedType = SelectedType::KEYMAP;
            selectedName = keyItem->getName();
            parentDevice = keyItem->getDevice();
        }
        MidiBankTreeWidgetItem *bankItem =
            dynamic_cast<MidiBankTreeWidgetItem*>(item);
        if (bankItem) {
            selectedType = SelectedType::BANK;
            selectedName = bankItem->getName();
            parentDevice = bankItem->getDevice();
        }
    }
    // if m_selectioName is set - us it
    if (m_selectionName != "") {
        selectedName = m_selectionName;
        m_selectionName = "";
    }
    RG_DEBUG << "selected item:" << (int)selectedType << selectedName <<
        parentDevice;

    m_treeWidget->clear();

    for (DeviceListIterator it = devices->begin(); it != devices->end(); ++it) {

        if ((*it)->getType() != Device::Midi){
            continue;
        }

        midiDevice = dynamic_cast<MidiDevice*>(*it);
        if (!midiDevice){
            continue;
        }

        // skip read-only devices
        if (midiDevice->getDirection() == MidiDevice::Record){
            continue;
        }

        QString itemName = strtoqstr(midiDevice->getName());

        RG_DEBUG << "BankEditorDialog::updateDialog - adding "
        << itemName;

        QTreeWidgetItem* deviceItem =
            new MidiDeviceTreeWidgetItem
            (midiDevice, m_treeWidget, itemName);

        deviceItem->setExpanded(true);

        populateDeviceItem(deviceItem, midiDevice);

    }// end for( it = device ...)

    m_treeWidget->blockSignals(false);

    // select item
    RG_DEBUG << "selecting item:" << (int)selectedType << selectedName <<
        parentDevice;
    // find the device item
    MidiDeviceTreeWidgetItem* selectDeviceItem = nullptr;
    QTreeWidgetItem* root = m_treeWidget->invisibleRootItem();
    for (int i=0; i<root->childCount(); i++) {
        QTreeWidgetItem* item = root->child(i);
        MidiDeviceTreeWidgetItem* deviceItem =
            dynamic_cast<MidiDeviceTreeWidgetItem *>(item);
        if (! deviceItem) continue;
        if (deviceItem->getDevice() == parentDevice) {
            // found the device item
            selectDeviceItem = deviceItem;
            break;
        }
    }

    if (selectDeviceItem == nullptr) {
        // the device is gone - no selection
        return;
    }

    if (selectedType == SelectedType::DEVICE) {
        // the device itself is selected
        m_treeWidget->setCurrentItem(selectDeviceItem);
        return;
    }

    if (selectedType == SelectedType::BANK ||
        selectedType == SelectedType::KEYMAP) {
        int childCount = selectDeviceItem->childCount();
        for(int i=0; i<childCount; i++) {
            QTreeWidgetItem* cItem = selectDeviceItem->child(i);
            MidiKeyMapTreeWidgetItem *keyItem =
                dynamic_cast<MidiKeyMapTreeWidgetItem*>(cItem);
            if (keyItem && selectedType == SelectedType::KEYMAP) {
                QString cName = keyItem->getName();
                if (cName == selectedName) {
                    // found it
                    RG_DEBUG << "updateDialog setCurrent keymap" << cName;
                    m_treeWidget->setCurrentItem(cItem);
                    return;
                }
            }
            MidiBankTreeWidgetItem *bankItem =
                dynamic_cast<MidiBankTreeWidgetItem*>(cItem);
            if (bankItem && selectedType == SelectedType::BANK) {
                QString cName = bankItem->getName();
                if (cName == selectedName) {
                    // found it
                    RG_DEBUG << "updateDialog setCurrent bank" << cName;
                    m_treeWidget->setCurrentItem(cItem);
                    return;
                }
            }
        }
        // no suitable child item found - select device
        RG_DEBUG << "updateDialog setCurrent device" <<
            selectDeviceItem->getName();
        m_treeWidget->setCurrentItem(selectDeviceItem);
    }
}

void
BankEditorDialog::setCurrentDevice(DeviceId device)
{
    unsigned int i, cnt;

    cnt = m_treeWidget->topLevelItemCount();
    for( i = 0; i < cnt; i++ ){
        QTreeWidgetItem *twItem = m_treeWidget->topLevelItem( i );
        MidiDeviceTreeWidgetItem *deviceItem = dynamic_cast<MidiDeviceTreeWidgetItem *>(twItem);
        if (deviceItem && deviceItem->getDevice()->getId() == device) {
//             m_treeWidget->setSelected(item, true);
            m_treeWidget->setCurrentItem(twItem);
            break;
        }
    }
}

void
BankEditorDialog::populateDeviceItem(QTreeWidgetItem* deviceItem, MidiDevice* midiDevice)
{
    clearItemChildren(deviceItem);

    QString itemName = strtoqstr(midiDevice->getName());

    BankList banks = midiDevice->getBanks();
    // add banks for this device
    for (size_t i = 0; i < banks.size(); ++i) {
        RG_DEBUG << "BankEditorDialog::populateDeviceItem - adding "
                 << itemName << " - " << strtoqstr(banks[i].getName());
        new MidiBankTreeWidgetItem(midiDevice, i, deviceItem,
                                 strtoqstr(banks[i].getName()),
                                 banks[i].isPercussion(),
                                 banks[i].getMSB(), banks[i].getLSB());
    }

    const KeyMappingList &mappings = midiDevice->getKeyMappings();
    for (size_t i = 0; i < mappings.size(); ++i) {
        RG_DEBUG << "BankEditorDialog::populateDeviceItem - adding key mapping "
                 << itemName << " - " << strtoqstr(mappings[i].getName());
        new MidiKeyMapTreeWidgetItem(midiDevice, deviceItem,
                                   strtoqstr(mappings[i].getName()));
    }
}

void
BankEditorDialog::clearItemChildren(QTreeWidgetItem* item)
{
//     QTreeWidgetItem* child = 0;

//     while ((child = item->child(0)))
//         delete child;
    while ((item->childCount() > 0))
        delete item->child(0);
}

void BankEditorDialog::slotPopulateDeviceEditors(QTreeWidgetItem* item, QTreeWidgetItem*)
{
    RG_DEBUG << "BankEditorDialog::slotPopulateDeviceEditors";

    if (!item)
        return ;

    populateDeviceEditors(item);
}


void BankEditorDialog::populateDeviceEditors(QTreeWidgetItem* item)
{
    /**
    *   shows the program and bank editors
    *   and calls their populate( currentBank ) functions
    **/
    RG_DEBUG << "BankEditorDialog::populateDeviceEditors \n";

    if (!item)
        return ;

    MidiKeyMapTreeWidgetItem *keyItem =
        dynamic_cast<MidiKeyMapTreeWidgetItem *>(item);

    if (keyItem) {

        enterActionState("on_key_item");
        leaveActionState("on_bank_item");

        m_delete->setEnabled(true);
        m_copyPrograms->setEnabled(true);

        m_pastePrograms->setEnabled(m_clipboard.itemType == ItemType::KEYMAP);

        MidiDevice *device = keyItem->getDevice();
        if (!device)
            return ;

        m_keyMappingEditor->populate(item);

        m_programEditor->hide();
        m_keyMappingEditor->show();

        m_rightSide->setEnabled(true);

        return ;
    }

    MidiBankTreeWidgetItem* bankItem =
        dynamic_cast<MidiBankTreeWidgetItem*>(item);

    if (bankItem) {

        enterActionState("on_bank_item");
        leaveActionState("on_key_item");

        m_delete->setEnabled(true);
        m_copyPrograms->setEnabled(true);

        m_pastePrograms->setEnabled(m_clipboard.itemType == ItemType::BANK);

        MidiDevice *device = bankItem->getDevice();
        if (!device)
            return ;

        m_variationToggle->blockSignals(true);
        m_variationToggle->setChecked(device->getVariationType() !=
                                      MidiDevice::NoVariations);
        m_variationToggle->blockSignals(false);

        m_variationCombo->setEnabled(m_variationToggle->isChecked());

        m_variationCombo->blockSignals(true);
        m_variationCombo->setCurrentIndex
            (device->getVariationType() ==
             MidiDevice::VariationFromLSB ? 0 : 1);
        m_variationCombo->blockSignals(false);

        m_programEditor->populate(item);

        m_keyMappingEditor->hide();
        m_programEditor->show();

        m_rightSide->setEnabled(true);

        return ;
    }

    // Device, not bank or key mapping
    // Ensure we fill these lists for the new device
    //
    MidiDeviceTreeWidgetItem* deviceItem = getParentDeviceItem(item);
    if (!deviceItem) {
        RG_DEBUG << "BankEditorDialog::populateDeviceEditors - got no deviceItem (banks parent item) \n";
        return ;
    }

    MidiDevice *device = deviceItem->getDevice();
    if (!device) {
        RG_DEBUG << "BankEditorDialog::populateDeviceEditors - no device for this item\n";
        return ;
    }

    RG_DEBUG << "BankEditorDialog::populateDeviceEditors : not a bank item - disabling";
    m_delete->setEnabled(false);
    m_copyPrograms->setEnabled(false);
    m_pastePrograms->setEnabled(false);
    m_rightSide->setEnabled(false);

    m_variationToggle->setChecked(device->getVariationType() !=
                                  MidiDevice::NoVariations);
    m_variationCombo->setEnabled(m_variationToggle->isChecked());
    m_variationCombo->setCurrentIndex
                (device->getVariationType() ==
                     MidiDevice::VariationFromLSB ? 0 : 1);

    leaveActionState("on_bank_item");
    leaveActionState("on_key_item");

    m_programEditor->clearAll();
    m_keyMappingEditor->clearAll();
}

MidiDeviceTreeWidgetItem*
BankEditorDialog::getParentDeviceItem(QTreeWidgetItem* item)
{
    /**
    *   return the parent t.w.Item of a bank or keymap (which is a MidiDeviceTreeWidgetItem )
    **/
    if (!item)
        return nullptr;

    if (dynamic_cast<MidiBankTreeWidgetItem*>(item)){
        // go up to the parent device item
        item = item->parent();
    }
    else if (dynamic_cast<MidiKeyMapTreeWidgetItem*>(item)){
        // go up to the parent device item
        item = item->parent();
    }

    if (!item) {
        RG_DEBUG << "BankEditorDialog::getParentDeviceItem : missing parent device item for bank item - this SHOULD NOT HAPPEN";
        return nullptr;
    }

    return dynamic_cast<MidiDeviceTreeWidgetItem*>(item);
//    QTreeWidgetItem *parent = item->parent();
//    if (!parent) {
//        // item must have been a top level widget
//        parent = item;
//    }
//
//    return dynamic_cast<MidiDeviceTreeWidgetItem*>(parent);
}

void
BankEditorDialog::slotAddBank()
{
    if (!m_treeWidget->currentItem())
        return ;

    QTreeWidgetItem* currentItem = m_treeWidget->currentItem();

    MidiDeviceTreeWidgetItem* deviceItem = getParentDeviceItem(currentItem);
    MidiDevice *device = deviceItem->getDevice();

    if (device) {
        QString name = "";
        int n = 0;
        while (name == "" ||
               device->getBankByName(qstrtostr(name)) != nullptr) {
            ++n;
            if (n == 1)
                name = tr("<new bank>");
            else
                name = tr("<new bank %1>").arg(n);
        }
        std::pair<int, int> bank =
            getFirstFreeBank(m_treeWidget->currentItem());

        MidiBank newBank(false,
                         bank.first, bank.second,
                         qstrtostr(name));

        BankList banks = device->getBanks();
        banks.push_back(newBank);

        RG_DEBUG <<
            "BankEditorDialog::slotAddBank() : deviceItem->getDeviceId() = " <<
            deviceItem->getDevice()->getId();
        ModifyDeviceCommand *command = makeCommand(tr("add MIDI Bank"));
        if (! command) return;
        command->setBankList(banks);
        addCommandToHistory(command);

    }
}

void
BankEditorDialog::slotAddKeyMapping()
{
    if (!m_treeWidget->currentItem())
        return ;

    QTreeWidgetItem* currentItem = m_treeWidget->currentItem();
    MidiDeviceTreeWidgetItem* deviceItem = getParentDeviceItem(currentItem);
    MidiDevice *device = deviceItem->getDevice();

    if (device) {

        QString name = "";
        int n = 0;
        while (name == "" || device->getKeyMappingByName(qstrtostr(name)) != nullptr) {
            ++n;
            if (n == 1)
                name = tr("<new mapping>");
            else
                name = tr("<new mapping %1>").arg(n);
        }

        MidiKeyMapping newKeyMapping(qstrtostr(name));

        ModifyDeviceCommand *command = makeCommand(tr("add Key Mapping"));
        if (! command) return;
        KeyMappingList kml;
        kml.push_back(newKeyMapping);
        command->setKeyMappingList(kml);
        command->setOverwrite(false);
        command->setRename(false);

        addCommandToHistory(command);

        selectDeviceItem(device);
    }
}

void
BankEditorDialog::slotDelete()
{
    if (!m_treeWidget->currentItem())
        return ;

    QTreeWidgetItem* currentItem = m_treeWidget->currentItem();

    MidiDevice* device = nullptr;
    MidiBankTreeWidgetItem* bankItem =
        dynamic_cast<MidiBankTreeWidgetItem*>(currentItem);
    if (bankItem) device = bankItem->getDevice();

    if (device && bankItem) {
        int currentBank = bankItem->getBank();
        BankList banks = device->getBanks();
        MidiBank bank = banks[currentBank];

        BankList newBanks;
        for (size_t i = 0; i < banks.size(); ++i) {
            MidiBank ibank = banks[i];
            if (! ibank.compareKey(bank)) {
                newBanks.push_back(ibank);
            }
        }

        bool used = tracksUsingBank(bank, *device);
        if (used) return;

        int reply =
            QMessageBox::warning(
              dynamic_cast<QWidget*>(this),
              "", /* no title */
              tr("Really delete this bank?"),
              QMessageBox::Yes | QMessageBox::No,
              QMessageBox::No);

        if (reply == QMessageBox::Yes) {

            // Copy across all programs that aren't in the doomed bank
            //
            ProgramList newProgramList;
            const ProgramList actualList = device->getPrograms();
            for (ProgramList::const_iterator it = actualList.begin();
                 it != actualList.end();
                 ++it) {
                // If this program isn't in the bank that is being deleted,
                // add it to the new program list.  We use compareKey()
                // because the MidiBank objects in the program list do not
                // have their name fields filled in.
                if (!it->getBank().compareKey(bank))
                    newProgramList.push_back(*it);
            }

            // Don't allow pasting from this defunct device
            // do this check before bankItem is deleted
            //
            if (m_clipboard.itemType == ItemType::BANK &&
                m_clipboard.deviceId == bankItem->getDevice()->getId() &&
                m_clipboard.bank == bankItem->getBank()) {

                m_pastePrograms->setEnabled(false);
                m_clipboard.itemType = ItemType::NONE;
                m_clipboard.deviceId = Device::NO_DEVICE;
                m_clipboard.bank = -1;
                m_clipboard.keymapName = "";
            }

            ModifyDeviceCommand *command = makeCommand(tr("delete MIDI bank"));
            if (! command) return;
            command->setBankList(newBanks);
            command->setProgramList(newProgramList);

            addCommandToHistory(command);
        }

        return ;
    }

    MidiKeyMapTreeWidgetItem* keyItem =
        dynamic_cast<MidiKeyMapTreeWidgetItem*>(currentItem);
    if (keyItem) device = keyItem->getDevice();

    if (device && keyItem) {

        int reply =
            QMessageBox::warning(
              dynamic_cast<QWidget*>(this),
              "", /* no title */
              tr("Really delete this key mapping?"),
              QMessageBox::Yes | QMessageBox::No,
              QMessageBox::No);

        if (reply == QMessageBox::Yes) {

            std::string keyMappingName = qstrtostr(keyItem->getName());

            ModifyDeviceCommand *command =
                makeCommand(tr("delete Key Mapping"));
            if (! command) return;
            KeyMappingList kml = device->getKeyMappings();

            for (KeyMappingList::iterator i = kml.begin();
                 i != kml.end(); ++i) {
                if (i->getName() == keyMappingName) {
                    RG_DEBUG << "erasing " << keyMappingName;
                    kml.erase(i);
                    break;
                }
            }

            RG_DEBUG << "setting" << kml.size() << "key mappings to device";

            command->setKeyMappingList(kml);

            addCommandToHistory(command);

            RG_DEBUG << "device has" <<
                device->getKeyMappings().size() << "key mappings now";
        }

        return ;
    }
}

void
BankEditorDialog::slotDeleteAll()
{
    if (!m_treeWidget->currentItem())
        return ;

    QTreeWidgetItem* currentIndex = m_treeWidget->currentItem();
    MidiDeviceTreeWidgetItem* deviceItem = getParentDeviceItem(currentIndex);
    MidiDevice *device = deviceItem->getDevice();
    BankList banks = device->getBanks();

    // check for used banks
    for (const MidiBank& bank : banks) {
        bool used = tracksUsingBank(bank, *device);
        if (used) return;
    }

    QString question = tr("Really delete all banks and keymaps for ") +
                       strtoqstr(device->getName()) + QString(" ?");

    int reply = QMessageBox::warning(
                  dynamic_cast<QWidget*>(this),
                  "", /* no title */
                  question,
                  QMessageBox::Yes | QMessageBox::No,
                  QMessageBox::No);

    if (reply == QMessageBox::Yes) {

        // reset copy/paste
        //
        if (m_clipboard.deviceId == device->getId()) {
            m_pastePrograms->setEnabled(false);
            m_clipboard.itemType = ItemType::NONE;
            m_clipboard.deviceId = Device::NO_DEVICE;
            m_clipboard.bank = -1;
            m_clipboard.keymapName = "";
        }

        BankList emptyBankList;
        ProgramList emptyProgramList;
        KeyMappingList emptyKeymapList;

        ModifyDeviceCommand *command = makeCommand(tr("delete all"));
        if (! command) return;
        command->setBankList(emptyBankList);
        command->setProgramList(emptyProgramList);
        command->setKeyMappingList(emptyKeymapList);

        addCommandToHistory(command);
    }
}

std::pair<int, int>
BankEditorDialog::getFirstFreeBank(QTreeWidgetItem* item)
{
    //!!! percussion? this is actually only called in the expectation
    // that percussion==false at the moment

    MidiDeviceTreeWidgetItem *deviceItem =
        dynamic_cast<MidiDeviceTreeWidgetItem*>(item);
    if (! deviceItem) return std::pair<int, int>(0, 0);
    MidiDevice* device = deviceItem->getDevice();

    BankList banks = device->getBanks();
    for (int msb = MidiMinValue; msb < MidiMaxValue; ++msb) {
        for (int lsb = MidiMinValue; lsb < MidiMaxValue; ++lsb) {
            BankList::iterator i = banks.begin();
            for (; i != banks.end(); ++i) {
                if (i->getLSB() == lsb && i->getMSB() == msb) {
                    break;
                }
            }
            if (i == banks.end())
                return std::pair<int, int>(msb, lsb);
        }
    }

    return std::pair<int, int>(0, 0);
}

void
BankEditorDialog::slotModifyDeviceOrBankName(QTreeWidgetItem* item, int)
{
    RG_DEBUG << "BankEditorDialog::slotModifyDeviceOrBankName";

    MidiDeviceTreeWidgetItem* deviceItem =
        dynamic_cast<MidiDeviceTreeWidgetItem*>(item);
    MidiBankTreeWidgetItem* bankItem =
        dynamic_cast<MidiBankTreeWidgetItem*>(item);
    MidiKeyMapTreeWidgetItem *keyItem =
        dynamic_cast<MidiKeyMapTreeWidgetItem*>(item);

    QString label = item->text(0);
    // do not allow blank names
    if (label == "") {
        updateDialog();
        return;
    }
    if (bankItem) {

        // renaming a bank item

        RG_DEBUG << "BankEditorDialog::slotModifyDeviceOrBankName - "
                 << "modify bank name to " << label;

        QTreeWidgetItem* currentItem = m_treeWidget->currentItem();
        MidiDeviceTreeWidgetItem* deviceItem =
            getParentDeviceItem(currentItem);
        MidiDevice *device = deviceItem->getDevice();
        if (! device) return;
        int bankIndex = bankItem->getBank();
        BankList banks = device->getBanks();
        QString uniqueName = makeUniqueBankName(label, banks);
        // set m_selectionName to select this item in updateDialot
        m_selectionName = uniqueName;
        banks[bankIndex].setName(qstrtostr(uniqueName));

        RG_DEBUG <<
            "slotModifyDeviceOrBankName : deviceItem->getDeviceId() = " <<
            deviceItem->getDevice()->getId();
        std::string deviceName = device->getName();
        ModifyDeviceCommand *command = makeCommand(tr("rename MIDI Bank"));
        if (! command) return;
        command->setBankList(banks);
        addCommandToHistory(command);
    } else if (keyItem) {

        RG_DEBUG << "BankEditorDialog::slotModifyDeviceOrBankName - "
        << "modify key mapping name to " << label;

        QString oldName = keyItem->getName();

        QTreeWidgetItem* currentItem = m_treeWidget->currentItem();
        MidiDeviceTreeWidgetItem* deviceItem =
            getParentDeviceItem(currentItem);
        MidiDevice *device = deviceItem->getDevice();

        if (! device) return;

        ModifyDeviceCommand *command =
            makeCommand(tr("rename Key Mapping"));
        if (! command) return;
        KeyMappingList kml = device->getKeyMappings();

        QString uniqueName = makeUniqueKeymapName(label, kml);
        // set m_selectionName to select this item in updateDialot
        m_selectionName = uniqueName;

        for (KeyMappingList::iterator i = kml.begin();
             i != kml.end(); ++i) {
            if (i->getName() == qstrtostr(oldName)) {
                i->setName(qstrtostr(uniqueName));
                break;
            }
        }

        command->setKeyMappingList(kml);

        addCommandToHistory(command);

    } else if (deviceItem) {
        // the device item is non-editable so a device rename cannot
        // happen here
    }

}

void
BankEditorDialog::selectDeviceItem(MidiDevice *device)
{
    /**
     * sets the device-TreeWidgetItem (visibly) selected
     **/
    MidiDevice *midiDevice;
    unsigned int i, cnt;

    cnt = m_treeWidget->topLevelItemCount();
    for( i=0; i<cnt; i++ ){
        QTreeWidgetItem *child = m_treeWidget->topLevelItem( i );
        MidiDeviceTreeWidgetItem *midiDeviceItem =
            dynamic_cast<MidiDeviceTreeWidgetItem*>(child);

        if (midiDeviceItem) {
            midiDevice = midiDeviceItem->getDevice();

            if (midiDevice == device) {
//                 m_treeWidget->setSelected(child, true);
                m_treeWidget->setCurrentItem(child);
                return ;
            }
        }

    }

}

QString BankEditorDialog::makeUniqueBankName(const QString& name,
                                             const BankList& banks)
{
    QString uniqueName = name;
    int suffix = 1;
    while (true) {
        bool foundName = false;
        for (size_t i = 0; i < banks.size(); ++i) {
            const MidiBank& ibank = banks[i];
            QString iName = strtoqstr(ibank.getName());
            if (uniqueName == iName) {
                // not unique
                foundName = true;
                uniqueName = QString("%1_%2").arg(name).arg(suffix);
                suffix++;
                break;
            }
        }
        if (! foundName) break; // unique
    }
    return uniqueName;
}

QString BankEditorDialog::makeUniqueKeymapName(const QString& name,
                                               const KeyMappingList& keymaps)
{
    QString uniqueName = name;
    int suffix = 1;
    while (true) {
        bool foundName = false;
        for (size_t i = 0; i < keymaps.size(); ++i) {
            const MidiKeyMapping& ikeymap = keymaps[i];
            QString iName = strtoqstr(ikeymap.getName());
            if (uniqueName == iName) {
                // not unique
                foundName = true;
                uniqueName = QString("%1_%2").arg(name).arg(suffix);
                suffix++;
                break;
            }
        }
        if (! foundName) break; // unique
    }
    return uniqueName;
}


void
BankEditorDialog::slotVariationToggled()
{
    if (!m_treeWidget->currentItem())
        return ;

    ModifyDeviceCommand *command = makeCommand(tr("variation toggled"));
    if (! command) return;
    MidiDevice::VariationType variation =
        MidiDevice::NoVariations;
    if (m_variationToggle->isChecked()) {
        if (m_variationCombo->currentIndex() == 0) {
            variation = MidiDevice::VariationFromLSB;
        } else {
            variation = MidiDevice::VariationFromMSB;
        }
    }

    command->setVariation(variation);
    addCommandToHistory(command);

    m_variationCombo->setEnabled(m_variationToggle->isChecked());
}

void
BankEditorDialog::slotVariationChanged(int)
{
    if (!m_treeWidget->currentItem())
        return ;

    ModifyDeviceCommand *command = makeCommand(tr("variation changed"));
    if (! command) return;
    MidiDevice::VariationType variation =
        MidiDevice::NoVariations;
    if (m_variationToggle->isChecked()) {
        if (m_variationCombo->currentIndex() == 0) {
            variation = MidiDevice::VariationFromLSB;
        } else {
            variation = MidiDevice::VariationFromMSB;
        }
    }

    command->setVariation(variation);
    addCommandToHistory(command);
}

ModifyDeviceCommand* BankEditorDialog::makeCommand(const QString& name)
{
    if (!m_treeWidget->currentItem())
        return nullptr;

    QTreeWidgetItem* currentItem = m_treeWidget->currentItem();

    MidiDeviceTreeWidgetItem* deviceItem = getParentDeviceItem(currentItem);
    if (!deviceItem) return nullptr;
    MidiDevice *device = deviceItem->getDevice();
    ModifyDeviceCommand *command =
        new ModifyDeviceCommand(m_studio,
                                device->getId(),
                                device->getName(),
                                device->getLibrarianName(),
                                device->getLibrarianEmail(),
                                name);
    return command;
}

void
BankEditorDialog::addCommandToHistory(Command *command)
{
    CommandHistory::getInstance()->addCommand(command);
}

void
BankEditorDialog::slotImport()
{
#if QT_VERSION >= 0x050000
    QString home = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).path();
#else
    QString home = QUrl::fromLocalFile(QDesktopServices::storageLocation(QDesktopServices::HomeLocation)).path();
#endif
    QString deviceDir = home + "/.local/share/rosegarden/library";

    QString url_str = FileDialog::getOpenFileName(this, tr("Import Banks from Device in File"), deviceDir,
                      tr("Rosegarden Device files") + " (*.rgd *.RGD)" + ";;" +
                      tr("Rosegarden files") + " (*.rg *.RG)" + ";;" +
                      tr("Sound fonts") + " (*.sf2 *.SF2)" + ";;" +
                      tr("LinuxSampler configuration files") + " (*.lscp *.LSCP)" + ";;" +
                      tr("All files") + " (*)", nullptr); ///!!! Should we allow 'All files' here since LinuxSampler files don't need to have an extension?!

    QUrl url(url_str);

    if (url.isEmpty())
        return ;

    ImportDeviceDialog *dialog = new ImportDeviceDialog(this, url);
    if (dialog->doImport() && dialog->exec() == QDialog::Accepted) {

        MidiDeviceTreeWidgetItem* deviceItem =
            dynamic_cast<MidiDeviceTreeWidgetItem*>
            (m_treeWidget->currentItem());    //### was ->selectedItem()

        if (!deviceItem) {
            QMessageBox::critical(
              dynamic_cast<QWidget*>(this),
              "", /* no title */
              tr("Some internal error: cannot locate selected device"),
              QMessageBox::Ok,
              QMessageBox::Ok);
            return ;
        }

        if (!dialog->haveDevice()) {
            QMessageBox::critical(
              dynamic_cast<QWidget*>(this),
              "", /* no title */
              tr("Some internal error: no device selected"),
              QMessageBox::Ok,
              QMessageBox::Ok);
            return ;
        }

        MidiDevice *device = deviceItem->getDevice();
        if (!device) return;

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

        // command with new device name
        ModifyDeviceCommand *command =
            new ModifyDeviceCommand(m_studio,
                                    device->getId(),
                                    dialog->getDeviceName(),
                                    librarianName,
                                    librarianEmail,
                                    tr("import device"));

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

        selectDeviceItem(device);
    }

    delete dialog;
    updateDialog();
}

void
BankEditorDialog::slotEdit(QTreeWidgetItem * item, int)
{
    RG_DEBUG << "BankEditorDialog::slotEdit()";

    if (item->flags() & Qt::ItemIsEditable)
        m_treeWidget->editItem(item);
}

void
BankEditorDialog::slotEditCopy()
{
    MidiBankTreeWidgetItem* bankItem =
        dynamic_cast<MidiBankTreeWidgetItem*>(m_treeWidget->currentItem());

    if (bankItem) {
        m_clipboard.itemType = ItemType::BANK;
        m_clipboard.deviceId = bankItem->getDevice()->getId();
        m_clipboard.bank = bankItem->getBank();
        m_clipboard.keymapName = "";
        m_pastePrograms->setEnabled(true);
    }

    MidiKeyMapTreeWidgetItem *keyItem =
        dynamic_cast<MidiKeyMapTreeWidgetItem*>(m_treeWidget->currentItem());

    if (keyItem) {
        m_clipboard.itemType = ItemType::KEYMAP;
        m_clipboard.deviceId = keyItem->getDevice()->getId();
        m_clipboard.bank = -1;
        m_clipboard.keymapName = keyItem->getName();
        m_pastePrograms->setEnabled(true);
    }
}

void
BankEditorDialog::slotEditPaste()
{
    RG_DEBUG << "slotEditPaste";
    MidiBankTreeWidgetItem* bankItem =
        dynamic_cast<MidiBankTreeWidgetItem*>(m_treeWidget->currentItem());

    if (bankItem) {

        if (m_clipboard.itemType != ItemType::BANK) return;
        // Get the full program and bank list for the source device
        //
        MidiDevice *device = bankItem->getDevice();
        BankList bankList = device->getBanks();

        ProgramList::iterator it;
        ProgramList newPrograms;
        MidiBank currentBank = bankList[bankItem->getBank()];

        ProgramList oldPrograms = device->getPrograms();

        // Remove programs that will be overwritten
        //
        RG_DEBUG << "slotEditPaste remove programs";
        for (it = oldPrograms.begin(); it != oldPrograms.end(); ++it) {
            RG_DEBUG << "slotEditPaste check remove program" << (*it).getName();
            if (!(it->getBank().compareKey(currentBank))) {
                RG_DEBUG << "slotEditPaste add program" << (*it).getName();
                newPrograms.push_back(*it);
            }
        }

        // Now get source list and msb/lsb
        //

        MidiBank sourceBank = bankList[m_clipboard.bank];
        Device *tmpDevice = m_studio->getDevice(m_clipboard.deviceId);
        if (! tmpDevice) return;
        MidiDevice *sourceDevice = dynamic_cast<MidiDevice *>(tmpDevice);
        if (! sourceDevice) return;
        ProgramList sourcePrograms = sourceDevice->getPrograms();

        // Add the new programs
        //
        RG_DEBUG << "slotEditPaste copy programs";
        for (it = sourcePrograms.begin(); it != sourcePrograms.end(); ++it) {
            RG_DEBUG << "slotEditPaste check copy program" << (*it).getName();
            if (it->getBank().compareKey(sourceBank)) {
                // Insert with new MSB and LSB
                //
                RG_DEBUG << "slotEditPaste copy program" << (*it).getName();
                MidiProgram copyProgram(currentBank,
                                        it->getProgram(),
                                        it->getName());

                newPrograms.push_back(copyProgram);
            }
        }

        ModifyDeviceCommand *command = makeCommand(tr("paste bank"));
        if (! command) return;
        command->setProgramList(newPrograms);
        addCommandToHistory(command);

        return;
    }

    MidiKeyMapTreeWidgetItem *keyItem =
        dynamic_cast<MidiKeyMapTreeWidgetItem*>(m_treeWidget->currentItem());
    if (keyItem) {

        if (m_clipboard.itemType != ItemType::KEYMAP) return;

        MidiDevice *device = keyItem->getDevice();
        QString selectedKeyItemName = keyItem->getName();
        const KeyMappingList &mappings = device->getKeyMappings();

        RG_DEBUG << "slotEditPaste paste keymap";
        Device *tmpDevice = m_studio->getDevice(m_clipboard.deviceId);
        if (! tmpDevice) return;
        MidiDevice *sourceDevice = dynamic_cast<MidiDevice *>(tmpDevice);
        if (! sourceDevice) return;
        const KeyMappingList &sourceMappings = sourceDevice->getKeyMappings();
        int sourceIndex = -1;
        for (size_t i = 0; i < sourceMappings.size(); ++i) {
            if (sourceMappings[i].getName() ==
                qstrtostr(m_clipboard.keymapName)) {
                sourceIndex = i;
                break;
            }
        }
        RG_DEBUG << "slotEditPaste sourceIndex" << sourceIndex;
        if (sourceIndex == -1) return;
        MidiKeyMapping sourceMap = sourceMappings[sourceIndex];
        // keep the old name
        sourceMap.setName(qstrtostr(selectedKeyItemName));

        KeyMappingList newKeymapList;
        for (size_t i = 0; i < mappings.size(); ++i) {
            if (mappings[i].getName() == qstrtostr(selectedKeyItemName)) {
                RG_DEBUG << "slotEditPaste add new keymap" << i;
                newKeymapList.push_back(sourceMap);
            } else {
                RG_DEBUG << "slotEditPaste add old keymap" << i;
                newKeymapList.push_back(mappings[i]);
            }
        }

        ModifyDeviceCommand *command = makeCommand(tr("paste keymap"));
        if (! command) return;
        command->setKeyMappingList(newKeymapList);
        addCommandToHistory(command);

        return;
    }
}

void BankEditorDialog::slotEditLibrarian()
{
    RG_DEBUG << "slotEditLibrarian";
    if (!m_treeWidget->currentItem())
        return;

    QTreeWidgetItem* currentItem = m_treeWidget->currentItem();

    MidiDeviceTreeWidgetItem* deviceItem = getParentDeviceItem(currentItem);
    if (!deviceItem) return;
    MidiDevice *device = deviceItem->getDevice();
    QString name = strtoqstr(device->getLibrarianName());
    QString mail = strtoqstr(device->getLibrarianEmail());
    LibrarianDialog dlg(this, name, mail);

    if (dlg.exec() != QDialog::Accepted) return;
    RG_DEBUG << "accepted";

    QString newName;
    QString newMail;
    dlg.getLibrarian(newName, newMail);
    if (newName == "") newName = "<none>";
    if (newMail == "") newMail = "<none>";
    RG_DEBUG << "librarian" << name << mail  << "->" <<
        newName << newMail;
    if (name == newName && mail == newMail) {
        // no change
        RG_DEBUG << "librarian unchanged";
        return;
    }
    ModifyDeviceCommand *command =
        new ModifyDeviceCommand(m_studio,
                                device->getId(),
                                device->getName(),
                                qstrtostr(newName),
                                qstrtostr(newMail),
                                tr("change librarian"));
    addCommandToHistory(command);
}

void
BankEditorDialog::slotExport()
{
    QString extension = "rgd";

/*
 * Qt4:
 * QString getSaveFileName (QWidget * parent = 0, const QString & caption =
 * QString(), const QString & dir = QString(), const QString & filter =
 * QString(), QString * selectedFilter = 0, Options options = 0)
 *
 * KDE3:
 * QString KFileDialog::getSaveFileName   ( const QString &   startDir =
 * QString::null,
 *   const QString &   filter = QString::null,
 *     QWidget *   parent = 0,
 *       const QString &   caption = QString::null
 *        )   [static]
 *
 */

    QString dir = ResourceFinder().getResourceSaveDir("library");

    QString name =
        FileDialog::getSaveFileName(this,
                                     tr("Export Device as..."),
                                     dir,
                                     (extension.isEmpty() ? QString("*") : ("*." + extension)));

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
        QMessageBox::warning(
          dynamic_cast<QWidget*>(this),
          "", /* no title */
          tr("You have specified a directory"),
          QMessageBox::Ok,
          QMessageBox::Ok);
        return ;
    }

    if (info.exists()) {
        int overwrite = QMessageBox::question(
                          dynamic_cast<QWidget*>(this),
                          "", /* no title */
                          tr("The specified file exists.  Overwrite?"),
                          QMessageBox::Yes | QMessageBox::No,
                          QMessageBox::No);

        if (overwrite != QMessageBox::Yes)
            return ;
    }

    MidiDeviceTreeWidgetItem* deviceItem =
            dynamic_cast<MidiDeviceTreeWidgetItem*>
                (m_treeWidget->currentItem());

    std::vector<DeviceId> devices;
    MidiDevice *md = deviceItem->getDevice();

    if (md) {
        ExportDeviceDialog *ed = new ExportDeviceDialog
                                 (this, strtoqstr(md->getName()));
        if (ed->exec() != QDialog::Accepted)
            return ;
        if (ed->getExportType() == ExportDeviceDialog::ExportOne) {
            devices.push_back(md->getId());
        }
    }

    QString errMsg;
    if (!m_doc->exportStudio(name, errMsg, devices)) {
        if (errMsg != "") {
            QMessageBox::critical
                (nullptr, tr("Rosegarden"), tr(QString("Could not export studio to file at %1\n(%2)")
                           .arg(name).arg(errMsg).toStdString().c_str()));
        } else {
            QMessageBox::critical
                (nullptr, tr("Rosegarden"), tr(QString("Could not export studio to file at %1")
                           .arg(name).toStdString().c_str()));
        }
    }
}

void
BankEditorDialog::slotFileClose()
{
    RG_DEBUG << "BankEditorDialog::slotFileClose()\n";

    if (m_observingStudio) {
        m_observingStudio = false;
        m_studio->removeObserver(this);
    }
    for(Device* device : m_observedDevices) {
        unobserveDevice(device);
    }

    // We need to do this because we might be here due to a
    // documentAboutToChange signal, in which case the document won't
    // be valid by the time we reach the dtor, since it will be
    // triggered when the closeEvent is actually processed.
    //
//     CommandHistory::getInstance()->detachView(actionCollection());    //&&&
    m_doc = nullptr;
    close();
}

void
BankEditorDialog::closeEvent(QCloseEvent *e)
{
    emit closing();
    QMainWindow::closeEvent(e);
}


void
BankEditorDialog::slotHelpRequested()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:bankEditorDialog-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:bankEditorDialog-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}

void
BankEditorDialog::slotHelpAbout()
{
    new AboutDialog(this);
}

bool BankEditorDialog::tracksUsingBank(const MidiBank& bank,
                                       const MidiDevice& device)
{
    QString bankName = strtoqstr(bank.getName());
    RG_DEBUG << "tracksUsingBank" << bankName << device.getId();
    std::vector<int> trackPositions;

    Composition &composition =
            RosegardenDocument::currentDocument->getComposition();
    const Composition::trackcontainer &tracks = composition.getTracks();

    // For each Track in the Composition...
    for (const Composition::trackcontainer::value_type &pair : tracks) {
        const Track *track = pair.second;
        if (!track)
            continue;

        const InstrumentId instrumentID = track->getInstrument();
        const Instrument *instrument = m_studio->getInstrumentById(instrumentID);
        if (!instrument)
            continue;
        if (instrument->getType() != Instrument::Midi)
            continue;

        Device *idevice = instrument->getDevice();
        // if the bank is on a different device ignore it
        if (idevice->getId() != device.getId()) continue;

        const MidiProgram& program = instrument->getProgram();
        const MidiBank& ibank = program.getBank();
        if (bank.compareKey(ibank)) {
            // Found a Track using this bank
            trackPositions.push_back(track->getPosition());
        }
    }

    // If there are Tracks using this Bank, issue a message and return true
    if (!trackPositions.empty()) {
        QString msg =
            QString(tr("The following tracks are using bank %1:")).
            arg(bankName);
        msg += '\n';
        for (const int &trackPos : trackPositions) {
            msg += QString::number(trackPos + 1) + " ";
        }
        msg += '\n';
        msg += tr("The bank cannot be deleted.");
        QMessageBox::warning(
                this,
                tr("Rosegarden"),
                msg);
        return true;
    }
    return false;
}

void BankEditorDialog::deviceAdded(Device* device)
{
    RG_DEBUG << "deviceAdded" << device;
    observeDevice(device);
    updateDialog();
}

void BankEditorDialog::deviceRemoved(Device* device)
{
    RG_DEBUG << "deviceRemoved" << device;
    unobserveDevice(device);
    updateDialog();
}

void BankEditorDialog::deviceModified(Device* device)
{
    RG_DEBUG << "deviceModified" << device;
    updateDialog();
}

void BankEditorDialog::observeDevice(Device* device)
{
    RG_DEBUG << "observeDevice" << device;
    if (m_observedDevices.find(device) != m_observedDevices.end()) return;
    m_observedDevices.insert(device);
    device->addObserver(this);
}

void BankEditorDialog::unobserveDevice(Device* device)
{
    RG_DEBUG << "unobserveDevice" << device;
    if (m_observedDevices.find(device) == m_observedDevices.end()) return;
    m_observedDevices.erase(device);
    device->removeObserver(this);
}

}
