// -*- c-basic-offset: 4 -*-

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

#include <iostream>
#include <cstdio>

#include <klocale.h>


#include <qdial.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qslider.h>
#include <qpushbutton.h>
#include <qsignalmapper.h>

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
      m_volumeLabel(new QLabel(i18n("Volume"), this)),
      m_programLabel(new QLabel(i18n("Program"), this)),
      m_bankLabel(new QLabel(i18n("Bank"), this)),
      m_bankValue(new RosegardenComboBox(false, false, this)),
      m_channelValue(new RosegardenComboBox(true, false, this)),
      m_programValue(new RosegardenComboBox(false, false, this)),
      m_panRotary(new RosegardenRotary(this, 0.0, 127.0, 1.0, 5.0, 64.0, 20)),
      m_volumeRotary(new RosegardenRotary(this, 0.0, 127.0, 1.0, 5.0, 64.0, 20)),
      m_bankCheckBox(new QCheckBox(this)),
      m_programCheckBox(new QCheckBox(this)),
      m_audioLevelFader(new RosegardenFader(this)),
      m_audioLevelValue(new QLabel(this)),
      m_audioLevelLabel(new QLabel(i18n("Level"), this)),
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
      m_signalMapper(new QSignalMapper(this)),
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

    // Set some nice pastel knob colours
    //

    // light blue
    m_volumeRotary->setKnobColour(QColor(205, 212, 255));

    // light red
    m_panRotary->setKnobColour(QColor(255, 168, 169));

    // light green
    m_chorusRotary->setKnobColour(QColor(231, 255, 223));
    m_reverbRotary->setKnobColour(QColor(231, 255, 223));

    // light orange
    m_highPassRotary->setKnobColour(QColor(255, 233, 208));
    m_resonanceRotary->setKnobColour(QColor(255, 233, 208));

    // light yellow
    m_attackRotary->setKnobColour(QColor(249, 255, 208));
    m_releaseRotary->setKnobColour(QColor(249, 255, 208));

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
    m_volumeLabel->setFont(getFont());
    m_programLabel->setFont(getFont());
    m_bankLabel->setFont(getFont());

    m_bankValue->setFont(getFont());
    m_programValue->setFont(getFont());
    m_channelValue->setFont(getFont());
    m_audioLevelValue->setFont(getFont());
    m_audioLevelFader->setLineStep(1);

    //m_volumeFader->setPageStep(1);
    m_audioLevelFader->setMaxValue(127);
    m_audioLevelFader->setMinValue(0);
    m_audioLevelValue->setFont(getFont());
    m_audioLevelLabel->setFont(getFont());
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
        QPushButton *pb = new QPushButton(this);
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

    gridLayout->addWidget(m_volumeLabel,    5, 0, AlignLeft);
    gridLayout->addWidget(m_volumeRotary,   5, 2, AlignRight);

    gridLayout->addWidget(m_panLabel,    6, 0, AlignLeft);
    gridLayout->addWidget(m_panRotary,   6, 2, AlignRight);

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
    gridLayout->addWidget(m_audioLevelLabel, 13, 0, AlignCenter);
    gridLayout->addMultiCellWidget(m_pluginLabel, 13, 13, 1, 2, AlignCenter);
    gridLayout->addMultiCellWidget(m_audioLevelFader, 14, 16, 0, 0,  AlignCenter);
    gridLayout->addWidget(m_audioLevelValue, 17, 0, AlignCenter);

    for (unsigned int i = 0; i < m_pluginButtons.size(); i++)
    {
        // Unfortunately while this fixes some of our layout problems
        // it also fixes the size of hidden rows making our parameter
        // box a bit unwieldy.
        //
        //gridLayout->addRowSpacing(12 + 1 + i, m_pluginButtons[i]->height());
        gridLayout->addMultiCellWidget(m_pluginButtons[i],
                                       12 + i, 12 + i, 1, 2, AlignCenter);
        m_signalMapper->setMapping(m_pluginButtons[i], i);

        connect(m_pluginButtons[i], SIGNAL(clicked()),
                m_signalMapper, SLOT(map()));
        

    }

    connect(m_signalMapper, SIGNAL(mapped(int)),
            this, SLOT(slotSelectPlugin(int)));


    // Populate channel list
    for (int i = 0; i < 16; i++)
        m_channelValue->insertItem(QString("%1").arg(i));

    /*
    for (int i = -Rosegarden::MidiMidValue;
             i < Rosegarden::MidiMidValue + 1; i++)
    {
        if (i > 0)
            m_panValue->insertItem(QString("+%1").arg(i));
        else
            m_panValue->insertItem(QString("%1").arg(i));
    }
    */

    /*
    // velocity values
    //
    for (int i = 0; i < Rosegarden::MidiMaxValue + 1; i++)
        m_velocityValue->insertItem(QString("%1").arg(i));
        */


    // Disable these three by default - they are activate by their
    // checkboxes
    //
    m_programValue->setDisabled(true);
    m_bankValue->setDisabled(true);

    // Only active is we have an Instrument selected
    //
    m_programCheckBox->setDisabled(true);
    m_bankCheckBox->setDisabled(true);

    // Connect up the toggle boxes
    //
    connect(m_programCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotActivateProgramChange(bool)));

    connect(m_bankCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotActivateBank(bool)));


    // Connect activations
    //
    connect(m_bankValue, SIGNAL(activated(int)),
            this, SLOT(slotSelectBank(int)));

    connect(m_panRotary, SIGNAL(valueChanged(float)),
            this, SLOT(slotSelectPan(float)));
    
    connect(m_volumeRotary, SIGNAL(valueChanged(float)),
            this, SLOT(slotSelectVolume(float)));

    connect(m_programValue, SIGNAL(activated(int)),
            this, SLOT(slotSelectProgram(int)));

    connect(m_channelValue, SIGNAL(activated(int)),
            this, SLOT(slotSelectChannel(int)));

    connect(m_audioLevelFader, SIGNAL(faderChanged(int)),
            this, SLOT(slotSelectAudioLevel(int)));

    // connect up mouse wheel movement
    //
    connect(m_bankValue, SIGNAL(propagate(int)),
            this, SLOT(slotSelectBank(int)));

    connect(m_programValue, SIGNAL(propagate(int)),
            this, SLOT(slotSelectProgram(int)));

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
        m_panRotary->hide();
        m_bankCheckBox->hide();
        m_programCheckBox->hide();
        m_volumeLabel->hide();
        m_volumeRotary->hide();

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
        m_audioLevelFader->hide();
        m_audioLevelValue->hide();
        m_audioLevelLabel->hide();
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
        m_panRotary->hide();
        m_bankCheckBox->hide();
        m_programCheckBox->hide();
        m_volumeRotary->hide();
        m_volumeLabel->hide();

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

        m_audioLevelFader->show();
        m_audioLevelValue->show();
        m_audioLevelLabel->show();
        m_pluginLabel->show();
        m_audioLevelFader->setFader(instrument->getVelocity());

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
        m_volumeLabel->show();
        m_volumeRotary->show();
        m_programLabel->show();
        m_bankLabel->show();
        m_bankValue->show();
        m_channelValue->show();
        m_programValue->show();
        m_panRotary->show();
        m_audioLevelValue->show();
        m_bankCheckBox->show();
        m_programCheckBox->show();

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
        m_audioLevelFader->hide();
        m_audioLevelValue->hide();
        m_audioLevelLabel->hide();
        m_pluginLabel->hide();

        for (unsigned int i = 0; i < m_pluginButtons.size(); i++)
            m_pluginButtons[i]->hide();

    }

    // Set instrument name
    //
    m_instrumentLabel->setText(strtoqstr(m_selectedInstrument->getName()));

    // Enable all check boxes
    //
    m_programCheckBox->setDisabled(false);
    m_bankCheckBox->setDisabled(false);

    // Activate all checkboxes
    //
    m_programCheckBox->setChecked(m_selectedInstrument->sendsProgramChange());
    m_bankCheckBox->setChecked(m_selectedInstrument->sendsBankSelect());

    // Basic parameters
    //
    m_channelValue->setCurrentItem((int)instrument->getMidiChannel());
    m_panRotary->setPosition((float)instrument->getPan());
    m_volumeRotary->setPosition((float)instrument->getVelocity());

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

    // clear bank list
    m_bankValue->clear();

    // create bank list
    Rosegarden::StringList list = 
        dynamic_cast<Rosegarden::MidiDevice*>
            (instrument->getDevice())->getBankList();

    Rosegarden::StringList::iterator it;

    for (it = list.begin(); it != list.end(); it++) {

        m_bankValue->insertItem(strtoqstr(*it));

	// Select 
	if (instrument->sendsBankSelect())
	{
	    Rosegarden::MidiBank *bank = 
		dynamic_cast<Rosegarden::MidiDevice*>(instrument->getDevice())
		->getBankByIndex(m_bankValue->count() - 1);

	    if (instrument->getMSB() == bank->msb &&
		instrument->getLSB() == bank->lsb) {
		m_bankValue->setCurrentItem(m_bankValue->count() - 1);
	    }
	}
    }

    if (!instrument->sendsBankSelect())
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
InstrumentParameterBox::slotSelectPan(float value)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setPan(Rosegarden::MidiByte(value));

    try
    {
        Rosegarden::MappedEvent *mE = 
         new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MIDI_CONTROLLER_PAN,
                                     (Rosegarden::MidiByte)value);
        Rosegarden::StudioControl::sendMappedEvent(mE);
    }
    catch(...) {;}

    updateAllBoxes();
}

void
InstrumentParameterBox::slotSelectVolume(float value)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setVelocity(Rosegarden::MidiByte(value));

    try
    {
        Rosegarden::MappedEvent *mE = 
         new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MIDI_CONTROLLER_VOLUME,
                                     (Rosegarden::MidiByte)value);
        Rosegarden::StudioControl::sendMappedEvent(mE);
    }
    catch(...) {;}

    updateAllBoxes();
}

void
InstrumentParameterBox::slotSelectAudioLevel(int value)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setVelocity(Rosegarden::MidiByte(value));

    if (m_selectedInstrument->getType() == Rosegarden::Instrument::Audio)
    {
        // stupid QSliders mean we have to invert this value so that
        // the top of the slider is max, the bottom min.
        //
        m_audioLevelValue->setNum(int(value));

        Rosegarden::StudioControl::setStudioObjectProperty
            (Rosegarden::MappedObjectId(m_selectedInstrument->getMappedId()),
             Rosegarden::MappedAudioFader::FaderLevel,
             Rosegarden::MappedObjectValue(value));
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

    for (it = list.begin(); it != list.end(); it++) {

        m_programValue->insertItem(strtoqstr(*it));

	Rosegarden::MidiProgram *program = 
	    dynamic_cast<Rosegarden::MidiDevice*>
	    (m_selectedInstrument->getDevice())->
	    getProgramByIndex(m_programValue->count() - 1);

	if (m_selectedInstrument->getProgramChange() == program->program) {
	    m_programValue->setCurrentItem(m_programValue->count() - 1);
	}
    }

//    m_programValue->setCurrentItem(
//            (int)m_selectedInstrument->getProgramChange());
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

        connect(aPD, SIGNAL(bypassed(int, bool)),
                this, SLOT(slotBypassed(int, bool)));

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
#endif // HAVE_LADSPA
    }

}

void
InstrumentParameterBox::slotBypassed(int pluginIndex, bool bp)
{
    Rosegarden::AudioPluginInstance *inst = 
        m_selectedInstrument->getPlugin(pluginIndex);

    if (inst)
    {
#ifdef HAVE_LADSPA
        Rosegarden::StudioControl::setStudioObjectProperty
            (inst->getMappedId(),
             Rosegarden::MappedLADSPAPlugin::Bypassed,
             Rosegarden::MappedObjectValue(bp));
#endif // HAVE_LADSPA
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




