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
#include <qslider.h>
#include <qpushbutton.h>

#include "Midi.h"
#include "Instrument.h"
#include "MidiDevice.h"
#include "MappedStudio.h"

#include "audioplugindialog.h"
#include "instrumentparameterbox.h"
#include "audiopluginmanager.h"
#include "widgets.h"
#include "studiocontrol.h"
#include "rosegardenguidoc.h"

#include "rosestrings.h"
#include "rosedebug.h"


InstrumentParameterBox::InstrumentParameterBox(RosegardenGUIDoc *doc,
                                               QWidget *parent)
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
      m_volumeFader(new RosegardenFader(this)),
      m_volumeValue(new QLabel(this)),
      m_volumeLabel(new QLabel(i18n("Volume"), this)),
      m_pluginLabel(new QLabel(i18n("Plugins"), this)),
      m_chorusRotary(new RosegardenRotary(this, 0.0, 127.0, 1.0, 5.0, 0.0, 20)),
      m_reverbRotary(new RosegardenRotary(this, 0.0, 127.0, 1.0, 5.0, 0.0, 20)),
      m_highPassRotary(new RosegardenRotary(this, 0.0, 127.0, 1.0, 5.0, 0.0, 20)),
      m_resonanceRotary(new RosegardenRotary(this, 0.0, 127.0, 1.0, 5.0, 0.0, 20)),
      m_attackRotary(new RosegardenRotary(this, 0.0, 127.0, 1.0, 5.0, 0.0, 20)),
      m_releaseRotary(new RosegardenRotary(this, 0.0, 127.0, 1.0, 5.0, 0.0, 20)),
      m_chorusLabel(new QLabel(i18n("Chorus"), this)),
      m_reverbLabel(new QLabel(i18n("Reverb"), this)),
      m_highPassLabel(new QLabel(i18n("Filter"), this)),
      m_resonanceLabel(new QLabel(i18n("Resonance"), this)),
      m_attackLabel(new QLabel(i18n("Attack"), this)),
      m_releaseLabel(new QLabel(i18n("Release"), this)),
      m_selectedInstrument(0),
      m_pluginManager(doc->getPluginManager()),
      m_doc(doc)
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

    for (unsigned int i = 0; i < m_pluginButtons.size(); i++)
        delete m_pluginButtons[i];
    m_pluginButtons.erase(m_pluginButtons.begin(), m_pluginButtons.end());
}

void
InstrumentParameterBox::initBox()
{
    QGridLayout *gridLayout = new QGridLayout(this, 12, 3, 8, 1);

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

    m_volumeFader->setLineStep(1);
    //m_volumeFader->setPageStep(1);
    m_volumeFader->setMaxValue(127);
    m_volumeFader->setMinValue(0);
    m_volumeValue->setFont(getFont());
    m_volumeLabel->setFont(getFont());
    m_pluginLabel->setFont(getFont());

    // advanced MIDI effects
    //
    m_chorusLabel->setFont(getFont());
    m_reverbLabel->setFont(getFont());
    m_highPassLabel->setFont(getFont());
    m_resonanceLabel->setFont(getFont());
    m_attackLabel->setFont(getFont());
    m_releaseLabel->setFont(getFont());

    unsigned int defaultPlugins = 5;
    for (unsigned int i = 0; i < defaultPlugins; i++)
    {
        PluginButton *pb = new PluginButton(this, i);
        pb->setFont(getFont());
        pb->setText(i18n("<no plugin>"));
        m_pluginButtons.push_back(pb);
    }

    // Some top space
    gridLayout->addRowSpacing(0, 8);
    gridLayout->addRowSpacing(1, 30);

    // MIDI widgets
    //
    gridLayout->addMultiCellWidget(m_instrumentLabel, 1, 1, 0, 2, AlignCenter);
    gridLayout->addWidget(m_bankLabel,    2, 0, AlignLeft);
    gridLayout->addWidget(m_bankCheckBox, 2, 1);
    gridLayout->addWidget(m_bankValue,    2, 2, AlignRight);

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

    gridLayout->addWidget(m_chorusLabel, 7, 0, AlignLeft);
    gridLayout->addWidget(m_chorusRotary, 7, 2, AlignRight);

    gridLayout->addWidget(m_reverbLabel, 8, 0, AlignLeft);
    gridLayout->addWidget(m_reverbRotary, 8, 2, AlignRight);

    gridLayout->addWidget(m_highPassLabel, 9, 0, AlignLeft);
    gridLayout->addWidget(m_highPassRotary, 9, 2, AlignRight);

    gridLayout->addWidget(m_resonanceLabel, 10, 0, AlignLeft);
    gridLayout->addWidget(m_resonanceRotary, 10, 2, AlignRight);

    gridLayout->addWidget(m_attackLabel, 11, 0, AlignLeft);
    gridLayout->addWidget(m_attackRotary, 11, 2, AlignRight);

    gridLayout->addWidget(m_releaseLabel, 12, 0, AlignLeft);
    gridLayout->addWidget(m_releaseRotary, 12, 2, AlignRight);

    // Audio widgets
    //
    gridLayout->addWidget(m_volumeLabel, 13, 0, AlignCenter);
    gridLayout->addMultiCellWidget(m_pluginLabel, 13, 13, 1, 2, AlignCenter);
    gridLayout->addMultiCellWidget(m_volumeFader, 14, 16, 0, 0,  AlignCenter);
    gridLayout->addWidget(m_volumeValue, 17, 0, AlignCenter);

    for (unsigned int i = 0; i < m_pluginButtons.size(); i++)
    {
        gridLayout->addRowSpacing(12 + 1 + i, m_pluginButtons[i]->height());
        gridLayout->addMultiCellWidget(m_pluginButtons[i],
                                       12 + i, 12 + i, 1, 2, AlignCenter);
        connect(m_pluginButtons[i], SIGNAL(released(int)),
                this, SLOT(slotSelectPlugin(int)));

    }

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

    connect(m_volumeFader, SIGNAL(faderChanged(int)),
            this, SLOT(slotSelectVelocity(int)));

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

    // connect the advanced MIDI controls
    connect(m_chorusRotary, SIGNAL(valueChanged(float)),
            this, SLOT(slotSelectChorus(float)));

    connect(m_reverbRotary, SIGNAL(valueChanged(float)),
            this, SLOT(slotSelectReverb(float)));

    connect(m_highPassRotary, SIGNAL(valueChanged(float)),
            this, SLOT(slotSelectHighPass(float)));

    connect(m_resonanceRotary, SIGNAL(valueChanged(float)),
            this, SLOT(slotSelectResonance(float)));

    connect(m_attackRotary, SIGNAL(valueChanged(float)),
            this, SLOT(slotSelectAttack(float)));

    connect(m_releaseRotary, SIGNAL(valueChanged(float)),
            this, SLOT(slotSelectRelease(float)));


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
    {
        m_instrumentLabel->setText(i18n("<no instrument>"));
        m_instrumentLabel->setFixedWidth(140);
        m_channelLabel->hide();
        m_panLabel->hide();
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
        m_velocityValue->hide();
        m_velocityLabel->hide();

        // Advanced MIDI controls
        //
        m_chorusRotary->hide();
        m_reverbRotary->hide();
        m_highPassRotary->hide();
        m_resonanceRotary->hide();
        m_attackRotary->hide();
        m_releaseRotary->hide();
        m_chorusLabel->hide();
        m_reverbLabel->hide();
        m_highPassLabel->hide();
        m_resonanceLabel->hide();
        m_attackLabel->hide();
        m_releaseLabel->hide();

        // Audio controls
        //
        m_volumeFader->hide();
        m_volumeValue->hide();
        m_volumeLabel->hide();
        m_pluginLabel->hide();

        for (unsigned int i = 0; i < m_pluginButtons.size(); i++)
            m_pluginButtons[i]->hide();

        return;
    }

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
        m_velocityValue->hide();
        m_velocityLabel->hide();

        // Advanced MIDI controls
        //
        m_chorusRotary->hide();
        m_reverbRotary->hide();
        m_highPassRotary->hide();
        m_resonanceRotary->hide();
        m_attackRotary->hide();
        m_releaseRotary->hide();
        m_chorusLabel->hide();
        m_reverbLabel->hide();
        m_highPassLabel->hide();
        m_resonanceLabel->hide();
        m_attackLabel->hide();
        m_releaseLabel->hide();

        // Audio controls
        //

        //m_velocityValue->setDisabled(false);
        //m_velocityValue->setCurrentItem(instrument->getVelocity());

        m_volumeFader->show();
        m_volumeValue->show();
        m_volumeLabel->show();
        m_pluginLabel->show();
        m_volumeFader->setFader(instrument->getVelocity());

        for (unsigned int i = 0; i < m_pluginButtons.size(); i++)
        {
            m_pluginButtons[i]->show();

            Rosegarden::AudioPluginInstance *inst = 
                m_selectedInstrument->getPlugin(i);

            if (inst && inst->isAssigned())
            {
                Rosegarden::AudioPlugin *pluginClass 
                    = m_pluginManager->getPlugin(
                            m_pluginManager->
                                getPositionByUniqueId(inst->getId()));
    
                if (pluginClass)
                    m_pluginButtons[i]->setText(pluginClass->getLabel());
            }
            else
                m_pluginButtons[i]->setText(i18n("<no plugin>"));
        }

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

        // Advanced MIDI controls
        //
        m_chorusRotary->show();
        m_reverbRotary->show();
        m_highPassRotary->show();
        m_resonanceRotary->show();
        m_attackRotary->show();
        m_releaseRotary->show();
        m_chorusLabel->show();
        m_reverbLabel->show();
        m_highPassLabel->show();
        m_resonanceLabel->show();
        m_attackLabel->show();
        m_releaseLabel->show();


        // Audio controls
        //
        m_volumeFader->hide();
        m_volumeValue->hide();
        m_volumeLabel->hide();
        m_pluginLabel->hide();

        for (unsigned int i = 0; i < m_pluginButtons.size(); i++)
            m_pluginButtons[i]->hide();

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

    // Advanced MIDI controllers
    //
    m_chorusRotary->setPosition(float(instrument->getChorus()));
    m_reverbRotary->setPosition(float(instrument->getReverb()));
    m_highPassRotary->setPosition(float(instrument->getFilter()));
    m_resonanceRotary->setPosition(float(instrument->getResonance()));
    m_attackRotary->setPosition(float(instrument->getAttack()));
    m_releaseRotary->setPosition(float(instrument->getRelease()));

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
        Rosegarden::StudioControl::sendMappedEvent(mE);
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
        Rosegarden::StudioControl::sendMappedEvent(mE);
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
        Rosegarden::StudioControl::sendMappedEvent(mE);
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
        // stupid QSliders mean we have to invert this value so that
        // the top of the slider is max, the bottom min.
        //
        m_volumeValue->setNum(index);

        Rosegarden::StudioControl::setStudioObjectProperty
            (Rosegarden::MappedObjectId(m_selectedInstrument->getMappedId()),
             Rosegarden::MappedAudioFader::FaderLevel,
             Rosegarden::MappedObjectValue(index));
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
        Rosegarden::StudioControl::sendMappedEvent(mE);

        mE = new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                        Rosegarden::MappedEvent::MidiController,
                                        Rosegarden::MIDI_CONTROLLER_BANK_LSB,
                                        bank->lsb);
        // Send the lsb
        //
        Rosegarden::StudioControl::sendMappedEvent(mE);
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
    Rosegarden::StudioControl::sendMappedInstrument(
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


void
InstrumentParameterBox::slotSelectPlugin(int index)
{
    // only create a dialog if we've got a plugin instance
    Rosegarden::AudioPluginInstance *inst = 
        m_selectedInstrument->getPlugin(index);

    if (inst)
    {
        Rosegarden::AudioPluginDialog *aPD = 
            new Rosegarden::AudioPluginDialog(this,
                                              m_pluginManager,
                                              m_selectedInstrument,
                                              index);

        connect(aPD, SIGNAL(pluginSelected(int, int)),
                this, SLOT(slotPluginSelected(int, int)));

        connect(aPD, SIGNAL(pluginPortChanged(int, int, float)),
                this, SLOT(slotPluginPortChanged(int, int, float)));

        aPD->show();
    }
    else
    {
        cout << "NO AudioPluginInstance found for index " << index << endl;
    }
}

void
InstrumentParameterBox::slotPluginSelected(int index, int plugin)
{

    Rosegarden::AudioPluginInstance *inst = 
        m_selectedInstrument->getPlugin(index);

    if (inst)
    {
        if (plugin == -1)
        {
            std:: cout << "InstrumentParameterBox::slotPluginSelected - "
                       << "no plugin selected" << std::endl;

            // Destroy plugin instance
            if (Rosegarden::StudioControl::
                    destroyStudioObject(inst->getMappedId()))
            {
                std::cout << "InstrumentParameterBox::slotPluginSelected - "
                          << "cannot destroy Studio object "
                          << inst->getMappedId() << std::endl;
            }

            inst->setAssigned(false);
            m_pluginButtons[index]->setText(i18n("<no plugin>"));
        }
        else
        {
            Rosegarden::AudioPlugin *plgn = 
                m_pluginManager->getPlugin(plugin);

            // If unassigned then create a sequencer instance of this
            // AudioPluginInstance.
            //
            if (inst->isAssigned())
            {
                // unassign, destory and recreate
                //cout << "MODIFY assigned " << inst->getMappedId() << endl;
                std::cout << "InstrumentParameterBox::slotPluginSelected - "
                          << "MappedObjectId = "
                          << inst->getMappedId()
                          << " - UniqueId = " << plgn->getUniqueId()
                          << std::endl;


#ifdef HAVE_LADSPA
                Rosegarden::StudioControl::setStudioObjectProperty
                    (inst->getMappedId(),
                     Rosegarden::MappedLADSPAPlugin::UniqueId,
                     plgn->getUniqueId());
#endif

            }
            else
            {
                // create a studio object at the sequencer
                Rosegarden::MappedObjectId newId =
                    Rosegarden::StudioControl::createStudioObject
                        (Rosegarden::MappedObject::LADSPAPlugin);

                std::cout << "InstrumentParameterBox::slotPluginSelected - "
                             " new MappedObjectId = " << newId << std::endl;

                // set the new Mapped ID and that this instance
                // is assigned
                inst->setMappedId(newId);
                inst->setAssigned(true);

#ifdef HAVE_LADSPA
                // set the instrument id
                Rosegarden::StudioControl::setStudioObjectProperty
                    (newId,
                     Rosegarden::MappedObject::Instrument,
                     Rosegarden::MappedObjectValue(
                         m_selectedInstrument->getId()));

                // set the position
                Rosegarden::StudioControl::setStudioObjectProperty
                    (newId,
                     Rosegarden::MappedObject::Position,
                     Rosegarden::MappedObjectValue(index));

                // set the plugin id
                Rosegarden::StudioControl::setStudioObjectProperty
                    (newId,
                     Rosegarden::MappedLADSPAPlugin::UniqueId,
                     Rosegarden::MappedObjectValue(
                         plgn->getUniqueId()));
#endif
            }

            Rosegarden::AudioPlugin *pluginClass 
                = m_pluginManager->getPlugin(plugin);

            if (pluginClass)
                m_pluginButtons[index]->setText(pluginClass->getLabel());
        }
    }
    else
        std::cerr << "InstrumentParameterBox::slotPluginSelected - "
                  << "got index of unknown plugin!" << std::endl;
}

void
InstrumentParameterBox::slotPluginPortChanged(int pluginIndex,
                                              int portIndex,
                                              float value)
{
    Rosegarden::AudioPluginInstance *inst = 
        m_selectedInstrument->getPlugin(pluginIndex);

    if (inst)
    {

#ifdef HAVE_LADSPA

        Rosegarden::StudioControl::
            setStudioPluginPort(inst->getMappedId(),
                                portIndex,
                                value);
                                
        /*
        std::cout << "InstrumentParameterBox::slotPluginPortChanged - "
                  << "setting plugin port to " << value << std::endl;
                  */
#endif
    }

}

void
InstrumentParameterBox::slotSelectChorus(float index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setChorus(Rosegarden::MidiByte(index));

    try
    {
        Rosegarden::MappedEvent *mE = 
         new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MIDI_CONTROLLER_CHORUS,
                                     (Rosegarden::MidiByte)index);
        Rosegarden::StudioControl::sendMappedEvent(mE);
    }
    catch(...) {;}

    updateAllBoxes();
}

void
InstrumentParameterBox::slotSelectReverb(float index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setReverb(Rosegarden::MidiByte(index));

    try
    {
        Rosegarden::MappedEvent *mE = 
         new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MIDI_CONTROLLER_REVERB,
                                     (Rosegarden::MidiByte)index);
        Rosegarden::StudioControl::sendMappedEvent(mE);
    }
    catch(...) {;}

    updateAllBoxes();
}


void
InstrumentParameterBox::slotSelectHighPass(float index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setFilter(Rosegarden::MidiByte(index));

    try
    {
        Rosegarden::MappedEvent *mE = 
         new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MIDI_CONTROLLER_FILTER,
                                     (Rosegarden::MidiByte)index);
        Rosegarden::StudioControl::sendMappedEvent(mE);
    }
    catch(...) {;}

    updateAllBoxes();
}

void
InstrumentParameterBox::slotSelectResonance(float index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setResonance(Rosegarden::MidiByte(index));

    try
    {
        Rosegarden::MappedEvent *mE = 
         new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MIDI_CONTROLLER_RESONANCE,
                                     (Rosegarden::MidiByte)index);
        Rosegarden::StudioControl::sendMappedEvent(mE);

        // Send the controller change
        //
        /*

        Rosegarden::MappedEvent *mE = 
         new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MidiByte(99),
                                     (Rosegarden::MidiByte)0);
        // Send the controller change
        //
        emit Rosegarden::StudioControl::sendMappedEvent(mE);
        mE = 
         new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MidiByte(98),
                                     (Rosegarden::MidiByte)33);
        // Send the controller change
        //
        emit Rosegarden::StudioControl::sendMappedEvent(mE);
        mE = 
         new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MidiByte(6),
                                     (Rosegarden::MidiByte)index);
        // Send the controller change
        //
        emit Rosegarden::StudioControl::sendMappedEvent(mE);
        */
    }
    catch(...) {;}

    updateAllBoxes();
}


void
InstrumentParameterBox::slotSelectAttack(float index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setAttack(Rosegarden::MidiByte(index));

    try
    {
        Rosegarden::MappedEvent *mE = 
         new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MIDI_CONTROLLER_ATTACK,
                                     (Rosegarden::MidiByte)index);
        Rosegarden::StudioControl::sendMappedEvent(mE);
    }
    catch(...) {;}

    updateAllBoxes();
}

void
InstrumentParameterBox::slotSelectRelease(float index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setRelease(Rosegarden::MidiByte(index));

    try
    {
        Rosegarden::MappedEvent *mE = 
         new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MIDI_CONTROLLER_RELEASE,
                                     (Rosegarden::MidiByte)index);
        Rosegarden::StudioControl::sendMappedEvent(mE);
    }
    catch(...) {;}

    updateAllBoxes();
}




