
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

#include <klocale.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qtabwidget.h>
#include <qgroupbox.h>
#include <qlayout.h>
#include <qpushbutton.h>

#include "bankeditor.h"
#include "widgets.h"
#include "Studio.h"
#include "MidiDevice.h"
#include "rosestrings.h"

BankEditorDialog::BankEditorDialog(QWidget *parent,
                                   Rosegarden::Studio *studio):
    KDialogBase(parent, "", true, i18n("Manage Banks and Programs..."),
                Ok | Apply | Cancel),
    m_studio(studio)
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
                                       i18n("Banks and Program details"),
                                       vBox);

   m_mainFrame = new QFrame(groupBox);
   QGridLayout *gridLayout = new QGridLayout(m_mainFrame,
                                             1,  // rows
                                             6,  // cols
                                             2); // margin


   gridLayout->addWidget(new QLabel(i18n("Bank Name"), m_mainFrame), 0, 0, AlignRight);
   m_bankCombo = new RosegardenComboBox(false, m_mainFrame);
   m_bankCombo->setEditable(true);
   gridLayout->addWidget(m_bankCombo, 0, 1, AlignLeft);

   gridLayout->addWidget(new QLabel(i18n("MSB Value"), m_mainFrame), 1, 0, AlignRight);
   m_msb = new QSpinBox(m_mainFrame);
   m_msb->setMinValue(0);
   m_msb->setMaxValue(127);
   gridLayout->addWidget(m_msb, 1, 1, AlignLeft);

   gridLayout->addWidget(new QLabel(i18n("LSB Value"), m_mainFrame), 2, 0, AlignRight);
   m_lsb = new QSpinBox(m_mainFrame);
   m_lsb->setMinValue(0);
   m_lsb->setMaxValue(127);
   gridLayout->addWidget(m_lsb, 2, 1, AlignLeft);

   /*
   gridLayout->addWidget(new QLabel(i18n("Manage Banks:"), frame),
                         0, 4, AlignCenter);
   gridLayout->addWidget(m_addBank, 0, 5, AlignLeft);
   gridLayout->addWidget(m_deleteBank, 1, 5, AlignLeft);
   gridLayout->addWidget(m_deleteAllBanks, 2, 5, AlignLeft);
   */

   // spacer
   gridLayout->addRowSpacing(3, 15);

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
           this, SLOT(slotPopulatePrograms(int)));

   connect(m_bankCombo, SIGNAL(propagate(int)),
           this, SLOT(slotPopulatePrograms(int)));

   slotPopulateDevice(0);

}

void
BankEditorDialog::slotPopulateDevice(int deviceNo)
{
    Rosegarden::DeviceList *devices = m_studio->getDevices();
    if (deviceNo > int(devices->size()))
        return;

    int count = 0;
    Rosegarden::DeviceListIterator it = devices->begin();
    for (; it != devices->end(); it++)
    {
        if ((*it)->getType() == Rosegarden::Device::Midi)
        {
            if (count == deviceNo)
                break;
            count++;
        }
    }

    if (it == devices->end())
        return;

    Rosegarden::MidiDevice *device = dynamic_cast<Rosegarden::MidiDevice*>(*it);

    if (device)
    {
        m_bankCombo->clear();
        Rosegarden::StringList banks = device->getBankList();

        if (banks.size() > 0)
        {
            m_mainFrame->setDisabled(false);
            for (unsigned int i = 0; i < banks.size(); i++)
            {
                m_bankCombo->insertItem(strtoqstr(banks[i]));
            }

            m_msb->setValue(device->getBankByIndex(0)->msb);
            m_lsb->setValue(device->getBankByIndex(0)->lsb);

            slotPopulatePrograms(0);
        }
        else
        {
            // no banks
            m_mainFrame->setDisabled(true);
        }

    }
}



void
BankEditorDialog::slotPopulatePrograms(int bank)
{
    Rosegarden::DeviceList *devices = m_studio->getDevices();
    if (m_deviceCombo->currentItem() > int(devices->size()))
        return;

    int count = 0;
    Rosegarden::DeviceListIterator it = devices->begin();
    for (; it != devices->end(); it++)
    {
        if ((*it)->getType() == Rosegarden::Device::Midi)
        {
            if (count == m_deviceCombo->currentItem())
                break;

            count++;
        }
    }

    if (it == devices->end())
        return;

    Rosegarden::MidiDevice *device = dynamic_cast<Rosegarden::MidiDevice*>(*it);

    if (device)
    {
        // set the bank values
        m_msb->setValue(device->getBankByIndex(bank)->msb);
        m_lsb->setValue(device->getBankByIndex(bank)->lsb);

        Rosegarden::StringList programs =
            device->getProgramList(device->getBankByIndex(bank)->msb,
                                   device->getBankByIndex(bank)->lsb);

        for (unsigned int i = 0; i < 128; i++)
        {
            if (i < programs.size())
                m_programNames[i]->setText(strtoqstr(programs[i]));
            else
                m_programNames[i]->setText("");

            m_programNames[i]->setCursorPosition(0);
        }

    }
}

