// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#include <qdial.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qslider.h>
#include <qpushbutton.h>
#include <qsignalmapper.h>
#include <qwidgetstack.h>

#include <kcombobox.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <klocale.h>
#include <kapp.h>

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
#include "trackvumeter.h"
#include "rosegardengui.h"

#include "rosestrings.h"
#include "rosedebug.h"

#include "studiocontrol.h"
#include "studiowidgets.h"

InstrumentParameterBox::InstrumentParameterBox(RosegardenGUIDoc *doc,
                                               QWidget *parent)
    : RosegardenParameterBox(1, Qt::Horizontal, i18n("Instrument Parameters"), parent),
      m_widgetStack(new QWidgetStack(this)),
      m_noInstrumentParameters(new QVBox(m_widgetStack)),
      m_midiInstrumentParameters(new MIDIInstrumentParameterPanel(doc, m_widgetStack)),
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

    connect(m_audioInstrumentParameters,
            SIGNAL(muteButton(Rosegarden::InstrumentId, bool)),
            this, 
            SIGNAL(setMute(Rosegarden::InstrumentId, bool)));
    
    connect(m_audioInstrumentParameters,
            SIGNAL(soloButton(Rosegarden::InstrumentId, bool)),
            this,
            SIGNAL(setSolo(Rosegarden::InstrumentId, bool)));
    
    connect(m_audioInstrumentParameters,
            SIGNAL(recordButton(Rosegarden::InstrumentId, bool)),
            this,
            SIGNAL(setRecord(Rosegarden::InstrumentId, bool)));
    
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
InstrumentParameterBox::setAudioMeter(double ch1, double ch2)
{
    m_audioInstrumentParameters->setAudioMeter(ch1, ch2);
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
InstrumentParameterBox::setMute(bool value)
{
    if (m_selectedInstrument && 
            m_selectedInstrument->getType() == Rosegarden::Instrument::Audio)
    {
        m_audioInstrumentParameters->slotSetMute(value);
    }
}

/*
 * Set the record state of the audio instrument parameter panel
 */
void
InstrumentParameterBox::setRecord(bool value)
{
    if (m_selectedInstrument &&
            m_selectedInstrument->getType() == Rosegarden::Instrument::Audio)
    {
        m_audioInstrumentParameters->slotSetRecord(value);
    }
}

void
InstrumentParameterBox::setSolo(bool value)
{
    if (m_selectedInstrument &&
            m_selectedInstrument->getType() == Rosegarden::Instrument::Audio)
    {
        m_audioInstrumentParameters->slotSetSolo(value);
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

    Rosegarden::MappedEvent *mE = 
     new Rosegarden::MappedEvent(m_selectedInstrument->getId(),
                                 Rosegarden::MappedEvent::MidiProgramChange,
                                 m_selectedInstrument->getProgramChange(),
                                 (Rosegarden::MidiByte)0);
    // Send the controller change
    //
    Rosegarden::StudioControl::sendMappedEvent(mE);
    emit updateAllBoxes();

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

    Rosegarden::MidiProgram *prg;

    Rosegarden::MidiByte msb = 0;
    Rosegarden::MidiByte lsb = 0;

    if (m_selectedInstrument->sendsBankSelect())
    {
        msb = m_selectedInstrument->getMSB();
        lsb = m_selectedInstrument->getLSB();
        prg =
            dynamic_cast<Rosegarden::MidiDevice*>
            (m_selectedInstrument->getDevice())->getProgram(msb, lsb, index);

        // Send the bank select message before any PC message
        //
        Rosegarden::MappedEvent *mE = 
            new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                    Rosegarden::MappedEvent::MidiController,
                                    Rosegarden::MIDI_CONTROLLER_BANK_MSB,
                                    msb);
        Rosegarden::StudioControl::sendMappedEvent(mE);

        mE = new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                    Rosegarden::MappedEvent::MidiController,
                                    Rosegarden::MIDI_CONTROLLER_BANK_LSB,
                                    lsb);
        // Send the lsb
        //
        Rosegarden::StudioControl::sendMappedEvent(mE);

        RG_DEBUG << "sending bank select " << msb << " : " << lsb << endl;
    }
    else
    {
        prg = dynamic_cast<Rosegarden::MidiDevice*>
            (m_selectedInstrument->getDevice())->getProgramByIndex(index);
    }

    if (prg == 0)
    {
        RG_DEBUG << "program change not found in bank" << endl;
        return;
    }

    m_selectedInstrument->setProgramChange(prg->program);

    RG_DEBUG << "sending program change " << prg->program << endl;

    Rosegarden::MappedEvent *mE = 
     new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                 Rosegarden::MappedEvent::MidiProgramChange,
                                 prg->program,
                                 (Rosegarden::MidiByte)0);
    // Send the controller change
    //
    Rosegarden::StudioControl::sendMappedEvent(mE);

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

    // For audio instruments we pan from -100 to +100 but storage
    // within an unsigned char is 0 - 200 - so we adjust by 100
    //
    float adjValue = value;
    if (m_selectedInstrument->getType() == Rosegarden::Instrument::Audio)
        value += 100;

    m_selectedInstrument->setPan(Rosegarden::MidiByte(adjValue));

    Rosegarden::MappedEvent *mE = 
     new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                 Rosegarden::MappedEvent::MidiController,
                                 Rosegarden::MIDI_CONTROLLER_PAN,
                                 (Rosegarden::MidiByte)value);
    Rosegarden::StudioControl::sendMappedEvent(mE);

    emit updateAllBoxes();
}

void
MIDIInstrumentParameterPanel::slotSelectVolume(float value)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setVelocity(Rosegarden::MidiByte(value));

    Rosegarden::MappedEvent *mE = 
     new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                 Rosegarden::MappedEvent::MidiController,
                                 Rosegarden::MIDI_CONTROLLER_VOLUME,
                                 (Rosegarden::MidiByte)value);
    Rosegarden::StudioControl::sendMappedEvent(mE);

    emit updateAllBoxes();
}

void
AudioInstrumentParameterPanel::slotSelectAudioLevel(int value)
{
    if (m_selectedInstrument == 0)
        return;

    if (m_selectedInstrument->getType() == Rosegarden::Instrument::Audio)
    {
        // If this is the record track then we store and send the record level
        //
        if (m_audioFader->m_recordButton->isOn())
        {
            //cout << "SETTING STORED RECORD LEVEL = " << value << endl;
            m_selectedInstrument->setRecordLevel(Rosegarden::MidiByte(value));

            Rosegarden::StudioControl::setStudioObjectProperty
              (Rosegarden::MappedObjectId(m_selectedInstrument->getMappedId()),
               Rosegarden::MappedAudioFader::FaderRecordLevel,
               Rosegarden::MappedObjectValue(value));
        }
        else
        {
            //cout << "SETTING STORED LEVEL = " << value << endl;
            m_selectedInstrument->setVelocity(Rosegarden::MidiByte(value));

            Rosegarden::StudioControl::setStudioObjectProperty
              (Rosegarden::MappedObjectId(m_selectedInstrument->getMappedId()),
               Rosegarden::MappedAudioFader::FaderLevel,
               Rosegarden::MappedObjectValue(value));
        }
    }

    emit updateAllBoxes();
}

void 
AudioInstrumentParameterPanel::slotSetMute(bool value)
{
    RG_DEBUG << "AudioInstrumentParameterPanel::slotSetMute - "
             << "value = " << value << endl;
    m_audioFader->m_muteButton->setOn(value);
}

void
AudioInstrumentParameterPanel::slotSetSolo(bool value)
{
    RG_DEBUG << "AudioInstrumentParameterPanel::slotSetSolo - "
             << "value = " << value << endl;
    m_audioFader->m_soloButton->setOn(value);
}

void 
AudioInstrumentParameterPanel::slotSetRecord(bool value)
{
    RG_DEBUG << "AudioInstrumentParameterPanel::slotSetRecord - "
             << "value = " << value << endl;

    //if (m_selectedInstrument)
        //cout << "INSTRUMENT NAME = " 
               //<< m_selectedInstrument->getName() << endl;

    // Set the background colour for the button
    //
    if (value)
    {
        m_audioFader->m_recordButton->
            setPalette(QPalette(RosegardenGUIColours::ActiveRecordTrack));

        if (m_selectedInstrument &&
            (m_selectedInstrument->getType() == Rosegarden::Instrument::Audio))
        {
            // set the fader value to the record value

            disconnect(m_audioFader->m_fader, SIGNAL(faderChanged(int)),
                       this, SLOT(slotSelectAudioLevel(int)));

            m_audioFader->m_fader->
                setFader(m_selectedInstrument->getRecordLevel());
            //cout << "SETTING VISIBLE FADER RECORD LEVEL = " << 
                    //int(m_selectedInstrument->getRecordLevel()) << endl;

            connect(m_audioFader->m_fader, SIGNAL(faderChanged(int)),
                    this, SLOT(slotSelectAudioLevel(int)));

            // Set the prepend text on the audio fader
            m_audioFader->m_fader->setPrependText(i18n("Record level = "));
        }
    }
    else
    {
        m_audioFader->m_recordButton->unsetPalette();

        if (m_selectedInstrument &&
            (m_selectedInstrument->getType() == Rosegarden::Instrument::Audio))
        {
            disconnect(m_audioFader->m_fader, SIGNAL(faderChanged(int)),
                       this, SLOT(slotSelectAudioLevel(int)));

            // set the fader value to the playback value
            m_audioFader->m_fader->
                setFader(m_selectedInstrument->getVelocity());

            //cout << "SETTING VISIBLE FADER LEVEL = " << 
                    //int(m_selectedInstrument->getVelocity()) << endl;

            connect(m_audioFader->m_fader, SIGNAL(faderChanged(int)),
                    this, SLOT(slotSelectAudioLevel(int)));

            // Set the prepend text on the audio fader
            m_audioFader->m_fader->setPrependText(i18n("Playback level = "));
        }
    }

    m_audioFader->m_recordButton->setOn(value);
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
    int key = (index << 24) + m_selectedInstrument->getId();

    if (m_pluginDialogs[key] == 0)
    {
        // only create a dialog if we've got a plugin instance
        Rosegarden::AudioPluginInstance *inst = 
            m_selectedInstrument->getPlugin(index);

        if (inst)
        {
            // Create the plugin dialog
            //
            m_pluginDialogs[key] = 
                new Rosegarden::AudioPluginDialog(this,
                                              m_doc->getPluginManager(),
                                              m_selectedInstrument,
                                              index);

            // Get the App pointer and plug the new dialog into the 
            // standard keyboard accelerators so that we can use them
            // still while the plugin has focus.
            //
            QWidget *par = parentWidget();
            while (!par->isTopLevel()) par = par->parentWidget();

            RosegardenGUIApp *app = dynamic_cast<RosegardenGUIApp*>(par);

            app->plugAccelerators(m_pluginDialogs[key],
                                  m_pluginDialogs[key]->getAccelerators());

            connect(m_pluginDialogs[key], SIGNAL(pluginSelected(int, int)),
                    this, SLOT(slotPluginSelected(int, int)));

            connect(m_pluginDialogs[key],
		    SIGNAL(pluginPortChanged(int, int, float)),
                    this, SLOT(slotPluginPortChanged(int, int, float)));

            connect(m_pluginDialogs[key], SIGNAL(bypassed(int, bool)),
                    this, SLOT(slotBypassed(int, bool)));

	    connect(m_pluginDialogs[key], SIGNAL(destroyed(int)),
		    this, SLOT(slotPluginDialogDestroyed(int)));

            m_pluginDialogs[key]->show();

            // Set modified
            m_doc->slotDocumentModified();
        }
        else
        {
	    std::cerr << "AudioInstrumentParameterPanel::slotSelectPlugin - "
		      << "no AudioPluginInstance found for index "
		      << index << std::endl;
        }
    }
    else
    {
	m_pluginDialogs[key]->raise();
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
                RG_DEBUG << "InstrumentParameterBox::slotPluginSelected - "
                         << "cannot destroy Studio object "
                         << inst->getMappedId() << endl;
            }

            inst->setAssigned(false);
            m_audioFader->m_plugins[index]->setText(i18n("<no plugin>"));

        }
        else
        {
            Rosegarden::AudioPlugin *plgn = 
                m_doc->getPluginManager()->getPlugin(plugin);

            // If unassigned then create a sequencer instance of this
            // AudioPluginInstance.
            //
            if (inst->isAssigned())
            {
                // unassign, destory and recreate
                std::cout << "MAPPED ID = " << inst->getMappedId() 
			  << " for Instrument " << inst->getId() << std::endl;

                RG_DEBUG << "InstrumentParameterBox::slotPluginSelected - "
                         << "MappedObjectId = "
                         << inst->getMappedId()
                         << " - UniqueId = " << plgn->getUniqueId()
                         << endl;


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

                RG_DEBUG << "InstrumentParameterBox::slotPluginSelected - "
                         << " new MappedObjectId = " << newId << endl;

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
                = m_doc->getPluginManager()->getPlugin(plugin);

            if (pluginClass)
                m_audioFader->m_plugins[index]->
                    setText(pluginClass->getLabel());
        }

        // Set modified
        m_doc->slotDocumentModified();
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
                                
        RG_DEBUG << "InstrumentParameterBox::slotPluginPortChanged - "
                 << "setting plugin port (" << portIndex << ") to "
                 << value << endl;

        // Set modified
        m_doc->slotDocumentModified();

#endif // HAVE_LADSPA
    }

}


void
AudioInstrumentParameterPanel::slotPluginDialogDestroyed(int index)
{
    int key = (index << 24) + m_selectedInstrument->getId();
    m_pluginDialogs[key] = 0;
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


        /// Set the colour on the button
        //
        setBypassButtonColour(pluginIndex, bp);

        // Set the bypass on the instance
        //
        inst->setBypass(bp);

        // Set modified
        m_doc->slotDocumentModified();
    }
}

// Set the button colour
//
void
AudioInstrumentParameterPanel::setBypassButtonColour(int pluginIndex,
                                                     bool bypassState)
{
    // Set the bypass colour on the plugin button
    if (bypassState)
    {
        m_audioFader->m_plugins[pluginIndex]->
            setPaletteForegroundColor(kapp->palette().
                    color(QPalette::Active, QColorGroup::Button));

        m_audioFader->m_plugins[pluginIndex]->
            setPaletteBackgroundColor(kapp->palette().
                    color(QPalette::Active, QColorGroup::ButtonText));
    }
    else
    {
        m_audioFader->m_plugins[pluginIndex]->
            setPaletteForegroundColor(kapp->palette().
                    color(QPalette::Active, QColorGroup::ButtonText));

        m_audioFader->m_plugins[pluginIndex]->
            setPaletteBackgroundColor(kapp->palette().
                    color(QPalette::Active, QColorGroup::Button));
    }
}


void
MIDIInstrumentParameterPanel::slotSelectChorus(float index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setChorus(Rosegarden::MidiByte(index));

    Rosegarden::MappedEvent *mE = 
     new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                 Rosegarden::MappedEvent::MidiController,
                                 Rosegarden::MIDI_CONTROLLER_CHORUS,
                                 (Rosegarden::MidiByte)index);
    Rosegarden::StudioControl::sendMappedEvent(mE);

    updateAllBoxes();
}

void
MIDIInstrumentParameterPanel::slotSelectReverb(float index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setReverb(Rosegarden::MidiByte(index));

    Rosegarden::MappedEvent *mE = 
     new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                 Rosegarden::MappedEvent::MidiController,
                                 Rosegarden::MIDI_CONTROLLER_REVERB,
                                 (Rosegarden::MidiByte)index);
    Rosegarden::StudioControl::sendMappedEvent(mE);

    updateAllBoxes();
}


void
MIDIInstrumentParameterPanel::slotSelectHighPass(float index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setFilter(Rosegarden::MidiByte(index));

    Rosegarden::MappedEvent *mE = 
     new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                 Rosegarden::MappedEvent::MidiController,
                                 Rosegarden::MIDI_CONTROLLER_FILTER,
                                 (Rosegarden::MidiByte)index);
    Rosegarden::StudioControl::sendMappedEvent(mE);

    updateAllBoxes();
}

void
MIDIInstrumentParameterPanel::slotSelectResonance(float index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setResonance(Rosegarden::MidiByte(index));

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

    updateAllBoxes();
}


void
MIDIInstrumentParameterPanel::slotSelectAttack(float index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setAttack(Rosegarden::MidiByte(index));

    Rosegarden::MappedEvent *mE = 
     new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                 Rosegarden::MappedEvent::MidiController,
                                 Rosegarden::MIDI_CONTROLLER_ATTACK,
                                 (Rosegarden::MidiByte)index);
    Rosegarden::StudioControl::sendMappedEvent(mE);

    updateAllBoxes();
}

void
MIDIInstrumentParameterPanel::slotSelectRelease(float index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setRelease(Rosegarden::MidiByte(index));

    Rosegarden::MappedEvent *mE = 
     new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                 Rosegarden::MappedEvent::MidiController,
                                 Rosegarden::MIDI_CONTROLLER_RELEASE,
                                 (Rosegarden::MidiByte)index);
    Rosegarden::StudioControl::sendMappedEvent(mE);

    updateAllBoxes();
}


InstrumentParameterPanel::InstrumentParameterPanel(RosegardenGUIDoc *doc, 
                                                   QWidget* parent)
    : QFrame(parent),
      m_instrumentLabel(new QLabel(this)),
      m_selectedInstrument(0),
      m_doc(doc)
{
}



AudioInstrumentParameterPanel::AudioInstrumentParameterPanel(RosegardenGUIDoc* doc, QWidget* parent)
    : InstrumentParameterPanel(doc, parent),
      m_audioFader(new AudioFaderWidget(this, "instrumentAudioFader", false))
{
    QGridLayout *gridLayout = new QGridLayout(this, 10, 6, 5, 5);

    // Instrument label : first row, all cols
    gridLayout->addMultiCellWidget(m_instrumentLabel, 0, 0, 0, 5, AlignCenter);

    // fader and connect it
    gridLayout->addMultiCellWidget(m_audioFader, 1, 9, 2, 3);

    connect(m_audioFader, SIGNAL(audioChannelsChanged(int)),
            this, SLOT(slotAudioChannels(int)));

    connect(m_audioFader->m_signalMapper, SIGNAL(mapped(int)),
            this, SLOT(slotSelectPlugin(int)));

    connect(m_audioFader->m_fader, SIGNAL(faderChanged(int)),
            this, SLOT(slotSelectAudioLevel(int)));

    connect(m_audioFader->m_muteButton, SIGNAL(clicked()),
            this, SLOT(slotMute()));

    connect(m_audioFader->m_soloButton, SIGNAL(clicked()),
            this, SLOT(slotSolo()));

    connect(m_audioFader->m_recordButton, SIGNAL(clicked()),
            this, SLOT(slotRecord()));

    connect(m_audioFader->m_pan, SIGNAL(valueChanged(float)),
            this, SLOT(slotSetPan(float)));

    connect(m_audioFader->m_audioInput, SIGNAL(activated(int)),
            this, SLOT(slotSelectAudioInput(int)));

}

void
AudioInstrumentParameterPanel::slotMute()
{
    RG_DEBUG << "AudioInstrumentParameterPanel::slotMute" << endl;
    emit muteButton(m_selectedInstrument->getId(),
                    m_audioFader->m_muteButton->isOn());
}

void
AudioInstrumentParameterPanel::slotSolo()
{
    RG_DEBUG << "AudioInstrumentParameterPanel::slotSolo" << endl;
    emit soloButton(m_selectedInstrument->getId(),
                    m_audioFader->m_soloButton->isOn());
}

void
AudioInstrumentParameterPanel::slotRecord()
{
    RG_DEBUG << "AudioInstrumentParameterPanel::slotRecord - " 
             << " isOn = " <<  m_audioFader->m_recordButton->isOn() << endl;

    // At the moment we can't turn a recording button off
    //
    if (m_audioFader->m_recordButton->isOn())
    {
        m_audioFader->m_recordButton->setOn(true);

        if (m_selectedInstrument)
        {
            //std::cout << "SETTING FADER RECORD LEVEL = " 
                 //<< int(m_selectedInstrument->getRecordLevel()) << endl;

            // set the fader value to the record value
            m_audioFader->m_fader->
                setFader(m_selectedInstrument->getRecordLevel());
        }

        emit recordButton(m_selectedInstrument->getId(),
                          m_audioFader->m_recordButton->isOn());
    }
    else
    {
        m_audioFader->m_recordButton->setOn(true);
    }

    /*
    else
    {
        if (m_selectedInstrument)
        {
            // set the fader value to the record value
            m_audioFader->m_fader->
                setFader(m_selectedInstrument->getVelocity());

            cout << "SETTING FADER LEVEL = " 
                 << int(m_selectedInstrument->getVelocity()) << endl;
        }
    }
    */
}

void
AudioInstrumentParameterPanel::slotSetPan(float pan)
{
    RG_DEBUG << "AudioInstrumentParameterPanel::slotSetPan - "
             << "pan = " << pan << endl;

    Rosegarden::StudioControl::setStudioObjectProperty
        (Rosegarden::MappedObjectId(m_selectedInstrument->getMappedId()),
         Rosegarden::MappedAudioFader::Pan,
         Rosegarden::MappedObjectValue(pan));

    m_selectedInstrument->setPan(Rosegarden::MidiByte(pan + 100.0));
}

void
AudioInstrumentParameterPanel::setAudioMeter(double ch1, double ch2)
{
    if (m_selectedInstrument)
    {
        if (m_selectedInstrument->getAudioChannels() == 1)
            m_audioFader->m_vuMeter->setLevel(ch1);
        else
            m_audioFader->m_vuMeter->setLevel(ch1, ch2);
    }
}


void
AudioInstrumentParameterPanel::setupForInstrument(Rosegarden::Instrument* instrument)
{
    m_selectedInstrument = instrument;

    m_instrumentLabel->setText(strtoqstr(instrument->getName()));

    /*
    if (m_audioFader->m_recordButton->isOn())
        m_audioFader->m_fader->setFader(instrument->getRecordLevel());
    else
        m_audioFader->m_fader->setFader(instrument->getVelocity());
        */


    for (unsigned int i = 0; i < m_audioFader->m_plugins.size(); i++)
    {
        m_audioFader->m_plugins[i]->show();

        Rosegarden::AudioPluginInstance *inst = instrument->getPlugin(i);

        if (inst && inst->isAssigned())
        {
            Rosegarden::AudioPlugin *pluginClass 
                = m_doc->getPluginManager()->getPlugin(
                        m_doc->getPluginManager()->
                            getPositionByUniqueId(inst->getId()));

            if (pluginClass)
                m_audioFader->m_plugins[i]->
                    setText(pluginClass->getLabel());

            setBypassButtonColour(i, inst->isBypassed());

        }
        else
        {
            m_audioFader->m_plugins[i]->setText(i18n("<no plugin>"));

            if (inst)
                setBypassButtonColour(i, inst->isBypassed());
            else
                setBypassButtonColour(i, false);
        }
    }

    // Set the number of channels on the fader widget
    //
    m_audioFader->setAudioChannels(instrument->getAudioChannels());

    // Pan - adjusted backwards
    //
    m_audioFader->m_pan->setPosition(instrument->getPan() - 100);
}

void
AudioInstrumentParameterPanel::slotAudioChannels(int channels)
{
    RG_DEBUG << "AudioInstrumentParameterPanel::slotAudioChannels - "
             << "channels = " << channels << endl;

    m_selectedInstrument->setAudioChannels(channels);
    /*
    Rosegarden::MappedEvent *mE =
        new Rosegarden::MappedEvent(
                m_selectedInstrument->getId(),
                Rosegarden::MappedEvent::AudioChannels,
                Rosegarden::MidiByte(m_selectedInstrument->getAudioChannels()));

    Rosegarden::StudioControl::sendMappedEvent(mE);
    */

    Rosegarden::StudioControl::setStudioObjectProperty
        (Rosegarden::MappedObjectId(m_selectedInstrument->getMappedId()),
         Rosegarden::MappedAudioObject::Channels,
         Rosegarden::MappedObjectValue(channels));

}

void
AudioInstrumentParameterPanel::slotSelectAudioInput(int value)
{
    RG_DEBUG << "AudioInstrumentParameterPanel::slotSelectAudioInput - "
             << value << endl;

    /*
   Rosegarden::MappedObjectValueList connectionList;
   connectionList.push_back(value);
   */

   Rosegarden::StudioControl::setStudioObjectProperty
        (Rosegarden::MappedObjectId(m_selectedInstrument->getMappedId()),
         Rosegarden::MappedAudioObject::ConnectionsIn,
         Rosegarden::MappedObjectValue(value));
}




MIDIInstrumentParameterPanel::MIDIInstrumentParameterPanel(RosegardenGUIDoc *doc, QWidget* parent)
    : InstrumentParameterPanel(doc, parent),
      m_deviceLabel(new QLabel(this)),
      m_bankValue(new KComboBox(false, this)),
      m_channelValue(new RosegardenComboBox(true, false, this)),
      m_programValue(new KComboBox(this)),
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
    m_volumeRotary->setKnobColour(RosegardenGUIColours::RotaryPastelBlue);

    // light red
    m_panRotary->setKnobColour(RosegardenGUIColours::RotaryPastelRed);

    // light green
    m_chorusRotary->setKnobColour(RosegardenGUIColours::RotaryPastelGreen);
    m_reverbRotary->setKnobColour(RosegardenGUIColours::RotaryPastelGreen);

    // light orange
    m_highPassRotary->setKnobColour(RosegardenGUIColours::RotaryPastelOrange);
    m_resonanceRotary->setKnobColour(RosegardenGUIColours::RotaryPastelOrange);

    // light yellow
    m_attackRotary->setKnobColour(RosegardenGUIColours::RotaryPastelYellow);
    m_releaseRotary->setKnobColour(RosegardenGUIColours::RotaryPastelYellow);

    QGridLayout *gridLayout = new QGridLayout(this, 12, 3, 8, 1);

    // Some top space
    gridLayout->addRowSpacing(0, 8);
    gridLayout->addRowSpacing(1, 30);

    // Ensure a reasonable amount of space in the program dropdowns even
    // if no instrument initially selected
    QFontMetrics metrics(m_programValue->font());
    int width = metrics.width("Acoustic Grand Piano 123");
    m_bankValue->setMinimumWidth(width);
    m_programValue->setMinimumWidth(width);

/*!!!
    m_bankValue->setMinimumWidth(100);
    m_programValue->setMinimumWidth(100);
*/

    // MIDI widgets
    //
    gridLayout->addMultiCellWidget(m_instrumentLabel, 0, 0, 0, 2, AlignCenter);
    gridLayout->addMultiCellWidget(m_deviceLabel, 1, 1, 0, 2, AlignCenter);

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
        m_channelValue->insertItem(QString("%1").arg(i+1));

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
/*!!!
    connect(m_bankValue, SIGNAL(propagate(int)),
            this, SLOT(slotSelectBank(int)));

    connect(m_programValue, SIGNAL(propagate(int)),
            this, SLOT(slotSelectProgram(int)));

    connect(m_channelValue, SIGNAL(propagate(int)),
            this, SLOT(slotSelectChannel(int)));
*/

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

//!!! dup with trackbuttons.cpp
static QString
getPresentationName(Rosegarden::Instrument *instr)
{
    if (!instr) {
	return i18n("<no instrument>");
    } else if (instr->getType() == Rosegarden::Instrument::Audio) {
	return strtoqstr(instr->getName());
    } else {
	return strtoqstr(instr->getDevice()->getName() + " " + 
			 instr->getName());
    }
}


void
MIDIInstrumentParameterPanel::setupForInstrument(Rosegarden::Instrument *instrument)
{
    m_selectedInstrument = instrument;

    // Set instrument name
    //
    m_instrumentLabel->setText(getPresentationName(instrument));

    // Set Studio Device name
    //
    if (instrument->getDevice()) {
	//!!! we should call this the connection label, I guess
	std::string connection(instrument->getDevice()->getConnection());
	if (connection == "") {
	    m_deviceLabel->setText(i18n("No connection"));
	} else {
	    m_deviceLabel->setText(i18n("Connection: %1").
				   arg(strtoqstr(connection)));
	}
    } else {
	m_deviceLabel->setText("");
    }

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
	m_programValue->clear();
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



