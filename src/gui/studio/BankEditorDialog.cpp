/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[BankEditorDialog]"

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
#include "base/Studio.h"
#include "commands/studio/ModifyDeviceCommand.h"
#include "document/CommandHistory.h"
#include "document/RosegardenDocument.h"
#include "misc/ConfigGroups.h"
#include "gui/dialogs/ExportDeviceDialog.h"
#include "gui/dialogs/ImportDeviceDialog.h"
#include "gui/widgets/TmpStatusMsg.h"
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
#include <QShortcut>
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
        m_doc(doc),
        m_copyBank(Device::NO_DEVICE, -1),
        m_keepBankList(false),
        m_deleteAllReally(false),
        m_lastDevice(Device::NO_DEVICE),
        m_updateDeviceList(false)
{
    QWidget* mainFrame = new QWidget(this);
    mainFrame->setContentsMargins(1, 1, 1, 1);
    setCentralWidget(mainFrame);
    QVBoxLayout *mainFrameLayout = new QVBoxLayout;
    mainFrameLayout->setMargin(0);
    mainFrameLayout->setSpacing(2);
    mainFrame->setLayout(mainFrameLayout);

    setWindowTitle(tr("Manage MIDI Banks and Programs"));

    QWidget *splitter = new QWidget;
    QHBoxLayout *splitterLayout = new QHBoxLayout;
    splitterLayout->setMargin(0);
    splitter->setLayout(splitterLayout);

    mainFrameLayout->addWidget(splitter);

    //
    // Left-side list view
    //
    QWidget *leftPart = new QWidget;
    QVBoxLayout *leftPartLayout = new QVBoxLayout;
    leftPartLayout->setMargin(2);
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
    m_copyPrograms->setToolTip(tr("Copy all Program names from current Bank to clipboard"));

    m_pastePrograms->setToolTip(tr("Paste Program names from clipboard to current Bank"));

//     connect(m_treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*)),
//             this, SLOT(slotPopulateDeviceEditors(QTreeWidgetItem*)));
    // note: above connection moved to setupActions

    // Right side layout
    m_rightSide = new QFrame;

    m_rightSide->setContentsMargins(8, 8, 8, 8);
    QVBoxLayout *rightSideLayout = new QVBoxLayout;
    rightSideLayout->setMargin(0);
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
    optionBoxLayout->setMargin(0);
    rightSideLayout->addWidget(m_optionBox);

    QWidget *variationBox = new QWidget(m_optionBox);
    QHBoxLayout *variationBoxLayout = new QHBoxLayout;
    variationBoxLayout->setMargin(4);
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

    connect(m_treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int )),
            this, SLOT(slotEdit(QTreeWidgetItem*, int)));

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

//     CommandHistory::getInstance()->attachView(actionCollection());    //&&&
    
    connect(CommandHistory::getInstance(), SIGNAL(commandExecuted()),
            this, SLOT(slotUpdate()));

    // Button box
    QDialogButtonBox *btnBox = new QDialogButtonBox(/*QDialogButtonBox::Apply  | */
                                                    QDialogButtonBox::Reset  |
                                                    QDialogButtonBox::Close);

    mainFrameLayout->addWidget(btnBox);

    m_closeButton = btnBox->button(QDialogButtonBox::Close);
    m_resetButton = btnBox->button(QDialogButtonBox::Reset);

    connect(m_resetButton, SIGNAL(clicked()), this, SLOT(slotReset()));

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

//     m_treeWidget->saveLayout(BankEditorConfigGroup);    //&&&

//     if (m_doc) // see slotFileClose() for an explanation on why we need to test m_doc
//         CommandHistory::getInstance()->detachView(actionCollection());
}

void
BankEditorDialog::setupActions()
{
//     KAction* close = KStandardAction::close (this, SLOT(slotFileClose()), actionCollection());
    
    createAction("file_close", SLOT(slotFileClose()));

    connect(m_closeButton, SIGNAL(clicked()), this, SLOT(slotFileClose()));

    createAction("edit_copy", SLOT(slotEditCopy()));
    createAction("edit_paste", SLOT(slotEditPaste()));
    createAction("bank_help", SLOT(slotHelpRequested()));
    createAction("help_about_app", SLOT(slotHelpAbout()));

    
//     connect(m_treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*)),
//             this, SLOT(slotPopulateDeviceEditors(QTreeWidgetItem*)));

//     connect( m_treeWidget, SIGNAL(itemActivated(QTreeWidgetItem*, int)),  //twItem, column
//              this, SLOT(slotPopulateDeviceEditors(QTreeWidgetItem*, int))  );
    
    
//     currentItemChanged( QTreeWidgetItem* current, QTreeWidgetItem* previous )
    connect( m_treeWidget, SIGNAL(currentItemChanged( QTreeWidgetItem*, QTreeWidgetItem*)),  //twItemCurr, teItemPrev
             this, SLOT(slotPopulateDeviceEditors(QTreeWidgetItem*, QTreeWidgetItem*))  );
    
    connect(m_treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, 
            SLOT(slotModifyDeviceOrBankName(QTreeWidgetItem*, int)));
    
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
    m_deviceNameMap.clear();
    m_treeWidget->clear();

    // Fill list view
    //
    MidiDevice* midiDevice = 0;
    QTreeWidgetItem* twItemDevice = 0;
    DeviceList *devices = m_studio->getDevices();
    DeviceListIterator it;
    Device *devx;
//     unsigned int i = 0;

    // iterates over devices and create device-TreeWidgetItems (level: topLevelItem)
    // then calls populateDeviceItem() to create bank-TreeWidgetItems (level: topLevelItem-child)
    for (it = devices->begin(); it != devices->end(); ++it) {
        devx = *it;
//     for ( i=0; i < int(devices->size()); i++ ){
//         devx = devices->at( i );
        
        if (devx->getType() == Device::Midi) {
            
            midiDevice = dynamic_cast<MidiDevice*>(devx);
            
            if (!midiDevice) continue;
            // skip read-only devices
            if (midiDevice->getDirection() == MidiDevice::Record) continue;

            m_deviceNameMap[midiDevice->getId()] = midiDevice->getName();
            QString itemName = strtoqstr(midiDevice->getName());

            RG_DEBUG << "BankEditorDialog::initDialog - adding " << itemName;

            twItemDevice = new MidiDeviceTreeWidgetItem(midiDevice->getId(), m_treeWidget, itemName);
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
    // Update list view
    //
    m_treeWidget->blockSignals(true);
    DeviceList *devices = m_studio->getDevices();
    DeviceListIterator it;
    bool deviceLabelUpdate = false;
    
    MidiDevice* midiDevice = 0;
    QTreeWidgetItem* currentIndex = 0;
    MidiDeviceTreeWidgetItem* deviceItem = 0;
    QTreeWidgetItem *child = 0;
    unsigned int cnt, i;
    
    for (it = devices->begin(); it != devices->end(); ++it) {

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

        if (m_deviceNameMap.find(midiDevice->getId()) != m_deviceNameMap.end()) {
            // Device already displayed but make sure the label is up to date
            //
            
            //QTreeWidgetItem* 
            currentIndex = m_treeWidget->currentItem();
            
            if (currentIndex) {
                //MidiDeviceTreeWidgetItem* 
                deviceItem = getParentDeviceItem(currentIndex);
                
                if (deviceItem &&
                        deviceItem->getDeviceId() == midiDevice->getId()) {
                    
                    if( deviceItem->text(0) != strtoqstr(midiDevice->getName()) ){
                        
                        deviceItem->setText(0, strtoqstr(midiDevice->getName()));
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

                    //QTreeWidgetItem *
                    //child = deviceItem->child(0);   // bank/keymap item
                    cnt = deviceItem->childCount();
                    
                    for( i=0; i<cnt; i++ ){
                        
                        //QTreeWidgetItem*
                        child = deviceItem->child( i );

                        MidiBankTreeWidgetItem *bankItem =
                            dynamic_cast<MidiBankTreeWidgetItem *>(child);

                        if (bankItem) {
                            bool percussion = bankItem->isPercussion();
                            int msb = bankItem->text(2).toInt();
                            int lsb = bankItem->text(3).toInt();
                            std::string bankName =
                                midiDevice->getBankName( MidiBank(percussion, msb, lsb) );
                            if (bankName != "" &&
                                    bankItem->text(0) != strtoqstr(bankName)) {
                                bankItem->setText(0, strtoqstr(bankName));
                            }
                        }

                    }// end for(i)
                    
                }
            }// end if( currentIndex )

            continue;
        }
        else
        {

        m_deviceNameMap[midiDevice->getId()] = midiDevice->getName();
        QString itemName = strtoqstr(midiDevice->getName());

        RG_DEBUG << "BankEditorDialog::updateDialog - adding "
        << itemName << endl;

        QTreeWidgetItem* deviceItem = new MidiDeviceTreeWidgetItem
                                    (midiDevice->getId(), m_treeWidget, itemName);
        
        deviceItem->setExpanded(true);

        populateDeviceItem(deviceItem, midiDevice);
        }
        
    }// end for( it = device ...)
    
    
    // delete items whose corresponding devices are no longer present,
    // and update the other ones
    //
    std::vector<MidiDeviceTreeWidgetItem*> itemsToDelete;

    MidiDeviceTreeWidgetItem* sibling = dynamic_cast<MidiDeviceTreeWidgetItem*>
                                      (m_treeWidget->topLevelItem(0));

//     while (sibling) {
    cnt = m_treeWidget->topLevelItemCount();
    for( i=0; i<cnt; i++ ){
        sibling = dynamic_cast<MidiDeviceTreeWidgetItem*>( m_treeWidget->topLevelItem(i) );
        
        if (m_deviceNameMap.find(sibling->getDeviceId()) == m_deviceNameMap.end())
            itemsToDelete.push_back(sibling);
        else
            updateDeviceItem(sibling);

    }

    for (size_t i = 0; i < itemsToDelete.size(); ++i)
        delete itemsToDelete[i];

//    m_treeWidget->sortItems(0, Qt::AscendingOrder);    // column, order  // sort by device (column 0)
//     m_treeWidget->sortChildren();

    m_treeWidget->blockSignals(false);
    if (deviceLabelUpdate)
        emit deviceNamesChanged();
}

void
BankEditorDialog::setCurrentDevice(DeviceId device)
{
    unsigned int i, cnt;
    QTreeWidgetItem* twItem;
    MidiDeviceTreeWidgetItem* deviceItem;
    
    cnt = m_treeWidget->topLevelItemCount();
    for( i = 0; i < cnt; i++ ){
        twItem = m_treeWidget->topLevelItem( i );
        deviceItem = dynamic_cast<MidiDeviceTreeWidgetItem *>(twItem);
        if (deviceItem && deviceItem->getDeviceId() == device) {
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
        << itemName << " - " << strtoqstr(banks[i].getName())
        << endl;
        new MidiBankTreeWidgetItem(midiDevice->getId(), i, deviceItem,
                                 strtoqstr(banks[i].getName()),
                                 banks[i].isPercussion(),
                                 banks[i].getMSB(), banks[i].getLSB());
    }

    const KeyMappingList &mappings = midiDevice->getKeyMappings();
    for (size_t i = 0; i < mappings.size(); ++i) {
        RG_DEBUG << "BankEditorDialog::populateDeviceItem - adding key mapping "
        << itemName << " - " << strtoqstr(mappings[i].getName())
        << endl;
        new MidiKeyMapTreeWidgetItem(midiDevice->getId(), deviceItem,
                                   strtoqstr(mappings[i].getName()));
    }
}

void
BankEditorDialog::updateDeviceItem(MidiDeviceTreeWidgetItem* deviceItem)
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
    for (size_t i = 0; i < banks.size(); ++i) {
        if (deviceItemHasBank(deviceItem, i))
            continue;

        RG_DEBUG << "BankEditorDialog::updateDeviceItem - adding "
        << itemName << " - " << strtoqstr(banks[i].getName())
        << endl;
        new MidiBankTreeWidgetItem(midiDevice->getId(), i, deviceItem,
                                 strtoqstr(banks[i].getName()),
                                 banks[i].isPercussion(),
                                 banks[i].getMSB(), banks[i].getLSB());
    }
    
    int cnt, i, n;
    
    cnt = int(keymaps.size());
    for ( i = 0; i < cnt; ++i) {

//         QTreeWidgetItem *child = deviceItem->firstChild();
        bool have = false;
        
        n = 0;
        while (n < deviceItem->childCount()) {
            QTreeWidgetItem *child = deviceItem->child(n);
            
            MidiKeyMapTreeWidgetItem *keyItem =
                dynamic_cast<MidiKeyMapTreeWidgetItem*>(child);
            if (keyItem) {
                if (keyItem->getName() == strtoqstr(keymaps[i].getName())) {
                    have = true;
                }
            }
            n += 1;
        }

        if (have)
            continue;

        RG_DEBUG << "BankEditorDialog::updateDeviceItem - adding "
        << itemName << " - " << strtoqstr(keymaps[i].getName())
        << endl;
        new MidiKeyMapTreeWidgetItem(midiDevice->getId(), deviceItem,
                                   strtoqstr(keymaps[i].getName()));
    }

    // delete banks which are no longer present
    //
    std::vector<QTreeWidgetItem*> childrenToDelete;


    n = 0;
    while (n < deviceItem->childCount()) {
        
        QTreeWidgetItem* child = deviceItem->child(n);
        
        MidiBankTreeWidgetItem *bankItem =
            dynamic_cast<MidiBankTreeWidgetItem *>(child);
        if (bankItem) {
            if (bankItem->getBank() >= int(banks.size()))
                childrenToDelete.push_back(child);
            else { // update the banks MSB/LSB which might have changed
                bankItem->setPercussion(banks[bankItem->getBank()].isPercussion());
                bankItem->setMSB(banks[bankItem->getBank()].getMSB());
                bankItem->setLSB(banks[bankItem->getBank()].getLSB());
            }
        }

        MidiKeyMapTreeWidgetItem *keyItem =
            dynamic_cast<MidiKeyMapTreeWidgetItem *>(child);
        if (keyItem) {
            if (!midiDevice->getKeyMappingByName(qstrtostr(keyItem->getName()))) {
                childrenToDelete.push_back(child);
            }
        }

//         child = child->nextSibling();
        n += 1;
    }

    for (size_t i = 0; i < childrenToDelete.size(); ++i)
        delete childrenToDelete[i];
}

bool
BankEditorDialog::deviceItemHasBank(MidiDeviceTreeWidgetItem* deviceItem, int bankNb)
{
//     QTreeWidgetItem *child = deviceItem->firstChild();
    MidiBankTreeWidgetItem *bankItem = 0;
    QTreeWidgetItem *child = 0;
    
    int n = 0;
    while (n < deviceItem->childCount()) {
        child = deviceItem->child(n);
        bankItem = dynamic_cast<MidiBankTreeWidgetItem*>(child);
        if (bankItem) {
            if (bankItem->getBank() == bankNb)
                return true;
        }
//         child = child->nextSibling();
        n += 1;
    }

    return false;
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

MidiDevice*
BankEditorDialog::getCurrentMidiDevice()
{
    return getMidiDevice(m_treeWidget->currentItem());
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

    MidiKeyMapTreeWidgetItem *keyItem = dynamic_cast<MidiKeyMapTreeWidgetItem *>(item);

    if (keyItem) {

        enterActionState("on_key_item");
        leaveActionState("on_bank_item");

        m_delete->setEnabled(true);

        MidiDevice *device = getMidiDevice(keyItem->getDeviceId());
        if (!device)
            return ;

        setProgramList(device);

        m_lastDevice = keyItem->getDeviceId();
        m_keyMappingEditor->populate(item);

        m_programEditor->hide();
        m_keyMappingEditor->show();

        m_rightSide->setEnabled(true);

        return ;
    }

    MidiBankTreeWidgetItem* bankItem = dynamic_cast<MidiBankTreeWidgetItem*>(item);

    if (bankItem) {

        enterActionState("on_bank_item");
        leaveActionState("on_key_item");

        m_delete->setEnabled(true);
        m_copyPrograms->setEnabled(true);

        m_rightSide->setEnabled(true);

        if (m_copyBank.first != Device::NO_DEVICE) m_pastePrograms->setEnabled(true);

        MidiDevice *device = getMidiDevice(bankItem->getDeviceId());
        if (!device)
            return ;

        if (!m_keepBankList || m_bankList.size() == 0) {
            m_bankList = device->getBanks();
        } else {
            m_keepBankList = false;
        }

        setProgramList(device);

        m_variationToggle->setChecked(device->getVariationType() !=
                                      MidiDevice::NoVariations);
        m_variationCombo->setEnabled(m_variationToggle->isChecked());
        m_variationCombo->setCurrentIndex
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
    MidiDeviceTreeWidgetItem* deviceItem = getParentDeviceItem(item);
    if (!deviceItem) {
        RG_DEBUG << "BankEditorDialog::populateDeviceEditors - got no deviceItem (banks parent item) \n";
        return ;
    }
    

    m_lastDevice = deviceItem->getDeviceId();

    MidiDevice *device = getMidiDevice(deviceItem);
    if (!device) {
        RG_DEBUG << "BankEditorDialog::populateDeviceEditors - no device for this item\n";
        return ;
    }

    // User must have selected a device (top level Widget)
    // Fetch The chosen device and info.
    // Deactivate the rightside panel and clear its contents.
    m_bankList = device->getBanks();
    setProgramList(device);

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
            m_deleteAllReally == true) {
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
            if (m_variationCombo->currentIndex() == 0) {
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

        MidiKeyMapTreeWidgetItem *keyItem = dynamic_cast<MidiKeyMapTreeWidgetItem*>
                                          (m_treeWidget->currentItem());
        if (keyItem) {
            KeyMappingList kml(device->getKeyMappings());
            for (size_t i = 0; i < kml.size(); ++i) {
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

}

void
BankEditorDialog::slotReset()
{
    resetProgramList();

    m_programEditor->reset();
    m_programEditor->populate(m_treeWidget->currentItem());
    m_keyMappingEditor->reset();
    m_keyMappingEditor->populate(m_treeWidget->currentItem());

    MidiDeviceTreeWidgetItem* deviceItem = getParentDeviceItem
                                         (m_treeWidget->currentItem());

    if (deviceItem) {
        MidiDevice *device = getMidiDevice(deviceItem);
        m_variationToggle->setChecked(device->getVariationType() !=
                                      MidiDevice::NoVariations);
        m_variationCombo->setEnabled(m_variationToggle->isChecked());
        m_variationCombo->setCurrentIndex
        (device->getVariationType() ==
         MidiDevice::VariationFromLSB ? 0 : 1);
    }

    updateDialog();
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

MidiDeviceTreeWidgetItem*
BankEditorDialog::getParentDeviceItem(QTreeWidgetItem* item)
{
    /**
    *   return the parent t.w.Item of a bank or keymap (which is a MidiDeviceTreeWidgetItem )
    **/
    if (!item)
        return 0;

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
        return 0;
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

    QTreeWidgetItem* currentIndex = m_treeWidget->currentItem();

    MidiDeviceTreeWidgetItem* deviceItem = getParentDeviceItem(currentIndex);
    MidiDevice *device = getMidiDevice(currentIndex);

    if (device) {
        // If the bank and program lists are empty then try to
        // populate them.
        //
        if (m_bankList.size() == 0 && m_programList.size() == 0) {
            m_bankList = device->getBanks();
            setProgramList(device);
        }

        QString name = "";
        int n = 0;
        while (name == "" || device->getBankByName(qstrtostr(name)) != 0) {
            ++n;
            if (n == 1)
                name = tr("<new bank>");
            else
                name = tr("<new bank %1>").arg(n);
        }
        std::pair<int, int> bank = getFirstFreeBank(m_treeWidget->currentItem());

        MidiBank newBank(false,
                         bank.first, bank.second,
                         qstrtostr(name));
        m_bankList.push_back(newBank);

RG_DEBUG << "BankEditorDialog::slotAddBank() : deviceItem->getDeviceId() = " << deviceItem->getDeviceId();
        QTreeWidgetItem* newBankItem =
            new MidiBankTreeWidgetItem(deviceItem->getDeviceId(),
                                     m_bankList.size() - 1,
                                     deviceItem,
                                     strtoqstr(newBank.getName()),
                                     newBank.isPercussion(),
                                     newBank.getMSB(), newBank.getLSB());
        keepBankListForNextPopulate();
        m_treeWidget->setCurrentItem(newBankItem);

        slotApply();
    }
}

void
BankEditorDialog::slotAddKeyMapping()
{
    if (!m_treeWidget->currentItem())
        return ;

    QTreeWidgetItem* currentIndex = m_treeWidget->currentItem();
//    MidiDeviceTreeWidgetItem* deviceItem = getParentDeviceItem(currentIndex);
    MidiDevice *device = getMidiDevice(currentIndex);

    if (device) {

        QString name = "";
        int n = 0;
        while (name == "" || device->getKeyMappingByName(qstrtostr(name)) != 0) {
            ++n;
            if (n == 1)
                name = tr("<new mapping>");
            else
                name = tr("<new mapping %1>").arg(n);
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
    if (!m_treeWidget->currentItem())
        return ;

    QTreeWidgetItem* currentIndex = m_treeWidget->currentItem();

    MidiBankTreeWidgetItem* bankItem = dynamic_cast<MidiBankTreeWidgetItem*>(currentIndex);

    MidiDevice *device = getMidiDevice(currentIndex);

    if (device && bankItem) {
        int currentBank = bankItem->getBank();

        int reply =
            QMessageBox::warning(
              dynamic_cast<QWidget*>(this),
              "", /* no title */
              tr("Really delete this bank?"),
              QMessageBox::Yes | QMessageBox::No,
              QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            MidiBank bank = m_bankList[currentBank];

            // Copy across all programs that aren't in the doomed bank
            //
            ProgramList newProgramList;
            for (ProgramList::iterator it = m_programList.begin();
                 it != m_programList.end();
                 ++it) {
                // If this program isn't in the bank that is being deleted,
                // add it to the new program list.  We use partialCompare()
                // because the MidiBank objects in the program list do not
                // have their name fields filled in.
                if (!it->getBank().partialCompare(bank))
                    newProgramList.push_back(*it);
            }

            // Erase the bank and repopulate
            //
            BankList::iterator er =
                m_bankList.begin();
            er += currentBank;
            m_bankList.erase(er);
            m_programList = newProgramList;
            keepBankListForNextPopulate();

            // the listview automatically selects a new current item
            m_treeWidget->blockSignals(true);
            delete currentIndex;
            m_treeWidget->blockSignals(false);

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

    MidiKeyMapTreeWidgetItem* keyItem = dynamic_cast<MidiKeyMapTreeWidgetItem*>(currentIndex);

    if (keyItem && device) {

        int reply =
            QMessageBox::warning(
              dynamic_cast<QWidget*>(this),
              "", /* no title */
              tr("Really delete this key mapping?"),
              QMessageBox::Yes | QMessageBox::No,
              QMessageBox::No);

        if (reply == QMessageBox::Yes) {

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
                    RG_DEBUG << "erasing " << keyMappingName;
                    kml.erase(i);
                    break;
                }
            }

            RG_DEBUG << " setting " << kml.size() << " key mappings to device ";

            command->setKeyMappingList(kml);
            command->setOverwrite(true);

            addCommandToHistory(command);

            RG_DEBUG << " device has " << device->getKeyMappings().size() << " key mappings now ";

            updateDialog();
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
    MidiDevice *device = getMidiDevice(deviceItem);

    QString question = tr("Really delete all banks for ") +
                       strtoqstr(device->getName()) + QString(" ?");

    int reply = QMessageBox::warning(
                  dynamic_cast<QWidget*>(this),
                  "", /* no title */
                  question,
                  QMessageBox::Yes | QMessageBox::No,
                  QMessageBox::No);

    if (reply == QMessageBox::Yes) {

        // erase all bank items
        QTreeWidgetItem* child = 0;
        while ((child = deviceItem->child(0)))
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
BankEditorDialog::getMidiDevice(QTreeWidgetItem* item)
{
    MidiDeviceTreeWidgetItem* deviceItem =
        dynamic_cast<MidiDeviceTreeWidgetItem*>(item);
    if (!deviceItem)
        return 0;

    return getMidiDevice(deviceItem->getDeviceId());
}

std::pair<int, int>
BankEditorDialog::getFirstFreeBank(QTreeWidgetItem* /* item */)
{
    //!!! percussion? this is actually only called in the expectation
    // that percussion==false at the moment

    for (int msb = 0; msb < 128; ++msb) {
        for (int lsb = 0; lsb < 128; ++lsb) {
            BankList::iterator i = m_bankList.begin();
            for (; i != m_bankList.end(); ++i) {
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
    if (bankItem) {

        // renaming a bank item

        RG_DEBUG << "BankEditorDialog::slotModifyDeviceOrBankName - "
        << "modify bank name to " << label << endl;

        if (m_bankList[bankItem->getBank()].getName() != qstrtostr(label)) {
            m_bankList[bankItem->getBank()].setName(qstrtostr(label));
            slotApply();
        }

    } else if (keyItem) {

        RG_DEBUG << "BankEditorDialog::slotModifyDeviceOrBankName - "
        << "modify key mapping name to " << label << endl;

        QString oldName = keyItem->getName();

        QTreeWidgetItem* currentIndex = m_treeWidget->currentItem();
        MidiDevice *device = getMidiDevice(currentIndex);

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
            slotApply();

            m_updateDeviceList = true;
        }

    }

}

void
BankEditorDialog::selectDeviceItem(MidiDevice *device)
{
    /**
     * sets the device-TreeWidgetItem (visibly) selected
     **/
    QTreeWidgetItem *child;
    MidiDeviceTreeWidgetItem *midiDeviceItem;
    MidiDevice *midiDevice;
    unsigned int i, cnt;
    
    cnt = m_treeWidget->topLevelItemCount();
    for( i=0; i<cnt; i++ ){
        child = m_treeWidget->topLevelItem( i );
        midiDeviceItem = dynamic_cast<MidiDeviceTreeWidgetItem*>(child);

        if (midiDeviceItem) {
            midiDevice = getMidiDevice(midiDeviceItem);

            if (midiDevice == device) {
//                 m_treeWidget->setSelected(child, true);
                m_treeWidget->setCurrentItem(child);
                return ;
            }
        }

    }

}

void
BankEditorDialog::selectDeviceBankItem(DeviceId deviceId,
                                       int bank)
{
    /**
     * sets the device-TreeWidgetItem and bank-TreeWidgetItem with deviceId (visibly) selected
    **/
//     QTreeWidgetItem *deviceChild = m_treeWidget->topLevelItem(0);
    QTreeWidgetItem *bankChild;
//     int deviceCount = 0, bankCount = 0;

    
    QTreeWidgetItem *twItemDevice = 0;
    MidiDeviceTreeWidgetItem *midiDeviceItem = 0;
    
    int bankI;
    int devI = 0;
//     while(n < m_treeWidget->childCount()){
    while( devI < m_treeWidget->topLevelItemCount() ){
//         bankChild = deviceChild->firstChild();
        
        twItemDevice = m_treeWidget->topLevelItem(devI);
        midiDeviceItem = dynamic_cast<MidiDeviceTreeWidgetItem*>(twItemDevice);

        if (midiDeviceItem){    // && bankChild) {
            
            bankI = 0;
            while( bankI < twItemDevice->childCount() ){
                bankChild = twItemDevice->child(bankI);
                
                if ((deviceId == midiDeviceItem->getDeviceId()) && (bank == bankI)) {
//                     m_treeWidget->setSelected(bankChild, true);
                    bankChild->setSelected(true);
                    return ;
                }
                bankI += 1;
//             } while ((bankChild = bankChild->nextSibling()));
            }
        }
        
//         deviceCount++;
//         bankCount = 0;
        devI += 1;
//     } while ((deviceChild = deviceChild->nextSibling()));
    }
}

void
BankEditorDialog::slotVariationToggled()
{
    slotApply();
    m_variationCombo->setEnabled(m_variationToggle->isChecked());
}

void
BankEditorDialog::slotVariationChanged(int)
{
    slotApply();
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
                      tr("All files") + " (*)", 0, 0); ///!!! Should we allow 'All files' here since LinuxSampler files don't need to have an extension?!

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
        // slotUpdate, signalled by the CommandHistory
        MidiDevice *device = getMidiDevice(deviceItem);
        if (device) {
            selectDeviceItem(device);
        }
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
    MidiBankTreeWidgetItem* bankItem
    = dynamic_cast<MidiBankTreeWidgetItem*>(m_treeWidget->currentItem());

    if (bankItem) {
        m_copyBank = std::pair<DeviceId, int>(bankItem->getDeviceId(),
                                              bankItem->getBank());
        m_pastePrograms->setEnabled(true);
    }
}

void
BankEditorDialog::slotEditPaste()
{
    MidiBankTreeWidgetItem* bankItem
    = dynamic_cast<MidiBankTreeWidgetItem*>(m_treeWidget->currentItem());

    if (bankItem) {
        // Get the full program and bank list for the source device
        //
        MidiDevice *device = getMidiDevice(m_copyBank.first);
        std::vector<MidiBank> tempBank = device->getBanks();

        ProgramList::iterator it;
        std::vector<MidiProgram> tempProg;

        // Remove programs that will be overwritten
        //
        for (it = m_programList.begin(); it != m_programList.end(); ++it) {
            if (!(it->getBank().partialCompare(m_lastBank)))
                tempProg.push_back(*it);
        }
        m_programList = tempProg;

        // Now get source list and msb/lsb
        //
        tempProg = device->getPrograms();
        MidiBank sourceBank = tempBank[m_copyBank.second];

        // Add the new programs
        //
        for (it = tempProg.begin(); it != tempProg.end(); ++it) {
            if (it->getBank().partialCompare(sourceBank)) {
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

    QString errMsg;
    if (!m_doc->exportStudio(name, errMsg, devices)) {
        if (errMsg != "") {
            QMessageBox::critical
                (0, tr("Rosegarden"), tr(QString("Could not export studio to file at %1\n(%2)")
                           .arg(name).arg(errMsg).toStdString().c_str()));
        } else {
            QMessageBox::critical
                (0, tr("Rosegarden"), tr(QString("Could not export studio to file at %1")
                           .arg(name).toStdString().c_str()));
        }
    }
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
//     CommandHistory::getInstance()->detachView(actionCollection());    //&&&
    m_doc = 0;
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
}
