
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

#include "bankeditor.h"
#include "widgets.h"
#include "Studio.h"
#include "MidiDevice.h"
#include "rosestrings.h"

BankEditorDialog::BankEditorDialog(QWidget *parent,
                                   Rosegarden::Studio *studio):
    KDialogBase(parent, "", true, i18n("Bank Editor Dialog"),
                Ok | Cancel),
    m_studio(studio)
{
   QVBox *vBox = makeVBoxMainWidget();

   QHBox *bankHBox = new QHBox(vBox);

   new QLabel(i18n("Device"), bankHBox);
   m_deviceCombo = new RosegardenComboBox(true, bankHBox);

   Rosegarden::DeviceList *devices = studio->getDevices();
   Rosegarden::DeviceListIterator it;
   for (it = devices->begin(); it != devices->end(); it++)
   {
       if ((*it)->getType() == Rosegarden::Device::Midi)
       {
           m_deviceCombo->insertItem(strtoqstr((*it)->getName()));
       }
   }


   new QLabel(i18n("Bank"), bankHBox);
   m_bankCombo = new RosegardenComboBox(true, bankHBox);

   new QLabel(i18n("MSB"), bankHBox);
   m_msb = new QSpinBox(bankHBox);
   new QLabel(i18n("LSB"), bankHBox);
   m_lsb = new QSpinBox(bankHBox);

   m_programTab = new QTabWidget(vBox);
   m_programTab->setMargin(10);

   QHBox *progHBox = new QHBox(vBox);
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

   progHBox = new QHBox(vBox);
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

   slotPopulateDevice(0);

   connect(m_deviceCombo, SIGNAL(activated(int)),
           this, SLOT(slotPopulateDevice(int)));

   connect(m_bankCombo, SIGNAL(activated(int)),
           this, SLOT(slotPopulatePrograms(int)));

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
        for (unsigned int i = 0; i < banks.size(); i++)
        {
            m_bankCombo->insertItem(strtoqstr(banks[i]));
        }

        m_msb->setValue(device->getBankByIndex(0)->msb);
        m_lsb->setValue(device->getBankByIndex(0)->lsb);
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
        Rosegarden::StringList programs =
            device->getProgramList(device->getBankByIndex(bank)->msb,
                                   device->getBankByIndex(bank)->lsb);

        for (unsigned int i = 0; i < programs.size(); i++)
        {
            m_programNames[i]->setText(strtoqstr(programs[i]));
        }

    }
}

