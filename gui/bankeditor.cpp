
/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#include <algorithm>

#include <qobjectlist.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qlabel.h>
#include <qtabwidget.h>
#include <qgroupbox.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>

#include <klocale.h>
#include <kmessagebox.h>
#include <klistview.h>
#include <klineedit.h>

#include "bankeditor.h"
#include "widgets.h"
#include "rosestrings.h"
#include "rosegardenguidoc.h"
#include "studiocommands.h"
#include "rosedebug.h"

#include "Studio.h"
#include "MidiDevice.h"

MidiDeviceListViewItem::MidiDeviceListViewItem(int deviceNb,
                                               QListView* parent, QString name)
    : QListViewItem(parent, name),
      m_deviceNb(deviceNb)
{
}

MidiDeviceListViewItem::MidiDeviceListViewItem(int deviceNb,
                                               QListViewItem* parent, QString name,
                                               QString msb, QString lsb)
    : QListViewItem(parent, name, msb, lsb),
      m_deviceNb(deviceNb)
{
}

//--------------------------------------------------

MidiBankListViewItem::MidiBankListViewItem(int deviceNb,
                                           int bankNb,
                                           QListViewItem* parent,
                                           QString name, QString msb, QString lsb)
    : MidiDeviceListViewItem(deviceNb, parent, name, msb, lsb),
      m_bankNb(bankNb)
{
}

//--------------------------------------------------

MidiProgramsEditor::MidiProgramsEditor(BankEditorDialog* bankEditor,
                                       QWidget* parent,
                                       const char* name)
    : QVGroupBox(i18n("Bank and Program details"),
                 parent, name),
      m_bankEditor(bankEditor),
      m_mainFrame(new QFrame(this)),
      m_bankName(new QLabel(m_mainFrame)),
      m_msb(new QSpinBox(m_mainFrame)),
      m_lsb(new QSpinBox(m_mainFrame)),
      m_bankList(bankEditor->getBankList()),
      m_programList(bankEditor->getProgramList())
{

    QGridLayout *gridLayout = new QGridLayout(m_mainFrame,
                                              1,  // rows
                                              6,  // cols
                                              2); // margin
 

    gridLayout->addWidget(new QLabel(i18n("Bank Name"), m_mainFrame),
                          0, 4, AlignLeft);

    gridLayout->addWidget(m_bankName, 0, 5, AlignRight);

    gridLayout->addWidget(new QLabel(i18n("MSB Value"), m_mainFrame),
                          1, 4, AlignLeft);
    m_msb->setMinValue(0);
    m_msb->setMaxValue(127);
    gridLayout->addWidget(m_msb, 1, 5, AlignRight);

    connect(m_msb, SIGNAL(valueChanged(int)),
            this, SLOT(slotNewMSB(int)));

    gridLayout->addWidget(new QLabel(i18n("LSB Value"), m_mainFrame),
                          2, 4, AlignLeft);
    m_lsb->setMinValue(0);
    m_lsb->setMaxValue(127);
    gridLayout->addWidget(m_lsb, 2, 5, AlignRight);

    connect(m_lsb, SIGNAL(valueChanged(int)),
            this, SLOT(slotNewLSB(int)));

    QTabWidget* tab = new QTabWidget(this);

    tab->setMargin(10);

    QHBox *progHBox = new QHBox(tab);
    QVBox *progVBox;
    QHBox *numBox;
    QLabel *label;

    for (unsigned int j = 0; j < 4; j++)
    {
        progVBox = new QVBox(progHBox);

        for (unsigned int i = 0; i < 16; i++)
        {
            unsigned int labelId = j*16 + i;

            numBox = new QHBox(progVBox);
            label = new QLabel(QString("%1").arg(labelId), numBox);
            label->setFixedWidth(50);
            label->setAlignment(AlignHCenter);

            KLineEdit* lineEdit = new KLineEdit(numBox, label->text().data());
            lineEdit->setCompletionMode(KGlobalSettings::CompletionAuto);
            m_programNames.push_back(lineEdit);

            connect(m_programNames[labelId],
                    SIGNAL(textChanged(const QString&)),
                    this,
                    SLOT(slotProgramChanged(const QString&)));
        }
    }
    tab->addTab(progHBox, i18n("Programs 0 - 63"));

    progHBox = new QHBox(tab);
    for (unsigned int j = 0; j < 4; j++)
    {
        progVBox = new QVBox(progHBox);

        for (unsigned int i = 0; i < 16; i++)
        {
            unsigned int labelId = 64 + j*16 + i;

            numBox = new QHBox(progVBox);
            label = new QLabel(QString("%1").arg(labelId), numBox);
            label->setFixedWidth(50);
            label->setAlignment(AlignHCenter);

            KLineEdit* lineEdit = new KLineEdit(numBox, label->text().data());
            lineEdit->setCompletionMode(KGlobalSettings::CompletionAuto);
            m_programNames.push_back(lineEdit);

            connect(m_programNames[labelId],
                    SIGNAL(textChanged(const QString&)),
                    this,
                    SLOT(slotProgramChanged(const QString&)));

        }
    }
    tab->addTab(progHBox, i18n("Programs 64 - 127"));

}

MidiProgramsEditor::MidiProgramContainer
MidiProgramsEditor::getBankSubset(Rosegarden::MidiByte msb,
                                  Rosegarden::MidiByte lsb)
{
    MidiProgramContainer program;
    MidiProgramContainer::iterator it;

    for (it = m_programList.begin(); it != m_programList.end(); it++)
    {
        if (it->msb == msb && it->lsb == lsb)
            program.push_back(*it);
    }

    return program;
}

Rosegarden::MidiBank*
MidiProgramsEditor::getCurrentBank()
{
    return m_currentBank;
}

void
MidiProgramsEditor::modifyCurrentPrograms(int oldMSB, int oldLSB,
                                        int msb, int lsb)
{
    MidiProgramContainer::iterator it;

    for (it = m_programList.begin(); it != m_programList.end(); it++)
    {
        if (it->msb == oldMSB && it->lsb == oldLSB)
        {
            it->msb = msb;
            it->lsb = lsb;
        }
    }
}

void
MidiProgramsEditor::slotPopulateBank(QListViewItem* item)
{
    RG_DEBUG << "MidiProgramsEditor::slotPopulateBank" << endl;

    MidiBankListViewItem* bankItem = dynamic_cast<MidiBankListViewItem*>(item);
    if (!bankItem) return;

    int deviceNb = bankItem->getDevice();
    Rosegarden::MidiDevice* device = m_bankEditor->getMidiDevice(deviceNb);
    if (!device) return;

    setBankName(item->text(0));

    RG_DEBUG << "MidiProgramsEditor::slotPopulateBank : bankItem->getBank = "
             << bankItem->getBank() << endl;

    m_currentBank = &(m_bankList[bankItem->getBank()]); // device->getBankByIndex(bankItem->getBank());

    blockAllSignals(true);

    // set the bank values
    m_msb->setValue(m_currentBank->msb);
    m_lsb->setValue(m_currentBank->lsb);
    MidiProgramContainer programSubset = getBankSubset(m_currentBank->msb,
                                                       m_currentBank->lsb);
    MidiProgramContainer::iterator it;

    for (unsigned int i = 0; i < m_programNames.size(); i++) {
        m_programNames[i]->clear();

        for (it = programSubset.begin(); it != programSubset.end(); it++) {
            if (it->program == i) {
                m_programNames[i]->setText(strtoqstr(it->name));
                break;
            }
        }

        // show start of label
        m_programNames[i]->setCursorPosition(0);
    }

    blockAllSignals(false);
}

void
MidiProgramsEditor::slotNewMSB(int value)
{
    m_msb->blockSignals(true);

    int msb;

    try
        {
            msb = ensureUniqueMSB(value, value > getCurrentBank()->msb);
        }
    catch(bool)
        {
            msb = getCurrentBank()->msb;
        }

    modifyCurrentPrograms(getCurrentBank()->msb,
                          getCurrentBank()->lsb,
                          msb,
                          getCurrentBank()->lsb);

    m_msb->setValue(msb);
    getCurrentBank()->msb = msb;

    m_msb->blockSignals(false);

    m_bankEditor->setModified(true);
}

void
MidiProgramsEditor::slotNewLSB(int value)
{
    m_lsb->blockSignals(true);

    int lsb;

    try
        {
            lsb = ensureUniqueLSB(value, value > getCurrentBank()->lsb);
        }
    catch(bool)
        {
            lsb = getCurrentBank()->lsb;
        }

    modifyCurrentPrograms(getCurrentBank()->msb,
                          getCurrentBank()->lsb,
                          getCurrentBank()->msb,
                          lsb);

    m_lsb->setValue(lsb);
    getCurrentBank()->lsb = lsb;

    m_lsb->blockSignals(false);

    m_bankEditor->setModified(true);
}

void
MidiProgramsEditor::slotProgramChanged(const QString &programName)
{
    QString senderName = sender()->name();

    unsigned int id = senderName.toUInt();

    RG_DEBUG << "BankEditorDialog::slotProgramChanged("
             << programName << ") : id = " << id << endl;

    Rosegarden::MidiProgram *program =
        getProgram(getCurrentBank()->msb,
                   getCurrentBank()->lsb,
                   id);

    if (program == 0)
    {
        program = new Rosegarden::MidiProgram;
        program->msb = getCurrentBank()->msb;
        program->lsb = getCurrentBank()->lsb;
        program->program = id;
        m_programList.push_back(*program);

        // Now, get with the program
        //
        program =
            getProgram(getCurrentBank()->msb,
                       getCurrentBank()->lsb,
                       id);
    }

    if (qstrtostr(programName) != program->name)
    {
        program->name = qstrtostr(programName);
        m_bankEditor->setModified(true);
    }
}

int
MidiProgramsEditor::ensureUniqueMSB(int msb, bool ascending)
{
    int newMSB = msb;
    while (banklistContains(newMSB, m_lsb->value())
           && newMSB < 128
           && newMSB > -1)
        if (ascending) newMSB++;
        else newMSB--;

   if (newMSB == -1 || newMSB == 128)
       throw false;

    return newMSB;
}

int
MidiProgramsEditor::ensureUniqueLSB(int lsb, bool ascending)
{
    int newLSB = lsb;
    while (banklistContains(m_msb->value(), newLSB)
           && newLSB < 128
           && newLSB > -1)
        if (ascending) newLSB++;
        else newLSB--;

   if (newLSB == -1 || newLSB == 128)
       throw false;

    return newLSB;
}

bool
MidiProgramsEditor::banklistContains(int msb, int lsb)
{
    MidiBankContainer::iterator it;

    for (it = m_bankList.begin(); it != m_bankList.end(); it++)
        if (it->msb == msb && it->lsb == lsb)
            return true;

    return false;
}

Rosegarden::MidiProgram*
MidiProgramsEditor::getProgram(int msb, int lsb, int program)
{
    MidiProgramsEditor::MidiProgramContainer::iterator it = m_programList.begin();

    for (; it != m_programList.end(); it++)
    {
        if (it->msb == msb && it->lsb == lsb && it->program == program)
            return &(*it);
    }

    return 0;

}

void
MidiProgramsEditor::setBankName(const QString& s)
{
    m_bankName->setText(s);
}

//--------------------------------------------------

BankEditorDialog::BankEditorDialog(QWidget *parent,
                                   RosegardenGUIDoc *doc):
    KDialogBase(parent, "bankeditordialog", true,
                i18n("Manage Banks and Programs..."),
                Ok | Apply | Cancel,
                Ok, true),
    m_studio(&doc->getStudio()),
    m_doc(doc),
    m_modified(false),
    m_replaceBankList(true),
    m_lastDevice(0),
    m_lastMSB(0),
    m_lastLSB(0)
{
    QHBox* mainFrame = makeHBoxMainWidget();

    QSplitter* splitter = new QSplitter(mainFrame);

    //
    // Left-side list view
    //
    QVBox* leftPart = new QVBox(splitter);
    m_listView = new KListView(leftPart);
    m_listView->addColumn(i18n("Midi Device"));
    m_listView->addColumn(i18n("MSB"));
    m_listView->addColumn(i18n("LSB"));
    m_listView->setRootIsDecorated(true);
    m_listView->setShowSortIndicator(true);
    m_listView->setItemsRenameable(true);

    // Fill list view
    //
    Rosegarden::DeviceList *devices = m_studio->getDevices();
    Rosegarden::DeviceListIterator it;
    int deviceIdx = 0;
    for (it = devices->begin(); it != devices->end(); ++it, ++deviceIdx)
    {
        if ((*it)->getType() == Rosegarden::Device::Midi)
        {
            Rosegarden::MidiDevice* midiDevice = dynamic_cast<Rosegarden::MidiDevice*>(*it);
            
            m_deviceList.push_back(midiDevice->getName());
            QString itemName = strtoqstr(midiDevice->getName());

            RG_DEBUG << "BankEditorDialog : adding "
                     << itemName << endl;

            QListViewItem* deviceItem = new MidiDeviceListViewItem(deviceIdx,
                                                                   m_listView, itemName);
            deviceItem->setOpen(true);

            MidiProgramsEditor::MidiBankContainer banks = midiDevice->getBanks();
            // add banks for this device
            for (unsigned int i = 0; i < banks.size(); ++i) {
                RG_DEBUG << "BankEditorDialog : adding "
                         << itemName << " - " << strtoqstr(banks[i].name)
                         << endl;
                new MidiBankListViewItem(deviceIdx, i, deviceItem,
                                         strtoqstr(banks[i].name),
                                         QString("%1").arg(banks[i].msb),
                                         QString("%1").arg(banks[i].lsb));
            }
            
            
        }
    }

    QGroupBox *bankBox  = new QGroupBox(3,
                                        Qt::Horizontal,
                                        i18n("Manage Banks..."),
                                        leftPart);

    m_addBank        = new QPushButton(i18n("Add Bank"),         bankBox);
    m_deleteBank     = new QPushButton(i18n("Delete Bank"),      bankBox);
    m_deleteAllBanks = new QPushButton(i18n("Delete All Banks"), bankBox);

    connect(m_listView, SIGNAL(currentChanged(QListViewItem*)),
            this,       SLOT(slotPopulateDevice(QListViewItem*)));

    //
    // Right-side Midi Program Editor
    //


    /*
    deviceBox->addSpace(0);
    deviceBox->addSpace(0);
    deviceBox->addSpace(0);
    m_metronomeCheckBox =
        new QCheckBox(i18n("use as Metronome device"), deviceBox);
    deviceBox->addSpace(0);

    connect(m_metronomeCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(slotMetronomeChecked(int)));

    deviceBox->addSpace(0);
    deviceBox->addSpace(0);
    deviceBox->addSpace(0);
    m_instrumentLabel = new QLabel(i18n("Metronome instrument"), deviceBox);
    m_metronomeInstrument = new RosegardenComboBox(true, deviceBox);

    deviceBox->addSpace(0);
    deviceBox->addSpace(0);
    deviceBox->addSpace(0);
    m_pitchLabel = new QLabel(i18n("Metronome voice"), deviceBox);
    m_metronomePitch = new QSpinBox(deviceBox);

    m_instrumentLabel->setDisabled(true);
    m_metronomeInstrument->setDisabled(true);
    m_pitchLabel->setDisabled(true);
    m_metronomePitch->setDisabled(true);
    */


    m_programEditor = new MidiProgramsEditor(this, splitter);


    slotPopulateDevice(m_listView->firstChild());

    // default buttons states
    enableButtonOK(false);
    enableButtonApply(false);

    // device/bank modification
    connect(m_listView, SIGNAL(itemRenamed             (QListViewItem*,const QString&,int)),
            this,       SLOT(slotModifyDeviceOrBankName(QListViewItem*,const QString&,int)));

    connect(m_addBank, SIGNAL(clicked()),
            this, SLOT(slotAddBank()));

    connect(m_deleteBank, SIGNAL(clicked()),
            this, SLOT(slotDeleteBank()));

    connect(m_deleteAllBanks, SIGNAL(clicked()),
            this, SLOT(slotDeleteAllBanks()));

    // Check for no Midi devices and disable everything
    //
    if (m_studio->getMidiDevice(0) == 0)
    {
        leftPart->setDisabled(true);
        m_programEditor->setDisabled(true);
    }

};

Rosegarden::MidiDevice*
BankEditorDialog::getCurrentMidiDevice()
{
    return getMidiDevice(m_listView->currentItem());
}

void
BankEditorDialog::slotPopulateDevice(QListViewItem* item)
{
    RG_DEBUG << "BankEditorDialog::slotPopulateDevice\n";

    if (!item) return;

    if (m_modified)
    {
        // then ask if we want to apply the changes
        QString text = QString("\"%1\"").
                       arg(strtoqstr(m_deviceList[m_lastDevice])) +
                       i18n(" - there are some pending changes for this device.\nApply these changes?");

        int reply = KMessageBox::questionYesNo(this, text);

        if (reply == KMessageBox::Yes)
        {
            ModifyDeviceCommand *command =
                new ModifyDeviceCommand(m_studio,
                                        m_lastDevice,
                                        m_deviceList[m_lastDevice],
                                        m_bankList,
                                        m_programList);

            addCommandToHistory(command);
        }
        setModified(false);
    }

    MidiBankListViewItem* bankItem = dynamic_cast<MidiBankListViewItem*>(item);
    
    if (!bankItem) {
        RG_DEBUG << "BankEditorDialog::slotPopulateDevice : not a bank item - trying first child\n";
        QListViewItem* child = item->firstChild();
        if (child) { // try it
            m_listView->setCurrentItem(child);
        } else {
            RG_DEBUG << "BankEditorDialog::slotPopulateDevice : not a bank item - no child\n";
        }

        return;
    }
    

    Rosegarden::MidiDevice *device = getMidiDevice(bankItem->getDevice());

    if (m_replaceBankList)
        m_bankList    = device->getBanks();
    else
        m_replaceBankList = true;

    m_programList = device->getPrograms();

    m_lastMSB = m_bankList[bankItem->getBank()].msb;
    m_lastLSB = m_bankList[bankItem->getBank()].lsb;

    m_programEditor->slotPopulateBank(item);

    m_lastDevice = bankItem->getDevice();

}

void
BankEditorDialog::slotOk()
{
    if (m_modified)
    {
        RG_DEBUG << "BankEditorDialog::slotOk() bankList size = "
                 << m_bankList.size() << endl;

        ModifyDeviceCommand *command =
            new ModifyDeviceCommand(m_studio,
                                    m_lastDevice,
                                    m_deviceList[m_lastDevice],
                                    m_bankList,
                                    m_programList);

        addCommandToHistory(command);
        accept();
    }
}

void
BankEditorDialog::slotApply()
{
    if (m_modified)
    {
        ModifyDeviceCommand *command =
            new ModifyDeviceCommand(m_studio,
                                    m_lastDevice,
                                    m_deviceList[m_lastDevice],
                                    m_bankList,
                                    m_programList);

        addCommandToHistory(command);
    }
}

MidiDeviceListViewItem*
BankEditorDialog::getParentDeviceItem(QListViewItem* item)
{
    if (!item) return 0;

    if (dynamic_cast<MidiBankListViewItem*>(item)) // this is a bank item,
        // go up to the parent device item
        item = item->parent();

    if (!item) {
        RG_DEBUG << "BankEditorDialog::getParentDeviceItem : missing parent device item for bank item - this SHOULD NOT HAPPEN\n";
        return 0;
    }

    return dynamic_cast<MidiDeviceListViewItem*>(item);
}


void
BankEditorDialog::slotAddBank()
{
    if (!m_listView->currentItem()) return;

    QListViewItem* currentItem = m_listView->currentItem();

    MidiDeviceListViewItem* deviceItem = getParentDeviceItem(currentItem);

    Rosegarden::MidiDevice *device = getMidiDevice(currentItem);
   
    if (device)
    {
        std::pair<int, int> bank = getFirstFreeBank(m_listView->currentItem());

        Rosegarden::MidiBank newBank;
        newBank.msb = bank.first;
        newBank.lsb = bank.second;
        newBank.name = "<new bank>";
        m_bankList.push_back(newBank);

        QListViewItem* newBankItem = new MidiBankListViewItem(deviceItem->getDevice(),
                                                              m_bankList.size() - 1,
                                                              deviceItem,
                                                              strtoqstr(newBank.name),
                                                              QString("%1").arg(newBank.msb),
                                                              QString("%1").arg(newBank.lsb));
        m_replaceBankList = false;
        m_listView->setCurrentItem(newBankItem);

        setModified(true);
    }
}

void
BankEditorDialog::slotDeleteBank()
{
//     blockAllSignals(true);

//     Rosegarden::MidiDevice *device =
//         getMidiDevice(m_listView->currentItem());

//     if (device)
//     {
//         int newBank = getCurrentBank() - 1;
//         if (newBank < 0) newBank = 0;

//         int msb = m_bankList[getCurrentBank()].msb;
//         int lsb = m_bankList[getCurrentBank()].lsb;

//         // Copy across all programs that aren't in the doomed bank
//         //
//         MidiProgramsEditor::MidiProgramContainer::iterator it;
//         MidiProgramsEditor::MidiProgramContainer tempList;
//         for (it = m_programList.begin(); it != m_programList.end(); it++)
//             if (it->msb != msb || it->lsb != lsb)
//                 tempList.push_back(*it);

//         // Erase the bank and repopulate
//         //
//         MidiProgramsEditor::MidiBankContainer::iterator er = m_bankList.begin();
//         er += m_programEditor->getCurrentBank();
//         m_bankList.erase(er);
//         m_programList = tempList;

//         slotPopulateDeviceBank(m_listView->currentItem(), newBank);

//         setModified(true);
//     }

//     blockAllSignals(false);
}

void
BankEditorDialog::slotDeleteAllBanks()
{
//     blockAllSignals(true);

//     Rosegarden::MidiDevice *device =
//         getMidiDevice(m_listView->currentItem());

//     if (device)
//     {
//         device->clearProgramList();
//         device->clearBankList();
//         slotPopulateDevice(m_listView->currentItem());
//     }

//     setModified(true);

//     blockAllSignals(false);
}

Rosegarden::MidiDevice*
BankEditorDialog::getMidiDevice(int deviceNb)
{
    Rosegarden::DeviceList *devices = m_studio->getDevices();

    if (!devices) return 0;
    if (unsigned(deviceNb) > devices->size()) return 0;

    return dynamic_cast<Rosegarden::MidiDevice*>((*devices)[deviceNb]);
}

Rosegarden::MidiDevice*
BankEditorDialog::getMidiDevice(QListViewItem* item)
{
    MidiDeviceListViewItem* deviceItem = dynamic_cast<MidiDeviceListViewItem*>(item);
    if (!deviceItem) return 0;

    Rosegarden::DeviceList *devices = m_studio->getDevices();

    if (!item || !devices) return 0;
    if (unsigned(deviceItem->getDevice()) > devices->size()) return 0;

    return dynamic_cast<Rosegarden::MidiDevice*>((*devices)[deviceItem->getDevice()]);
}

// Try to find a unique MSB/LSB pair for a new bank
//
std::pair<int, int>
BankEditorDialog::getFirstFreeBank(QListViewItem* item)
{
    int msb = 0;
    int lsb = 0;

    Rosegarden::MidiDevice *device = getMidiDevice(item);

    if (device)
    {
        do
        {
            lsb = 0;
            while(m_programEditor->banklistContains(msb, lsb) && lsb < 128)
                lsb++;
        }
        while (lsb == 128 && msb++);
    }

    return std::pair<int, int>(msb, lsb);
}

void
BankEditorDialog::slotModifyDeviceOrBankName(QListViewItem* item, const QString &label, int)
{
    RG_DEBUG << "MidiProgramsEditor::slotModifyDeviceorBankName\n";

    MidiDeviceListViewItem* deviceItem = dynamic_cast<MidiDeviceListViewItem*>(item);
    MidiBankListViewItem* bankItem     = dynamic_cast<MidiBankListViewItem*>(item);
    
    if (bankItem) {

        // renaming a bank item

        RG_DEBUG << "MidiProgramsEditor::slotModifyDeviceorBankName : modify bank name to "
                 << label << endl;

        if (m_bankList[bankItem->getBank()].name != qstrtostr(label)) {
            m_bankList[bankItem->getBank()].name = qstrtostr(label);
            setModified(true);
        }
        
    } else if (deviceItem) {

        // renaming a device item

        RG_DEBUG << "MidiProgramsEditor::slotModifyDeviceorBankName : modify device name to "
                 << label << endl;

        if (m_deviceList[deviceItem->getDevice()] != qstrtostr(label)) {
            m_deviceList[deviceItem->getDevice()] = qstrtostr(label);
            setModified(true);
        }
    }
    
}

void
BankEditorDialog::setModified(bool value)
{
    RG_DEBUG << "BankEditorDialog::setModified("
             << value << ")\n";

    if (m_modified == value) return;

    if (value == true)
    {
        enableButtonOK(true);
        enableButtonApply(true);
    }
    else
    {
        enableButtonOK(false);
        enableButtonApply(false);
    }

    m_modified = value;
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

void MidiProgramsEditor::blockAllSignals(bool block)
{
    RG_DEBUG << "MidiProgramsEditor : Blocking all signals = " << block << endl;

    const QObjectList* allChildren = queryList("KLineEdit", "[0-9]+");
    QObjectListIt it(*allChildren);
    QObject *obj;

    while ( (obj = it.current()) != 0 ) {
//         RG_DEBUG << "Blocking signals for " << obj->name()
//                  << ", class " << obj->className() << endl;
        obj->blockSignals(block);
        ++it;
    }

    m_msb->blockSignals(block);
    m_lsb->blockSignals(block);
}

// ------------------------ RemapInstrumentDialog -----------------------
//
RemapInstrumentDialog::RemapInstrumentDialog(QWidget *parent,
                                             RosegardenGUIDoc *doc):
    KDialogBase(parent, "", true, i18n("Remap Instrument assigments..."),
                Ok | Apply | Cancel),
    m_doc(doc)
{
    QVBox *vBox = makeVBoxMainWidget();

    m_buttonGroup = new QButtonGroup(1, Qt::Horizontal,
                                     i18n("Device or Instrument"),
                                     vBox);

    new QLabel(i18n("Remap Tracks by all Instruments on a Device or by single Instrument"), m_buttonGroup);
    m_deviceButton = new QRadioButton(i18n("Device"), m_buttonGroup);
    m_instrumentButton = new QRadioButton(i18n("Instrument"), m_buttonGroup);


    connect(m_buttonGroup, SIGNAL(released(int)),
            this, SLOT(slotRemapReleased(int)));

    QGroupBox *groupBox = new QGroupBox(2, Qt::Horizontal,
                                        i18n("Choose Source and Destination"),
                                        vBox);

    new QLabel(i18n("From"), groupBox);
    new QLabel(i18n("To"), groupBox);
    m_fromCombo = new RosegardenComboBox(false, groupBox);
    m_toCombo = new RosegardenComboBox(false, groupBox);

    /*
    QGridLayout *gridLayout = new QGridLayout(frame,
                                              2,  // rows
                                              2,  // cols
                                              2); // margin
    m_fromCombo = new RosegardenComboBox(true, frame);
    m_toCombo = new RosegardenComboBox(true, frame);

    gridLayout->addWidget(new QLabel(i18n("From:"), frame), 0, 0, AlignLeft);
    gridLayout->addWidget(new QLabel(i18n("To:"), frame), 0, 1, AlignLeft);
    gridLayout->addWidget(m_fromCombo, 1, 0, AlignLeft);
    gridLayout->addWidget(m_toCombo, 1, 1, AlignLeft);
    */

    m_buttonGroup->setButton(0);
    populateCombo(0);
}

void
RemapInstrumentDialog::populateCombo(int id)
{
    m_fromCombo->clear();
    m_toCombo->clear();
    Rosegarden::Studio *studio = &m_doc->getStudio();

    if (id == 0)
    {
        Rosegarden::DeviceList *devices = studio->getDevices();
        Rosegarden::DeviceListIterator it;

        for (it = devices->begin(); it != devices->end(); it++)
        {
            m_fromCombo->insertItem(strtoqstr((*it)->getName()));
            m_toCombo->insertItem(strtoqstr((*it)->getName()));
        }

        if (devices->size() == 0)
        {
            m_fromCombo->insertItem(i18n("<no devices>"));
            m_toCombo->insertItem(i18n("<no devices>"));
        }
    }
    else 
    {
        //Rosegarden::DeviceList *devices = studio->getDevices();
        Rosegarden::InstrumentList list = studio->getPresentationInstruments();
        Rosegarden::InstrumentList::iterator it = list.begin();

        for (; it != list.end(); it++)
        {
            m_fromCombo->insertItem(strtoqstr((*it)->getName()));
            m_toCombo->insertItem(strtoqstr((*it)->getName()));
        }
    }

    /*
    for (int i = 0; m_fromCombo->count(); i++)
    {
    }
    */

}


void
RemapInstrumentDialog::slotRemapReleased(int id)
{
    populateCombo(id);
}

void
RemapInstrumentDialog::slotOk()
{
    slotApply();
    accept();
}

void
RemapInstrumentDialog::slotApply()
{
    if (m_buttonGroup->id(m_buttonGroup->selected()) == 0) // devices
    {
        ModifyDeviceMappingCommand *command =
            new ModifyDeviceMappingCommand(m_doc,
                                           m_fromCombo->currentItem(),
                                           m_toCombo->currentItem());
        addCommandToHistory(command);
    }
    else // instruments
    {
        ModifyInstrumentMappingCommand *command =
            new ModifyInstrumentMappingCommand(m_doc,
                                               m_fromCombo->currentItem(),
                                               m_toCombo->currentItem());

        addCommandToHistory(command);
    }

    emit applyClicked();
}

void
RemapInstrumentDialog::addCommandToHistory(KCommand *command)
{
    getCommandHistory()->addCommand(command);
}


MultiViewCommandHistory*
RemapInstrumentDialog::getCommandHistory()
{
    return m_doc->getCommandHistory();
}


