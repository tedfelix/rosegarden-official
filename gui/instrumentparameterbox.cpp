// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.2
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

#include "Midi.h"
#include "Instrument.h"
#include "MidiDevice.h"
#include "instrumentparameterbox.h"
#include "widgets.h"

#include "rosestrings.h"
#include "rosedebug.h"

InstrumentParameterBox::InstrumentParameterBox(QWidget *parent)
    : RosegardenParameterBox(i18n("Instrument Parameters"), parent),
      m_instrumentLabel(new QLabel(this)),
      m_channelLabel(new QLabel(i18n("Channel"), this)),
      m_panLabel(new QLabel(i18n("Pan"), this)),
      m_velocityLabel(new QLabel(i18n("Velocity"), this)),
      m_programLabel(new QLabel(i18n("Program"), this)),
      m_bankLabel(new QLabel(i18n("Bank"), this)),
      m_bankValue(new RosegardenComboBox(false, false, this)),
      m_channelValue(new RosegardenComboBox(true, false, this)),
      m_programValue(new RosegardenComboBox(false, false, this)),
      m_panValue(new RosegardenComboBox(true, false, this)),
      m_velocityValue(new RosegardenComboBox(true, false, this)),
      m_bankCheckBox(new QCheckBox(this)),
      m_programCheckBox(new QCheckBox(this)),
      m_panCheckBox(new QCheckBox(this)),
      m_velocityCheckBox(new QCheckBox(this)),
      m_selectedInstrument(0)
{
    initBox();

    bool contains = false;

    std::vector<InstrumentParameterBox*>::iterator it =
        instrumentParamBoxes.begin();

    for (; it != instrumentParamBoxes.end(); it++)
        if ((*it) == this)
            contains = true;

    if (!contains)
        instrumentParamBoxes.push_back(this);
}

InstrumentParameterBox::~InstrumentParameterBox()
{
    // deregister this paramter box
    std::vector<InstrumentParameterBox*>::iterator it =
        instrumentParamBoxes.begin();

    for (; it != instrumentParamBoxes.end(); it++)
    {
        if ((*it) == this)
        {
            instrumentParamBoxes.erase(it);
            break;
        }
    }
}

void
InstrumentParameterBox::initBox()
{
    QGridLayout *gridLayout = new QGridLayout(this, 6, 3, 8, 1);

    m_instrumentLabel->setFont(getFont());

    m_channelLabel->setFont(getFont());
    m_panLabel->setFont(getFont());
    m_velocityLabel->setFont(getFont());
    m_programLabel->setFont(getFont());
    m_bankLabel->setFont(getFont());

    m_bankValue->setFont(getFont());
    m_programValue->setFont(getFont());
    m_channelValue->setFont(getFont());
    m_panValue->setFont(getFont());
    m_velocityValue->setFont(getFont());

    gridLayout->addRowSpacing(0, 8);

    gridLayout->addRowSpacing(1, 30);
    gridLayout->addMultiCellWidget(m_instrumentLabel, 1, 1, 0, 2, AlignCenter);

    gridLayout->addWidget(m_bankLabel,    2, 0, AlignLeft);
    gridLayout->addWidget(m_bankCheckBox, 2, 1);
    gridLayout->addWidget(m_bankValue,    2, 2, AlignRight);

    // program label under heading - filling the entire cell
    gridLayout->addWidget(m_programLabel,    3, 0);
    gridLayout->addWidget(m_programCheckBox, 3, 1);
    gridLayout->addWidget(m_programValue,    3, 2, AlignRight);

    gridLayout->addMultiCellWidget(m_channelLabel, 4, 4, 0, 1, AlignLeft);
    gridLayout->addWidget(m_channelValue, 4, 2, AlignRight);

    gridLayout->addWidget(m_panLabel,    5, 0, AlignLeft);
    gridLayout->addWidget(m_panCheckBox, 5, 1);
    gridLayout->addWidget(m_panValue,    5, 2, AlignRight);

    gridLayout->addWidget(m_velocityLabel,    6, 0, AlignLeft);
    gridLayout->addWidget(m_velocityCheckBox, 6, 1);
    gridLayout->addWidget(m_velocityValue,    6, 2, AlignRight);

    // Populate channel list
    for (int i = 0; i < 16; i++)
        m_channelValue->insertItem(QString("%1").arg(i));

    for (int i = -Rosegarden::MidiMidValue;
             i < Rosegarden::MidiMidValue + 1; i++)
    {
        if (i > 0)
            m_panValue->insertItem(QString("+%1").arg(i));
        else
            m_panValue->insertItem(QString("%1").arg(i));
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
    RG_DEBUG << "useInstrument() - populate Instrument\n";

    if (instrument == 0)
        return;

    // ok
    m_selectedInstrument = instrument;

    // Hide or Show according to Instrumen type
    //
    if (instrument->getType() == Rosegarden::Instrument::Audio)
    {
        m_instrumentLabel->show();
        m_instrumentLabel->setText(strtoqstr(m_selectedInstrument->getName()));

        m_channelLabel->hide();
        m_panLabel->hide();
        m_velocityLabel->hide();
        m_programLabel->hide();
        m_bankLabel->hide();
        m_bankValue->hide();
        m_channelValue->hide();
        m_programValue->hide();
        m_panValue->hide();
        m_bankCheckBox->hide();
        m_programCheckBox->hide();
        m_panCheckBox->hide();
        m_velocityCheckBox->hide();

        m_velocityValue->show();
        m_velocityValue->setDisabled(false);
        m_velocityValue->setCurrentItem(instrument->getVelocity());

        return; // for the moment
    }
    else // Midi
    {
        m_instrumentLabel->show();
        m_channelLabel->show();
        m_panLabel->show();
        m_velocityLabel->show();
        m_programLabel->show();
        m_bankLabel->show();
        m_bankValue->show();
        m_channelValue->show();
        m_programValue->show();
        m_panValue->show();
        m_velocityValue->show();
        m_bankCheckBox->show();
        m_programCheckBox->show();
        m_panCheckBox->show();
        m_velocityCheckBox->show();
    }

    // Set instrument name
    //
    m_instrumentLabel->setText(strtoqstr(m_selectedInstrument->getName()));

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
        m_velocityValue->setCurrentItem(instrument->getVelocity());
    }
    else
    {
        m_velocityValue->setDisabled(true);
        m_velocityValue->setCurrentItem(-1);
    }

    // clear bank list
    m_bankValue->clear();

    // create bank list
    Rosegarden::StringList list = 
        dynamic_cast<Rosegarden::MidiDevice*>
            (instrument->getDevice())->getBankList();

    Rosegarden::StringList::iterator it;

    for (it = list.begin(); it != list.end(); it++)
        m_bankValue->insertItem(strtoqstr(*it));

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
        updateAllBoxes();
        return;
    }

    m_selectedInstrument->setSendProgramChange(value);
    m_programValue->setDisabled(!value);
    populateProgramList();

    try
    {
        Rosegarden::MappedEvent *mE = 
         new Rosegarden::MappedEvent(m_selectedInstrument->getId(),
                                     Rosegarden::MappedEvent::MidiProgramChange,
                                     m_selectedInstrument->getProgramChange(),
                                     (Rosegarden::MidiByte)0);
        // Send the controller change
        //
        emit sendMappedEvent(mE);
        updateAllBoxes();
    }
    catch(...) {;}

    emit changeInstrumentLabel(m_selectedInstrument->getId(),
			       strtoqstr(m_selectedInstrument->
					 getProgramName()));
    updateAllBoxes();
}

void
InstrumentParameterBox::slotActivateVelocity(bool value)
{
    if (m_selectedInstrument == 0)
    {
        m_velocityCheckBox->setChecked(false);
        updateAllBoxes();
        return;
    }

    m_selectedInstrument->setSendVelocity(value);
    m_velocityValue->setDisabled(!value);
    m_velocityValue->setCurrentItem(m_selectedInstrument->getVelocity());

    updateAllBoxes();
}

void
InstrumentParameterBox::slotActivatePan(bool value)
{
    if (m_selectedInstrument == 0)
    {
        m_panCheckBox->setChecked(false);
        updateAllBoxes();
        return;
    }

    m_selectedInstrument->setSendPan(value);
    m_panValue->setDisabled(!value);
    m_panValue->setCurrentItem(m_selectedInstrument->getPan());

    updateAllBoxes();
}

void
InstrumentParameterBox::slotActivateBank(bool value)
{
    if (m_selectedInstrument == 0)
    {
        m_bankCheckBox->setChecked(false);
        updateAllBoxes();
        return;
    }

    m_selectedInstrument->setSendBankSelect(value);
    m_bankValue->setDisabled(!value);

    emit changeInstrumentLabel(m_selectedInstrument->getId(),
			       strtoqstr(m_selectedInstrument->
					 getProgramName()));
    updateAllBoxes();
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

    try
    {
        Rosegarden::MappedEvent *mE = 
         new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                     Rosegarden::MappedEvent::MidiProgramChange,
                                     prg->program,
                                     (Rosegarden::MidiByte)0);
        // Send the controller change
        //
        emit sendMappedEvent(mE);
    }
    catch(...) {;}

    emit changeInstrumentLabel(m_selectedInstrument->getId(),
			       strtoqstr(m_selectedInstrument->
					 getProgramName()));
    updateAllBoxes();
}


void
InstrumentParameterBox::slotSelectPan(int index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setPan(index);

    try
    {
        Rosegarden::MappedEvent *mE = 
         new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MIDI_CONTROLLER_PAN,
                                     (Rosegarden::MidiByte)index);
        // Send the controller change
        //
        emit sendMappedEvent(mE);
    }
    catch(...) {;}

    updateAllBoxes();
}

void
InstrumentParameterBox::slotSelectVelocity(int index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setVelocity(index);

    if (m_selectedInstrument->getType() == Rosegarden::Instrument::Audio)
    {
        emit setMappedProperty(m_selectedInstrument->getId(),
                               QString("value"),
                               index);
    }

    updateAllBoxes();
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

    try
    {
        Rosegarden::MappedEvent *mE = 
            new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                        Rosegarden::MappedEvent::MidiController,
                                        Rosegarden::MIDI_CONTROLLER_BANK_MSB,
                                        bank->msb);
        // Send the msb
        //
        emit sendMappedEvent(mE);

        mE = new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                        Rosegarden::MappedEvent::MidiController,
                                        Rosegarden::MIDI_CONTROLLER_BANK_LSB,
                                        bank->lsb);
        // Send the lsb
        //
        emit sendMappedEvent(mE);
    }
    catch(...) {;}

    // also need to resend Program change to activate new program
    slotSelectProgram(m_selectedInstrument->getProgramChange());
    updateAllBoxes();
}

void
InstrumentParameterBox::slotSelectChannel(int index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setMidiChannel(index);

    // don't use the emit - use this method instead
    emit sendMappedInstrument(
            Rosegarden::MappedInstrument(m_selectedInstrument));
    updateAllBoxes();
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

    Rosegarden::StringList list = 
        dynamic_cast<Rosegarden::MidiDevice*>
            (m_selectedInstrument->getDevice())->getProgramList(msb, lsb);

    Rosegarden::StringList::iterator it;

    for (it = list.begin(); it != list.end(); it++)
        m_programValue->insertItem(strtoqstr(*it));

    m_programValue->setCurrentItem(
            (int)m_selectedInstrument->getProgramChange());
}

void
InstrumentParameterBox::updateAllBoxes()
{
    std::vector<InstrumentParameterBox*>::iterator it =
        instrumentParamBoxes.begin();

    for (; it != instrumentParamBoxes.end(); it++)
    {
        if ((*it) != this && m_selectedInstrument &&
            (*it)->getSelectedInstrument() == m_selectedInstrument)
            (*it)->useInstrument(m_selectedInstrument);
    }

}


