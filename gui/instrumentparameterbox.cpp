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
#include <qwidgetstack.h>

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
    : RosegardenParameterBox(1, Qt::Horizontal, i18n("Instrument Parameters"), parent),
      m_widgetStack(new QWidgetStack(this)),
      m_noInstrumentParameters(new QVBox(m_widgetStack)),
      m_midiInstrumentParameters(new MIDIInstrumentParameterPanel(m_widgetStack)),
      m_audioInstrumentParameters(new AudioInstrumentParameterPanel(doc, m_widgetStack)),
      m_selectedInstrument(0),
      m_doc(doc)
{
    m_widgetStack->setFont(getFont());

    bool contains = false;

    std::vector<InstrumentParameterBox*>::iterator it =
        instrumentParamBoxes.begin();

    for (; it != instrumentParamBoxes.end(); it++)
        if ((*it) == this)
            contains = true;

    if (!contains)
        instrumentParamBoxes.push_back(this);

    QLabel *label = new QLabel(i18n("<no instrument>"), m_noInstrumentParameters);
    label->setAlignment(label->alignment() | Qt::AlignHCenter);
    
    m_widgetStack->addWidget(m_midiInstrumentParameters);
    m_widgetStack->addWidget(m_audioInstrumentParameters);
    m_widgetStack->addWidget(m_noInstrumentParameters);

    m_midiInstrumentParameters->adjustSize();
    m_audioInstrumentParameters->adjustSize();
    m_noInstrumentParameters->adjustSize();

    connect(m_audioInstrumentParameters, SIGNAL(updateAllBoxes()),
            this, SLOT(slotUpdateAllBoxes()));
    
    connect(m_midiInstrumentParameters, SIGNAL(updateAllBoxes()),
            this, SLOT(slotUpdateAllBoxes()));

    connect(m_midiInstrumentParameters, SIGNAL(changeInstrumentLabel(Rosegarden::InstrumentId, QString)),
            this, SIGNAL(changeInstrumentLabel(Rosegarden::InstrumentId, QString)));
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
InstrumentParameterBox::useInstrument(Rosegarden::Instrument *instrument)
{
    RG_DEBUG << "useInstrument() - populate Instrument\n";

    if (instrument == 0)
    {
        m_widgetStack->raiseWidget(m_noInstrumentParameters);

        return;
    } 

    // ok
    m_selectedInstrument = instrument;

    // Hide or Show according to Instrumen type
    //
    if (instrument->getType() == Rosegarden::Instrument::Audio)
    {
        m_audioInstrumentParameters->setupForInstrument(m_selectedInstrument);
        m_widgetStack->raiseWidget(m_audioInstrumentParameters);

    } else { // Midi

        m_midiInstrumentParameters->setupForInstrument(m_selectedInstrument);
        m_widgetStack->raiseWidget(m_midiInstrumentParameters);
    }
    
}

void
MIDIInstrumentParameterPanel::slotActivateProgramChange(bool value)
{
    if (m_selectedInstrument == 0)
    {
        m_programCheckBox->setChecked(false);
        emit updateAllBoxes();
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
        emit updateAllBoxes();
    }
    catch(...) {;}

    emit changeInstrumentLabel(m_selectedInstrument->getId(),
			       strtoqstr(m_selectedInstrument->
					 getProgramName()));
    emit updateAllBoxes();
}

void
MIDIInstrumentParameterPanel::slotActivateBank(bool value)
{
    if (m_selectedInstrument == 0)
    {
        m_bankCheckBox->setChecked(false);
        emit updateAllBoxes();
        return;
    }

    m_selectedInstrument->setSendBankSelect(value);
    m_bankValue->setDisabled(!value);

    emit changeInstrumentLabel(m_selectedInstrument->getId(),
			       strtoqstr(m_selectedInstrument->
					 getProgramName()));
    emit updateAllBoxes();
}


void
MIDIInstrumentParameterPanel::slotSelectProgram(int index)
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
    emit updateAllBoxes();
}


void
MIDIInstrumentParameterPanel::slotSelectPan(float value)
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

    emit updateAllBoxes();
}

void
MIDIInstrumentParameterPanel::slotSelectVolume(float value)
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

    emit updateAllBoxes();
}

void
AudioInstrumentParameterPanel::slotSelectAudioLevel(int value)
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

    emit updateAllBoxes();
}

void
MIDIInstrumentParameterPanel::slotSelectBank(int index)
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
    emit updateAllBoxes();
}

void
MIDIInstrumentParameterPanel::slotSelectChannel(int index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setMidiChannel(index);

    // don't use the emit - use this method instead
    Rosegarden::StudioControl::sendMappedInstrument(
            Rosegarden::MappedInstrument(m_selectedInstrument));
    emit updateAllBoxes();
}





// Populate program list by bank context
//
void
MIDIInstrumentParameterPanel::populateProgramList()
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
InstrumentParameterBox::slotUpdateAllBoxes()
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
AudioInstrumentParameterPanel::slotSelectPlugin(int index)
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
AudioInstrumentParameterPanel::slotPluginSelected(int index, int plugin)
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
AudioInstrumentParameterPanel::slotPluginPortChanged(int pluginIndex,
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
                                
        std::cout << "InstrumentParameterBox::slotPluginPortChanged - "
                  << "setting plugin port to " << value << std::endl;
#endif // HAVE_LADSPA
    }

}

void
AudioInstrumentParameterPanel::slotBypassed(int pluginIndex, bool bp)
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
MIDIInstrumentParameterPanel::slotSelectChorus(float index)
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
MIDIInstrumentParameterPanel::slotSelectReverb(float index)
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
MIDIInstrumentParameterPanel::slotSelectHighPass(float index)
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
MIDIInstrumentParameterPanel::slotSelectResonance(float index)
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
MIDIInstrumentParameterPanel::slotSelectAttack(float index)
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
MIDIInstrumentParameterPanel::slotSelectRelease(float index)
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


InstrumentParameterPanel::InstrumentParameterPanel(QWidget* parent)
    : QFrame(parent),
      m_instrumentLabel(new QLabel(this)),
      m_selectedInstrument(0)
{
}



AudioInstrumentParameterPanel::AudioInstrumentParameterPanel(RosegardenGUIDoc* doc, QWidget* parent)
    : InstrumentParameterPanel(parent),
      m_audioLevelFader(new RosegardenFader(this)),
      m_audioLevelValue(new QLabel(this)),
      m_signalMapper(new QSignalMapper(this)),
      m_pluginManager(doc->getPluginManager())
{
    QGridLayout *gridLayout = new QGridLayout(this, 7, 2);
    // Some top space
//     gridLayout->addRowSpacing(0, 8);
//     gridLayout->addRowSpacing(1, 30);

    QLabel* audioLevelLabel = new QLabel(i18n("Level"), this);
    QLabel* pluginLabel = new QLabel(i18n("Plugins"), this);

    m_audioLevelFader->setLineStep(1);

    //m_volumeFader->setPageStep(1);
    m_audioLevelFader->setMaxValue(127);
    m_audioLevelFader->setMinValue(0);
    
    unsigned int defaultPlugins = 5;
    for (unsigned int i = 0; i < defaultPlugins; i++)
    {
        QPushButton *pb = new QPushButton(this);
        pb->setText(i18n("<no plugin>"));
        m_pluginButtons.push_back(pb);
    }

    // Instrument label : first row, both cols
    gridLayout->addMultiCellWidget(m_instrumentLabel, 0, 0, 0, 1, AlignCenter);

    // Fader column (col. 0)
    gridLayout->addWidget(audioLevelLabel, 1, 0, AlignCenter);
    gridLayout->addMultiCellWidget(m_audioLevelFader, 2, 5, 0, 0, AlignCenter);
    gridLayout->addWidget(m_audioLevelValue, 6, 0, AlignCenter);

    // Plugins column (col. 1)
    gridLayout->addWidget(pluginLabel, 1, 1, AlignCenter);

    for (unsigned int i = 0; i < m_pluginButtons.size(); i++)
    {
        gridLayout->addWidget(m_pluginButtons[i],
                              2 + i, 1, AlignCenter);
        m_signalMapper->setMapping(m_pluginButtons[i], i);

        connect(m_pluginButtons[i], SIGNAL(clicked()),
                m_signalMapper, SLOT(map()));

    }

    connect(m_signalMapper, SIGNAL(mapped(int)),
            this, SLOT(slotSelectPlugin(int)));


    connect(m_audioLevelFader, SIGNAL(faderChanged(int)),
            this, SLOT(slotSelectAudioLevel(int)));

}

void
AudioInstrumentParameterPanel::setupForInstrument(Rosegarden::Instrument* instrument)
{
    m_selectedInstrument = instrument;

    m_instrumentLabel->setText(strtoqstr(instrument->getName()));
    m_audioLevelFader->setFader(instrument->getVelocity());

        for (unsigned int i = 0; i < m_pluginButtons.size(); i++)
        {
            m_pluginButtons[i]->show();

            Rosegarden::AudioPluginInstance *inst = 
                instrument->getPlugin(i);

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

}



MIDIInstrumentParameterPanel::MIDIInstrumentParameterPanel(QWidget* parent)
    : InstrumentParameterPanel(parent),
      m_bankValue(new RosegardenComboBox(false, false, this)),
      m_channelValue(new RosegardenComboBox(true, false, this)),
      m_programValue(new RosegardenComboBox(false, false, this)),
      m_panRotary(new RosegardenRotary(this, 0.0, 127.0, 1.0, 5.0, 64.0, 20)),
      m_volumeRotary(new RosegardenRotary(this, 0.0, 127.0, 1.0, 5.0, 64.0, 20)),
      m_bankCheckBox(new QCheckBox(this)),
      m_programCheckBox(new QCheckBox(this)),
      m_chorusRotary(new RosegardenRotary(this, 0.0, 127.0, 1.0, 5.0, 0.0, 20)),
      m_reverbRotary(new RosegardenRotary(this, 0.0, 127.0, 1.0, 5.0, 0.0, 20)),
      m_highPassRotary(new RosegardenRotary(this, 0.0, 127.0, 1.0, 5.0, 0.0, 20)),
      m_resonanceRotary(new RosegardenRotary(this, 0.0, 127.0, 1.0, 5.0, 0.0, 20)),
      m_attackRotary(new RosegardenRotary(this, 0.0, 127.0, 1.0, 5.0, 0.0, 20)),
      m_releaseRotary(new RosegardenRotary(this, 0.0, 127.0, 1.0, 5.0, 0.0, 20))
{
    

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


    QLabel* channelLabel = new QLabel(i18n("Channel"), this);
    QLabel* panLabel = new QLabel(i18n("Pan"), this);
    QLabel* volumeLabel= new QLabel(i18n("Volume"), this);
    QLabel* programLabel = new QLabel(i18n("Program"), this);
    QLabel* bankLabel = new QLabel(i18n("Bank"), this);

    QLabel* chorusLabel = new QLabel(i18n("Chorus"), this);
    QLabel* reverbLabel = new QLabel(i18n("Reverb"), this);
    QLabel* highPassLabel = new QLabel(i18n("Filter"), this);
    QLabel* resonanceLabel = new QLabel(i18n("Resonance"), this);
    QLabel* attackLabel = new QLabel(i18n("Attack"), this);
    QLabel* releaseLabel = new QLabel(i18n("Release"), this);

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

    QGridLayout *gridLayout = new QGridLayout(this, 12, 3, 8, 1);

    // Some top space
    gridLayout->addRowSpacing(0, 8);
    gridLayout->addRowSpacing(1, 30);

    // MIDI widgets
    //
    gridLayout->addMultiCellWidget(m_instrumentLabel, 0, 0, 0, 2, AlignCenter);
    gridLayout->addWidget(bankLabel,      2, 0, AlignLeft);
    gridLayout->addWidget(m_bankCheckBox, 2, 1);
    gridLayout->addWidget(m_bankValue,    2, 2, AlignRight);

    gridLayout->addWidget(programLabel,      3, 0);
    gridLayout->addWidget(m_programCheckBox, 3, 1);
    gridLayout->addWidget(m_programValue,    3, 2, AlignRight);

    gridLayout->addMultiCellWidget(channelLabel, 4, 4, 0, 1, AlignLeft);
    gridLayout->addWidget(m_channelValue, 4, 2, AlignRight);

    gridLayout->addWidget(volumeLabel,      5, 0, AlignLeft);
    gridLayout->addWidget(m_volumeRotary,   5, 2, AlignRight);

    gridLayout->addWidget(panLabel,      6, 0, AlignLeft);
    gridLayout->addWidget(m_panRotary,   6, 2, AlignRight);

    gridLayout->addWidget(chorusLabel,    7, 0, AlignLeft);
    gridLayout->addWidget(m_chorusRotary, 7, 2, AlignRight);

    gridLayout->addWidget(reverbLabel,    8, 0, AlignLeft);
    gridLayout->addWidget(m_reverbRotary, 8, 2, AlignRight);

    gridLayout->addWidget(highPassLabel,    9, 0, AlignLeft);
    gridLayout->addWidget(m_highPassRotary, 9, 2, AlignRight);

    gridLayout->addWidget(resonanceLabel,    10, 0, AlignLeft);
    gridLayout->addWidget(m_resonanceRotary, 10, 2, AlignRight);

    gridLayout->addWidget(attackLabel,    11, 0, AlignLeft);
    gridLayout->addWidget(m_attackRotary, 11, 2, AlignRight);

    gridLayout->addWidget(releaseLabel,    12, 0, AlignLeft);
    gridLayout->addWidget(m_releaseRotary, 12, 2, AlignRight);

    // Populate channel list
    for (int i = 0; i < 16; i++)
        m_channelValue->insertItem(QString("%1").arg(i));

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
MIDIInstrumentParameterPanel::setupForInstrument(Rosegarden::Instrument *instrument)
{
    m_selectedInstrument = instrument;

    // Set instrument name
    //
    m_instrumentLabel->setText(strtoqstr(instrument->getName()));

    // Enable all check boxes
    //
    m_programCheckBox->setDisabled(false);
    m_bankCheckBox->setDisabled(false);

    // Activate all checkboxes
    //
    m_programCheckBox->setChecked(instrument->sendsProgramChange());
    m_bankCheckBox->setChecked(instrument->sendsBankSelect());

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
