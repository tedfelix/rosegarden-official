
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

#include <klocale.h>
#include <kmessagebox.h>

#include <qhbox.h>
#include <qvbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qtabwidget.h>
#include <qgroupbox.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>

#include "bankeditor.h"
#include "widgets.h"
#include "rosestrings.h"
#include "rosegardenguidoc.h"
#include "studiocommands.h"
#include "rosedebug.h"

#include "Studio.h"
#include "MidiDevice.h"

// use this flag to cheat when setting up new devices
//
bool _newBank = false;

BankEditorDialog::BankEditorDialog(QWidget *parent,
                                   RosegardenGUIDoc *doc):
    KDialogBase(parent, "", true, i18n("Manage Banks and Programs..."),
                Ok | Apply | Cancel),
    m_studio(&doc->getStudio()),
    m_doc(doc),
    m_modified(false),
    m_lastDevice(0),
    m_lastMSB(0),
    m_lastLSB(0)
{


    QVBox *vBox = makeVBoxMainWidget();

    QGroupBox *deviceBox  = new QGroupBox(5,
                                          Qt::Horizontal,
                                          i18n("Select MIDI Device..."),
                                          vBox);

    deviceBox->addSpace(0);

    m_deviceCombo = new RosegardenComboBox(false, deviceBox);
    m_deviceCombo->setEditable(true);
    m_deviceCombo->setInsertionPolicy(QComboBox::NoInsertion);

    deviceBox->addSpace(0);
    deviceBox->addSpace(0);
    deviceBox->addSpace(0);

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

    Rosegarden::DeviceList *devices = m_studio->getDevices();
    Rosegarden::DeviceListIterator it;
    for (it = devices->begin(); it != devices->end(); it++)
    {
        if ((*it)->getType() == Rosegarden::Device::Midi)
        {
            m_deviceList.push_back((*it)->getName());
            m_deviceCombo->insertItem(strtoqstr((*it)->getName()));
        }
    }

    QGroupBox *bankBox  = new QGroupBox(3,
                                        Qt::Horizontal,
                                        i18n("Manage Banks..."),
                                        vBox);


    m_addBank = new QPushButton(i18n("Add Bank"), bankBox);
    m_deleteBank = new QPushButton(i18n("Delete Bank"), bankBox);
    m_deleteAllBanks = new QPushButton(i18n("Delete All Banks"), bankBox);

    QGroupBox *groupBox = new QGroupBox(1,
                                        Qt::Horizontal,
                                        i18n("Bank and Program details"),
                                        vBox);

    m_mainFrame = new QFrame(groupBox);
    QGridLayout *gridLayout = new QGridLayout(m_mainFrame,
                                              1,  // rows
                                              6,  // cols
                                              2); // margin
 

    gridLayout->addWidget(new QLabel(i18n("Bank Name"), m_mainFrame),
                          0, 4, AlignRight);
    m_bankCombo = new RosegardenComboBox(false, m_mainFrame);
    m_bankCombo->setEditable(true);
    gridLayout->addWidget(m_bankCombo, 0, 5, AlignRight);

    gridLayout->addWidget(new QLabel(i18n("MSB Value"), m_mainFrame),
                          1, 4, AlignRight);
    m_msb = new QSpinBox(m_mainFrame);
    m_msb->setMinValue(0);
    m_msb->setMaxValue(127);
    gridLayout->addWidget(m_msb, 1, 5, AlignRight);

    connect(m_msb, SIGNAL(valueChanged(int)),
            this, SLOT(slotNewMSB(int)));

    gridLayout->addWidget(new QLabel(i18n("LSB Value"), m_mainFrame),
                          2, 4, AlignRight);
    m_lsb = new QSpinBox(m_mainFrame);
    m_lsb->setMinValue(0);
    m_lsb->setMaxValue(127);
    gridLayout->addWidget(m_lsb, 2, 5, AlignRight);

    connect(m_lsb, SIGNAL(valueChanged(int)),
            this, SLOT(slotNewLSB(int)));

    /*
    gridLayout->addWidget(new QLabel(i18n("Manage Banks:"), frame),
                          0, 4, AlignCenter);
    gridLayout->addWidget(m_addBank, 0, 5, AlignLeft);
    gridLayout->addWidget(m_deleteBank, 1, 5, AlignLeft);
    gridLayout->addWidget(m_deleteAllBanks, 2, 5, AlignLeft);
    */

    // spacer
    //gridLayout->addRowSpacing(3, 15);

    m_programTab = new QTabWidget(m_mainFrame);
    m_programTab->setMargin(10);
    gridLayout->addMultiCellWidget(m_programTab, 4, 4, 0, 6);

    QHBox *progHBox = new QHBox(m_mainFrame);
    QVBox *progVBox;
    QHBox *numBox;
    QLabel *label;

    for (unsigned int j = 0; j < 4; j++)
    {
        progVBox = new QVBox(progHBox);

        for (unsigned int i = 0; i < 16; i++)
        {
            numBox = new QHBox(progVBox);
            label = new QLabel(QString("%1").arg(j*16 + i), numBox);
            label->setFixedWidth(50);
            label->setAlignment(AlignHCenter);

            m_programNames.push_back(new ProgramLine(numBox, j*16 + i));

            connect(m_programNames[j*16 + i],
                    SIGNAL(newText(const QString&, int)),
                    this,
                    SLOT(slotProgramChanged(const QString&, int)));
        }
    }
    m_programTab->addTab(progHBox, i18n("Programs 0 - 63"));

    progHBox = new QHBox(groupBox);
    for (unsigned int j = 0; j < 4; j++)
    {
        progVBox = new QVBox(progHBox);

        for (unsigned int i = 0; i < 16; i++)
        {
            numBox = new QHBox(progVBox);
            label = new QLabel(QString("%1").arg(64 + j*16 + i), numBox);
            label->setFixedWidth(50);
            label->setAlignment(AlignHCenter);

            m_programNames.push_back(new ProgramLine(numBox, 64 + j*16 + i));

            connect(m_programNames[64 + j*16 + i],
                    SIGNAL(newText(const QString&, int)),
                    this,
                    SLOT(slotProgramChanged(const QString&, int)));

        }
    }
    m_programTab->addTab(progHBox, i18n("Programs 64 - 127"));

    connect(m_deviceCombo, SIGNAL(activated(int)),
            this, SLOT(slotPopulateDevice(int)));

    connect(m_deviceCombo, SIGNAL(propagate(int)),
            this, SLOT(slotPopulateDevice(int)));

    connect(m_bankCombo, SIGNAL(activated(int)),
            this, SLOT(slotPopulateBank(int)));

    connect(m_bankCombo, SIGNAL(propagate(int)),
            this, SLOT(slotPopulateBank(int)));

    slotPopulateDevice(0);

    // default buttons states
    enableButtonOK(false);
    enableButtonApply(false);

    // modification
    connect(m_deviceCombo, SIGNAL(textChanged(const QString&)),
            this, SLOT(slotModifyDeviceName(const QString&)));

    connect(m_bankCombo, SIGNAL(textChanged(const QString&)),
            this, SLOT(slotModifyBankName(const QString&)));

    connect(m_addBank, SIGNAL(released()),
            this, SLOT(slotAddBank()));

    connect(m_deleteBank, SIGNAL(released()),
            this, SLOT(slotDeleteBank()));

    connect(m_deleteAllBanks, SIGNAL(released()),
            this, SLOT(slotDeleteAllBanks()));
};

void
BankEditorDialog::slotPopulateDeviceBank(int deviceNo, int bank)
{
    _newBank = true;

    Rosegarden::MidiDevice *device = getMidiDevice(deviceNo);

    if (device)
    {
        m_bankCombo->clear();

        if (m_bankList.size() > 0)
        {
            m_mainFrame->setDisabled(false);
            m_deleteBank->setDisabled(false);
            m_deleteAllBanks->setDisabled(false);

            for (unsigned int i = 0; i < m_bankList.size(); i++)
            {
                m_bankCombo->insertItem(strtoqstr(m_bankList[i].name));
            }

            m_msb->setValue(m_bankList[bank].msb);
            m_lsb->setValue(m_bankList[bank].lsb);

            m_bankCombo->setCurrentItem(bank);

            /*
            if (device->getMetronome())
            {
                m_instrumentLabel->setDisabled(false);
                m_metronomeInstrument->setDisabled(false);
                m_pitchLabel->setDisabled(false);
                m_metronomePitch->setDisabled(false);
            }
            */

            slotPopulateBank(bank);
        }
        else
        {
            // no banks
            m_mainFrame->setDisabled(true);
            m_deleteBank->setDisabled(true);
            m_deleteAllBanks->setDisabled(true);
        }

        m_lastMSB = m_bankList[bank].msb;
        m_lastLSB = m_bankList[bank].lsb;
    }

    _newBank = false;

    // remember last device
    m_lastDevice = deviceNo;
}

void
BankEditorDialog::slotPopulateDevice(int devNo)
{
    _newBank = true;

    if (m_modified)
    {
        // then ask if we want to apply the changes
        QString text = QString("\"%1\"").
                       arg(strtoqstr(m_deviceList[m_lastDevice])) +
                       i18n(" - there are some pending changes for this device.\n") +
                       i18n("Apply these changes?");

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

    // Populate from actual MidiDevice
    //
    Rosegarden::MidiDevice *device = getMidiDevice(devNo);

    m_bankList.clear();
    m_programList.clear();

    if (device)
    {
        Rosegarden::StringList banks = device->getBankList();

        m_bankList.clear();
        m_programList.clear();
        m_bankList = device->getBanks();
        m_programList = device->getPrograms();
    }

    slotPopulateDeviceBank(devNo, 0);

    _newBank = false;
}




void
BankEditorDialog::slotPopulateBank(int bank)
{
    //RG_DEBUG << "SLOT POPULATE BANK" << endl;
    _newBank = true;

    Rosegarden::MidiDevice *device =
        getMidiDevice(m_deviceCombo->currentItem());

    if (device)
    {
        // set the bank values
        m_msb->setValue(m_bankList[bank].msb);
        m_lsb->setValue(m_bankList[bank].lsb);
        MidiProgramContainer programSubset 
            = getBankSubset(m_bankList[bank].msb,
                            m_bankList[bank].lsb);
        MidiProgramContainer::iterator it;

        for (unsigned int i = 0; i < 128; i++)
        {
            m_programNames[i]->setText("");

            for (it = programSubset.begin(); it != programSubset.end(); it++)
            {
                if (it->program == i)
                {
                    m_programNames[i]->setText(strtoqstr(it->name));
                    break;
                }
            }

            // show start of label
            m_programNames[i]->setCursorPosition(0);
        }

    }

    _newBank = false;

}

BankEditorDialog::MidiProgramContainer
BankEditorDialog::getBankSubset(Rosegarden::MidiByte msb,
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


void
BankEditorDialog::slotOk()
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

void
BankEditorDialog::slotAddBank()
{
    _newBank = true;

    Rosegarden::MidiDevice *device =
        getMidiDevice(m_deviceCombo->currentItem());

    if (device)
    {
        std::pair<int, int> bank =
            getFirstFreeBank(m_deviceCombo->currentItem());

        Rosegarden::MidiBank *newBank = new Rosegarden::MidiBank();
        newBank->msb = bank.first;
        newBank->lsb = bank.second;
        newBank->name = std::string("<new bank>");
        setModified(true);

        m_bankList.push_back(*newBank);

        slotPopulateDeviceBank(m_deviceCombo->currentItem(),
                               m_bankCombo->count());
    }

    _newBank = false;

}

void
BankEditorDialog::slotDeleteBank()
{
    _newBank = true;

    Rosegarden::MidiDevice *device =
        getMidiDevice(m_deviceCombo->currentItem());

    if (device)
    {
        int newBank = m_bankCombo->currentItem() - 1;
        if (newBank < 0) newBank = 0;

        int msb = m_bankList[m_bankCombo->currentItem()].msb;
        int lsb = m_bankList[m_bankCombo->currentItem()].lsb;

        // Copy across all programs that aren't in the doomed bank
        //
        MidiProgramContainer::iterator it;
        MidiProgramContainer tempList;
        for (it = m_programList.begin(); it != m_programList.end(); it++)
            if (it->msb != msb || it->lsb != lsb)
                tempList.push_back(*it);

        // Erase the bank and repopulate
        //
        MidiBankContainer::iterator er = m_bankList.begin();
        er += m_bankCombo->currentItem();
        m_bankList.erase(er);
        m_programList = tempList;

        slotPopulateDeviceBank(m_deviceCombo->currentItem(), newBank);

        setModified(true);
    }

    _newBank = false;

}

void
BankEditorDialog::slotDeleteAllBanks()
{
    _newBank = true;

    Rosegarden::MidiDevice *device =
        getMidiDevice(m_deviceCombo->currentItem());

    if (device)
    {
        device->clearProgramList();
        device->clearBankList();
        slotPopulateDevice(m_deviceCombo->currentItem());
    }

    setModified(true);

    _newBank = false;

}


Rosegarden::MidiDevice*
BankEditorDialog::getMidiDevice(int number)
{
    Rosegarden::DeviceList *devices = m_studio->getDevices();
    if (number > int(devices->size()))
        return 0;

    int count = 0;
    Rosegarden::DeviceListIterator it = devices->begin();
    for (; it != devices->end(); it++)
    {
        if ((*it)->getType() == Rosegarden::Device::Midi)
        {
            if (count == number)
                break;
            count++;
        }
    }

    if (it == devices->end())
        return 0;

    return dynamic_cast<Rosegarden::MidiDevice*>(*it);
}

// Try to find a unique MSB/LSB pair for a new bank
//
std::pair<int, int>
BankEditorDialog::getFirstFreeBank(int devNo)
{
    int msb = 0;
    int lsb = 0;

    Rosegarden::MidiDevice *device = getMidiDevice(devNo);

    if (device)
    {
        do
        {
            lsb = 0;
            while(banklistContains(msb, lsb) && lsb < 128)
                lsb++;
        }
        while (lsb == 128 && msb++);
    }

    return std::pair<int, int>(msb, lsb);
}



void
BankEditorDialog::slotModifyBankName(const QString &label)
{
    /*
    const QObject *s = sender();
    const QComboBox *comboBox = dynamic_cast<const QComboBox*>(s);
    if (comboBox) return;
    */

    if (_newBank) return;

    //m_bankCombo->changeItem(label, m_bankCombo->currentItem());

    if (label != strtoqstr(m_bankList[m_bankCombo->currentItem()].name))
    {
        m_bankList[m_bankCombo->currentItem()].name = qstrtostr(label);
        setModified(true);
    }
}

void
BankEditorDialog::slotModifyDeviceName(const QString &label)
{
    /*
    const QObject *s = sender();
    const QComboBox *comboBox = dynamic_cast<const QComboBox*>(s);
    if (comboBox) return;
    */

    if (_newBank || m_deviceCombo->currentItem() > (int)(m_deviceList.size()))
        return;
    
    /*
    Rosegarden::DeviceList *devices = m_studio->getDevices();
    if (label != strtoqstr((*devices)[m_deviceCombo->currentItem()]->getName()))
    */
    if (label != strtoqstr(m_deviceList[m_deviceCombo->currentItem()]))
    {
        /*
        disconnect(m_deviceCombo, SIGNAL(textChanged(const QString&)),
                   this, SLOT(slotModifyDeviceName(const QString&)));

        connect(m_deviceCombo, SIGNAL(textChanged(const QString&)),
                this, SLOT(slotModifyDeviceName(const QString&)));
                */
        m_deviceList[m_deviceCombo->currentItem()] = qstrtostr(label);
        m_deviceCombo->changeItem(label, m_deviceCombo->currentItem());

        setModified(true);
    }
}

void
BankEditorDialog::setModified(bool value)
{

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
BankEditorDialog::slotProgramChanged(const QString &programName, int id)
{
    //RG_DEBUG << "PROGRAM (" << id << ") = \"" << program << "\"" << endl;

    Rosegarden::MidiProgram *program =
        getProgram(m_bankList[m_bankCombo->currentItem()].msb,
                   m_bankList[m_bankCombo->currentItem()].lsb,
                   id);

    if (program == 0)
    {
        program = new Rosegarden::MidiProgram();
        program->msb = m_bankList[m_bankCombo->currentItem()].msb;
        program->lsb = m_bankList[m_bankCombo->currentItem()].lsb;
        program->program = id;
        m_programList.push_back(*program);

        // Now, get with the program
        //
        program =
            getProgram(m_bankList[m_bankCombo->currentItem()].msb,
                       m_bankList[m_bankCombo->currentItem()].lsb,
                       id);
    }

    if (qstrtostr(programName) != program->name)
    {
        program->name = qstrtostr(programName);
        setModified(true);
    }
}

Rosegarden::MidiProgram*
BankEditorDialog::getProgram(int msb, int lsb, int program)
{
    MidiProgramContainer::iterator it = m_programList.begin();

    for (; it != m_programList.end(); it++)
    {
        if (it->msb == msb && it->lsb == lsb && it->program == program)
            return &(*it);
    }

    return 0;

}


void
BankEditorDialog::slotNewMSB(int value)
{
    if (_newBank == false)
    {
        disconnect(m_msb, SIGNAL(valueChanged(int)),
                   this, SLOT(slotNewMSB(int)));

        int msb;

        try
        {
            msb = ensureUniqueMSB(value, value >
                             m_bankList[m_bankCombo->currentItem()].msb);
        }
        catch(bool)
        {
            msb = m_bankList[m_bankCombo->currentItem()].msb;
        }

        modifyCurrentPrograms(m_bankList[m_bankCombo->currentItem()].msb,
                              m_bankList[m_bankCombo->currentItem()].lsb,
                              msb,
                              m_bankList[m_bankCombo->currentItem()].lsb);

        m_msb->setValue(msb);
        m_bankList[m_bankCombo->currentItem()].msb = msb;

        connect(m_msb, SIGNAL(valueChanged(int)),
                this, SLOT(slotNewMSB(int)));

        setModified(true);
    }

}

void
BankEditorDialog::slotNewLSB(int value)
{
    if (_newBank == false)
    {
        disconnect(m_lsb, SIGNAL(valueChanged(int)),
                   this, SLOT(slotNewLSB(int)));

        int lsb;

        try
        {
            lsb = ensureUniqueLSB(value, value >
                         m_bankList[m_bankCombo->currentItem()].lsb);
        }
        catch(bool)
        {
            lsb = m_bankList[m_bankCombo->currentItem()].lsb;
        }

        modifyCurrentPrograms(m_bankList[m_bankCombo->currentItem()].msb,
                              m_bankList[m_bankCombo->currentItem()].lsb,
                              m_bankList[m_bankCombo->currentItem()].msb,
                              lsb);

        m_lsb->setValue(lsb);
        m_bankList[m_bankCombo->currentItem()].lsb = lsb;

        connect(m_lsb, SIGNAL(valueChanged(int)),
                this, SLOT(slotNewLSB(int)));

        setModified(true);
    }
}

int
BankEditorDialog::ensureUniqueMSB(int msb, bool ascending)
{
    int newMSB = msb;
    while (banklistContains(newMSB, m_lsb->value())
           && newMSB < 128
           && newMSB > -1)
        if (ascending) newMSB++;
        else newMSB--;

   if (newMSB == -1 || newMSB == 128)
       throw bool(false);

    return newMSB;
}

int
BankEditorDialog::ensureUniqueLSB(int lsb, bool ascending)
{
    int newLSB = lsb;
    while (banklistContains(m_msb->value(), newLSB)
           && newLSB < 128
           && newLSB > -1)
        if (ascending) newLSB++;
        else newLSB--;

   if (newLSB == -1 || newLSB == 128)
       throw bool(false);

    return newLSB;
}

bool
BankEditorDialog::banklistContains(int msb, int lsb)
{
    MidiBankContainer::iterator it;

    for (it = m_bankList.begin(); it != m_bankList.end(); it++)
        if (it->msb == msb && it->lsb == lsb)
            return true;

    return false;
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
BankEditorDialog::modifyCurrentPrograms(int oldMSB, int oldLSB,
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


// ---------------------- ProgramLine -----------------------------
//
ProgramLine::ProgramLine(QWidget *parent, int id):
    QLineEdit(parent), m_id(id)
{
    connect (this, SIGNAL(textChanged(const QString&)),
             this, SLOT(slotNewText(const QString&)));
}


void
ProgramLine::slotNewText(const QString &label)
{
    if (_newBank == false)
    emit newText(label, m_id);
}

// ------------------------ RemapInstrumentDialog -----------------------
//
RemapInstrumentDialog::RemapInstrumentDialog(QWidget *parent,
                                             RosegardenGUIDoc *doc):
    KDialogBase(parent, "", true, i18n("Remap Instruments and Devices..."),
                Ok | Apply | Cancel),
    m_doc(doc)
{
    QVBox *vBox = makeVBoxMainWidget();

    QButtonGroup *buttonGroup = new QButtonGroup(i18n("Remap"), vBox);

    m_deviceButton = new QRadioButton(i18n("Devices"), vBox);
    m_instrumentButton = new QRadioButton(i18n("Instruments"), vBox);

    buttonGroup->insert(m_deviceButton);
    buttonGroup->insert(m_instrumentButton);

}



