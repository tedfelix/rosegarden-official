// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
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

#include <iostream>
#include <klocale.h>

#include <cstdio>

#include <qdial.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>

#include "Instrument.h"
#include "MidiDevice.h"
#include "instrumentparameterbox.h"
#include "widgets.h"

#include "rosedebug.h"

InstrumentParameterBox::InstrumentParameterBox(QWidget *parent,
                                               const char *name,
                                               WFlags f)
    : QGroupBox(i18n("Instrument Parameters"), parent, name),
      m_bankValue(new RosegardenComboBox(true, false, this)),
      m_channelValue(new RosegardenComboBox(true, false, this)),
      m_programValue(new RosegardenComboBox(true, false, this)),
      m_panValue(new RosegardenComboBox(true, false, this)),
      m_velocityValue(new RosegardenComboBox(true, false, this)),
      m_bankCheckBox(new QCheckBox(this)),
      m_programCheckBox(new QCheckBox(this)),
      m_panCheckBox(new QCheckBox(this)),
      m_velocityCheckBox(new QCheckBox(this)),
      m_selectedInstrument(0)
{
    initBox();
}

InstrumentParameterBox::~InstrumentParameterBox()
{
}

void
InstrumentParameterBox::initBox()
{
    QFont plainFont;
    plainFont.setPointSize(10);

    setFont(plainFont);

    QGridLayout *gridLayout = new QGridLayout(this, 6, 3, 8, 1);

    QLabel *channelLabel = new QLabel(i18n("Channel"), this);
    QLabel *panLabel = new QLabel(i18n("Pan"), this);
    QLabel *velocityLabel = new QLabel(i18n("Vely"), this);
    QLabel *programLabel = new QLabel(i18n("Prg"), this);
    QLabel *bankLabel = new QLabel(i18n("Bank"), this);

    gridLayout->addRowSpacing(0, 8);
    gridLayout->addWidget(bankLabel,      1, 0, AlignLeft);
    gridLayout->addWidget(m_bankCheckBox, 1, 1);
    gridLayout->addWidget(m_bankValue,    1, 2, AlignRight);

    // program label under heading - filling the entire cell
    gridLayout->addWidget(programLabel,      2, 0);
    gridLayout->addWidget(m_programCheckBox, 2, 1);
    gridLayout->addWidget(m_programValue,    2, 2, AlignRight);

    gridLayout->addMultiCellWidget(channelLabel, 3, 3, 0, 1, AlignLeft);
    gridLayout->addWidget(m_channelValue, 3, 2, AlignRight);

    gridLayout->addWidget(panLabel,      4, 0, AlignLeft);
    gridLayout->addWidget(m_panCheckBox, 4, 1);
    gridLayout->addWidget(m_panValue,    4, 2, AlignRight);

    gridLayout->addWidget(velocityLabel,      5, 0, AlignLeft);
    gridLayout->addWidget(m_velocityCheckBox, 5, 1);
    gridLayout->addWidget(m_velocityValue,    5, 2, AlignRight);

    // Populate channel list
    for (int i = 0; i < 16; i++)
        m_channelValue->insertItem(QString("%1").arg(i));

    // Populate pan
    //
    QString mod;

    for (int i = -Rosegarden::MidiMidValue;
             i < Rosegarden::MidiMidValue + 1; i++)
    {
        if (i > 0)
            mod = QString("+");

        m_panValue->insertItem(mod + QString("%1").arg(i));
    }

    // velocity values
    //
    for (int i = 0; i < Rosegarden::MidiMaxValue + 1; i++)
        m_velocityValue->insertItem(QString("%1").arg(i));


    // Disable these three by default - they are activate by their
    // checkboxes
    //
    m_panValue->setDisabled(true);
    m_programValue->setDisabled(true);
    m_velocityValue->setDisabled(true);
    m_bankValue->setDisabled(true);

    // Only active is we have an Instrument selected
    //
    m_panCheckBox->setDisabled(true);
    m_velocityCheckBox->setDisabled(true);
    m_programCheckBox->setDisabled(true);
    m_bankCheckBox->setDisabled(true);

    // Connect up the toggle boxes
    //
    connect(m_panCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotActivatePan(bool)));

    connect(m_velocityCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotActivateVelocity(bool)));

    connect(m_programCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotActivateProgramChange(bool)));

    connect(m_bankCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotActivateBank(bool)));


    // Connect activations
    //
    connect(m_bankValue, SIGNAL(activated(int)),
            this, SLOT(slotSelectBank(int)));

    connect(m_panValue, SIGNAL(activated(int)),
            this, SLOT(slotSelectPan(int)));

    connect(m_programValue, SIGNAL(activated(int)),
            this, SLOT(slotSelectProgram(int)));

    connect(m_velocityValue, SIGNAL(activated(int)),
            this, SLOT(slotSelectVelocity(int)));

    connect(m_channelValue, SIGNAL(activated(int)),
            this, SLOT(slotSelectChannel(int)));

    // connect up mouse wheel movement
    //
    connect(m_bankValue, SIGNAL(propagate(int)),
            this, SLOT(slotSelectBank(int)));

    connect(m_panValue, SIGNAL(propagate(int)),
            this, SLOT(slotSelectPan(int)));

    connect(m_programValue, SIGNAL(propagate(int)),
            this, SLOT(slotSelectProgram(int)));

    connect(m_velocityValue, SIGNAL(propagate(int)),
            this, SLOT(slotSelectVelocity(int)));

    connect(m_channelValue, SIGNAL(propagate(int)),
            this, SLOT(slotSelectChannel(int)));

    // don't select any of the options in any dropdown
    m_panValue->setCurrentItem(-1);
    m_velocityValue->setCurrentItem(-1);
    m_programValue->setCurrentItem(-1);
    m_bankValue->setCurrentItem(-1);
    m_channelValue->setCurrentItem(-1);

}

void
InstrumentParameterBox::useInstrument(Rosegarden::Instrument *instrument)
{
    kdDebug(KDEBUG_AREA) << "useInstrument() - populate Instrument\n";

    if (instrument == 0)
        return;

    // for the moment ignore everything other than MIDI
    //
    if (instrument->getType() != Rosegarden::Instrument::Midi)
        return;

    m_selectedInstrument = instrument;

    // Enable all check boxes
    //
    m_panCheckBox->setDisabled(false);
    m_velocityCheckBox->setDisabled(false);
    m_programCheckBox->setDisabled(false);
    m_bankCheckBox->setDisabled(false);

    // Activate all checkboxes
    //
    m_panCheckBox->setChecked(m_selectedInstrument->sendsPan());
    m_velocityCheckBox->setChecked(m_selectedInstrument->sendsVelocity());
    m_programCheckBox->setChecked(m_selectedInstrument->sendsProgramChange());
    m_bankCheckBox->setChecked(m_selectedInstrument->sendsBankSelect());

    // The MIDI channel
    m_channelValue->setCurrentItem((int)instrument->getMidiChannel());

    // Check for pan
    //
    if (instrument->sendsPan())
    {
        m_panValue->setDisabled(false);
        m_panValue->setCurrentItem(instrument->getPan());
    }
    else
    {
        m_panValue->setDisabled(true);
        m_panValue->setCurrentItem(-1);
    }

    // Check for program change
    //
    if (instrument->sendsProgramChange())
    {
        m_programValue->setDisabled(false);
        populateProgramList();
    }
    else
    {
        m_programValue->setDisabled(true);
        m_programValue->setCurrentItem(-1);
    }

    // Check for velocity
    //
    if (instrument->sendsVelocity())
    {
        m_velocityValue->setDisabled(false);
    }
    else
    {
        m_velocityValue->setDisabled(true);
        m_velocityValue->setCurrentItem(-1);
    }

    // clear bank list
    m_bankValue->clear();

    // create bank list
    Rosegarden::BankList list = 
        dynamic_cast<Rosegarden::MidiDevice*>
            (instrument->getDevice())->getBankList();

    Rosegarden::BankList::iterator it;

    for (it = list.begin(); it != list.end(); it++)
        m_bankValue->insertItem(QString((*it).data()));

    // Select 
    if (instrument->sendsBankSelect())
    {
        //m_bankValue
    }
    else
    {
        m_bankValue->setDisabled(true);
        m_bankValue->setCurrentItem(-1);
    }

}

void
InstrumentParameterBox::slotActivateProgramChange(bool value)
{
    if (m_selectedInstrument == 0)
    {
        m_programCheckBox->setChecked(false);
        return;
    }

    m_selectedInstrument->setSendProgramChange(value);
    m_programValue->setDisabled(!value);
    populateProgramList();
}

void
InstrumentParameterBox::slotActivateVelocity(bool value)
{
    if (m_selectedInstrument == 0)
    {
        m_velocityCheckBox->setChecked(false);
        return;
    }

    m_selectedInstrument->setSendVelocity(value);
    m_velocityValue->setDisabled(!value);
    m_velocityValue->setCurrentItem(m_selectedInstrument->getVelocity());
}

void
InstrumentParameterBox::slotActivatePan(bool value)
{
    if (m_selectedInstrument == 0)
    {
        m_panCheckBox->setChecked(false);
        return;
    }

    m_selectedInstrument->setSendPan(value);
    m_panValue->setDisabled(!value);

    m_panValue->setCurrentItem(m_selectedInstrument->getPan());
}

void
InstrumentParameterBox::slotActivateBank(bool value)
{
    if (m_selectedInstrument == 0)
    {
        m_bankCheckBox->setChecked(false);
        return;
    }

    m_selectedInstrument->setSendBankSelect(value);
    m_bankValue->setDisabled(!value);

}


void
InstrumentParameterBox::slotSelectProgram(int index)
{
    if (m_selectedInstrument == 0)
        return;

    Rosegarden::MidiProgram *prg = 
        dynamic_cast<Rosegarden::MidiDevice*>
            (m_selectedInstrument->getDevice())->getProgramByIndex(index);

    m_selectedInstrument->setProgramChange(prg->program);

}


void
InstrumentParameterBox::slotSelectPan(int index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setPan(index);

    // Send the controller change
    //
    emit sendMidiController(m_selectedInstrument->getID(), 
                            (Rosegarden::MidiByte)10,
                            (Rosegarden::MidiByte)index);
}

void
InstrumentParameterBox::slotSelectVelocity(int index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setVelocity(index);
}

void
InstrumentParameterBox::slotSelectBank(int index)
{
    if (m_selectedInstrument == 0)
        return;

    Rosegarden::MidiBank *bank = 
        dynamic_cast<Rosegarden::MidiDevice*>
            (m_selectedInstrument->getDevice())->getBankByIndex(index);

    m_selectedInstrument->setMSB(bank->msb);
    m_selectedInstrument->setLSB(bank->lsb);

    // repopulate program list
    populateProgramList();
}

void
InstrumentParameterBox::slotSelectChannel(int index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setMidiChannel(index);
}





// Populate program list by bank context
//
void
InstrumentParameterBox::populateProgramList()
{
    if (m_selectedInstrument == 0)
        return;

    // The program list
    m_programValue->clear();


    Rosegarden::MidiByte msb = 0;
    Rosegarden::MidiByte lsb = 0;

    if (m_selectedInstrument->sendsBankSelect())
    {
        msb = m_selectedInstrument->getMSB();
        lsb = m_selectedInstrument->getLSB();
    }

    Rosegarden::ProgramList list = 
        dynamic_cast<Rosegarden::MidiDevice*>
            (m_selectedInstrument->getDevice())->getProgramList(msb, lsb);

    Rosegarden::ProgramList::iterator it;

    for (it = list.begin(); it != list.end(); it++)
        m_programValue->insertItem(QString((*it).data()));

    m_programValue->setCurrentItem(
            (int)m_selectedInstrument->getProgramChange());
}



