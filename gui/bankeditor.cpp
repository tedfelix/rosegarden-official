
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
#include <qhbox.h>
#include <qvbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qtabwidget.h>
#include <qgroupbox.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qspinbox.h>

#include "bankeditor.h"
#include "widgets.h"
#include "rosestrings.h"

#include "Studio.h"
#include "MidiDevice.h"

BankEditorDialog::BankEditorDialog(QWidget *parent,
                                   Rosegarden::Studio *studio):
    KDialogBase(parent, "", true, i18n("Manage Banks and Programs..."),
                Ok | Apply | Cancel),
    m_studio(studio),
    m_modified(false)
{


    QVBox *vBox = makeVBoxMainWidget();

    QGroupBox *deviceBox  = new QGroupBox(3,
                                          Qt::Horizontal,
                                          i18n("Select MIDI Device..."),
                                          vBox);

    deviceBox->addSpace(0);
    deviceBox->addSpace(0);
    m_deviceCombo = new RosegardenComboBox(false, deviceBox);
    m_deviceCombo->setEditable(true);

    Rosegarden::DeviceList *devices = studio->getDevices();
    Rosegarden::DeviceListIterator it;
    for (it = devices->begin(); it != devices->end(); it++)
    {
        if ((*it)->getType() == Rosegarden::Device::Midi)
        {
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

    gridLayout->addWidget(new QLabel(i18n("LSB Value"), m_mainFrame),
                          2, 4, AlignRight);
    m_lsb = new QSpinBox(m_mainFrame);
    m_lsb->setMinValue(0);
    m_lsb->setMaxValue(127);
    gridLayout->addWidget(m_lsb, 2, 5, AlignRight);

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

            m_programNames.push_back(new QLineEdit(numBox));
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

            m_programNames.push_back(new QLineEdit(numBox));
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

    /*
    // modification
    connect(m_deviceCombo, SIGNAL(textChanged(const QString&)),
            this, SLOT(slotModifyDeviceName(const QString&)));

    connect(m_bankCombo, SIGNAL(textChanged(const QString&)),
            this, SLOT(slotModifyBankName(const QString&)));
            */

    connect(m_addBank, SIGNAL(released()),
            this, SLOT(slotAddBank()));

    connect(m_deleteBank, SIGNAL(released()),
            this, SLOT(slotDeleteBank()));

    connect(m_deleteAllBanks, SIGNAL(released()),
            this, SLOT(slotDeleteAllBanks()));
}

void
BankEditorDialog::slotPopulateDeviceBank(int deviceNo, int bank)
{
    Rosegarden::MidiDevice *device = getMidiDevice(deviceNo);

    if (device)
    {
        m_bankCombo->clear();

        if (m_bankList.size() > 0)
        {
            m_mainFrame->setDisabled(false);
            for (unsigned int i = 0; i < m_bankList.size(); i++)
            {
                m_bankCombo->insertItem(strtoqstr(m_bankList[i].name));
            }

            m_msb->setValue(m_bankList[bank].msb);
            m_lsb->setValue(m_bankList[bank].lsb);

            m_bankCombo->setCurrentItem(bank);
            slotPopulateBank(bank);
        }
        else
        {
            // no banks
            m_mainFrame->setDisabled(true);
        }

    }
}

void
BankEditorDialog::slotPopulateDevice(int devNo)
{
    /*
    disconnect(m_deviceCombo, SIGNAL(textChanged(const QString&)),
               this, SLOT(slotModifyDeviceName(const QString&)));

    disconnect(m_bankCombo, SIGNAL(textChanged(const QString&)),
               this, SLOT(slotModifyBankName(const QString&)));
               */

    if (m_modified)
    {
        // then ask if we want to apply the changes
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

    /*

    connect(m_deviceCombo, SIGNAL(textChanged(const QString&)),
            this, SLOT(slotModifyDeviceName(const QString&)));

    connect(m_bankCombo, SIGNAL(textChanged(const QString&)),
            this, SLOT(slotModifyBankName(const QString&)));
            */

}




void
BankEditorDialog::slotPopulateBank(int bank)
{
    Rosegarden::MidiDevice *device =
        getMidiDevice(m_deviceCombo->currentItem());

    /*
    disconnect(m_deviceCombo, SIGNAL(textChanged(const QString&)),
               this, SLOT(slotModifyDeviceName(const QString&)));

    disconnect(m_bankCombo, SIGNAL(textChanged(const QString&)),
               this, SLOT(slotModifyBankName(const QString&)));
    */

    if (device)
    {
        // set the bank values
        m_msb->setValue(m_bankList[bank].msb);
        m_lsb->setValue(m_bankList[bank].lsb);
        std::vector<Rosegarden::MidiProgram> programSubset 
            = getBankSubset(m_bankList[bank].msb,
                            m_bankList[bank].lsb);
        std::vector<Rosegarden::MidiProgram>::iterator it;

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

    /*
    connect(m_deviceCombo, SIGNAL(textChanged(const QString&)),
            this, SLOT(slotModifyDeviceName(const QString&)));

    connect(m_bankCombo, SIGNAL(textChanged(const QString&)),
            this, SLOT(slotModifyBankName(const QString&)));
            */

}

std::vector<Rosegarden::MidiProgram>
BankEditorDialog::getBankSubset(Rosegarden::MidiByte msb,
                                Rosegarden::MidiByte lsb)
{
    std::vector<Rosegarden::MidiProgram> program;
    std::vector<Rosegarden::MidiProgram>::iterator it;

    for (it = m_programList.begin(); it != m_programList.end(); it++)
    {
        if (it->msb == msb && it->lsb == lsb)
            program.push_back(*it);
    }

    return program;
    
}


void
BankEditorDialog::slotOK()
{
}

void
BankEditorDialog::slotApply()
{
}

void
BankEditorDialog::slotAddBank()
{
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

        m_bankList.push_back(*newBank);

        slotPopulateDeviceBank(m_deviceCombo->currentItem(),
                               m_bankCombo->count());
    }

}

void
BankEditorDialog::slotDeleteBank()
{
    Rosegarden::MidiDevice *device =
        getMidiDevice(m_deviceCombo->currentItem());

    if (device)
    {
        slotPopulateDevice(m_deviceCombo->currentItem());
    }

}

void
BankEditorDialog::slotDeleteAllBanks()
{
    Rosegarden::MidiDevice *device =
        getMidiDevice(m_deviceCombo->currentItem());

    if (device)
    {
        device->clearProgramList();
        device->clearBankList();
        slotPopulateDevice(m_deviceCombo->currentItem());
    }

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

std::pair<int, int>
BankEditorDialog::getFirstFreeBank(int devNo)
{
    int msb = 0;
    int lsb = 0;

    Rosegarden::MidiDevice *device = getMidiDevice(devNo);

    if (device)
    {

        std::vector<Rosegarden::MidiBank>::iterator it;
        std::vector<int> allMSB;
        std::vector<int> allLSB;

        for (it = m_bankList.begin(); it != m_bankList.end(); it++)
        {
            allMSB.push_back(it->msb);
            allLSB.push_back(it->lsb);
        }

        do
        {
            lsb = 0;

            while(std::find(allLSB.begin(), allLSB.end(), lsb) != allLSB.end()
                  && lsb < 128)
                lsb++;
        }
        while (lsb == 128 && msb++);

        /*
        while(std::find(allMSB.begin(), allMSB.end(), msb) != allMSB.end()
              && msb < 128)
            msb++;
            */

    }

    return std::pair<int, int>(msb, lsb);
}



void
BankEditorDialog::slotModifyBankName(const QString &label)
{
    m_bankList[m_bankCombo->currentItem()].name = qstrtostr(label);
    m_bankCombo->changeItem(label, m_bankCombo->currentItem());

    setModified(true);
}

void
BankEditorDialog::slotModifyDeviceName(const QString &label)
{
    m_deviceCombo->changeItem(label, m_deviceCombo->currentItem());

    setModified(true);
}

void
BankEditorDialog::setModified(bool value)
{
    if (m_modified == value) return;

    if (value)
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


