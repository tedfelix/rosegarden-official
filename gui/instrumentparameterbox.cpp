// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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
#include <qregexp.h>
#include <qslider.h>
#include <qpushbutton.h>
#include <qsignalmapper.h>
#include <qwidgetstack.h>
#include <qsignalmapper.h>

#include <kcombobox.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <klocale.h>
#include <kapp.h>

#include <algorithm>

#include "Midi.h"
#include "Instrument.h"
#include "MidiDevice.h"
#include "MappedStudio.h"
#include "ControlParameter.h"
#include "AudioLevel.h"

#include "audioplugindialog.h"
#include "instrumentparameterbox.h"
#include "audiopluginmanager.h"
#include "widgets.h"
#include "studiocontrol.h"
#include "rosegardenguidoc.h"
#include "studiowidgets.h"
#include "rosegardengui.h"

#include "rosestrings.h"
#include "rosedebug.h"

#include "studiocontrol.h"
#include "studiowidgets.h"

using Rosegarden::Instrument;
using Rosegarden::MidiDevice;

InstrumentParameterBox::InstrumentParameterBox(RosegardenGUIDoc *doc,
                                               QWidget *parent)
    : RosegardenParameterBox(1, Qt::Horizontal, i18n("Instrument Parameters"), parent),
      m_widgetStack(new QWidgetStack(this)),
      m_noInstrumentParameters(new QVBox(this)),
      m_midiInstrumentParameters(new MIDIInstrumentParameterPanel(doc, this)),
      m_audioInstrumentParameters(new AudioInstrumentParameterPanel(doc, this)),
      m_selectedInstrument(0),
      m_doc(doc)
{
    m_widgetStack->setFont(m_font);
    m_noInstrumentParameters->setFont(m_font);
    m_midiInstrumentParameters->setFont(m_font);
    m_audioInstrumentParameters->setFont(m_font);

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
	    SIGNAL(instrumentParametersChanged(Rosegarden::InstrumentId)),
	    this,
	    SIGNAL(instrumentParametersChanged(Rosegarden::InstrumentId)));

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

    connect(m_audioInstrumentParameters,
	    SIGNAL(selectPlugin(QWidget *, Rosegarden::InstrumentId, int)),
	    this,
	    SIGNAL(selectPlugin(QWidget *, Rosegarden::InstrumentId, int)));

    connect(m_audioInstrumentParameters,
	    SIGNAL(startPluginGUI(Rosegarden::InstrumentId, int)),
	    this,
	    SIGNAL(startPluginGUI(Rosegarden::InstrumentId, int)));
    
    connect(m_midiInstrumentParameters, SIGNAL(updateAllBoxes()),
            this, SLOT(slotUpdateAllBoxes()));

    connect(m_midiInstrumentParameters, SIGNAL(changeInstrumentLabel(Rosegarden::InstrumentId, QString)),
            this, SIGNAL(changeInstrumentLabel(Rosegarden::InstrumentId, QString)));

    connect(m_midiInstrumentParameters,
	    SIGNAL(instrumentParametersChanged(Rosegarden::InstrumentId)),
	    this,
	    SIGNAL(instrumentParametersChanged(Rosegarden::InstrumentId)));
}

InstrumentParameterPanel::InstrumentParameterPanel(RosegardenGUIDoc *doc, 
                                                   QWidget* parent)
    : QFrame(parent),
      m_instrumentLabel(new QLabel(this)),
      m_selectedInstrument(0),
      m_doc(doc)
{
}

void
InstrumentParameterPanel::setDocument(RosegardenGUIDoc* doc)
{
    m_doc = doc;
}

InstrumentParameterBox::~InstrumentParameterBox()
{
    // deregister this parameter box
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
InstrumentParameterBox::setAudioMeter(float ch1, float ch2)
{
    m_audioInstrumentParameters->setAudioMeter(ch1, ch2);
}

void
InstrumentParameterBox::setDocument(RosegardenGUIDoc* doc)
{
    m_doc = doc;
    m_midiInstrumentParameters->setDocument(m_doc);
    m_audioInstrumentParameters->setDocument(m_doc);
}

void
InstrumentParameterBox::slotPluginSelected(Rosegarden::InstrumentId id, int index, int plugin)
{
    m_audioInstrumentParameters->slotPluginSelected(id, index, plugin);
}

void
InstrumentParameterBox::slotPluginBypassed(Rosegarden::InstrumentId id, int index, bool bypassState)
{
    m_audioInstrumentParameters->slotPluginBypassed(id, index, bypassState);
}

void
InstrumentParameterBox::useInstrument(Instrument *instrument)
{
    RG_DEBUG << "useInstrument() - populate Instrument\n";

    if (instrument == 0)
    {
        m_widgetStack->raiseWidget(m_noInstrumentParameters);
        return;
    } 

    // ok
    m_selectedInstrument = instrument;

    // Hide or Show according to Instrument type
    //
    if (instrument->getType() == Instrument::Audio ||
	instrument->getType() == Instrument::SoftSynth)
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
	(m_selectedInstrument->getType() == Instrument::Audio ||
	 m_selectedInstrument->getType() == Instrument::SoftSynth))
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
	(m_selectedInstrument->getType() == Instrument::Audio ||
	 m_selectedInstrument->getType() == Instrument::SoftSynth))
    {
        m_audioInstrumentParameters->slotSetRecord(value);
    }
}

void
InstrumentParameterBox::setSolo(bool value)
{
    if (m_selectedInstrument &&
	(m_selectedInstrument->getType() == Instrument::Audio ||
	 m_selectedInstrument->getType() == Instrument::SoftSynth))
    {
        m_audioInstrumentParameters->slotSetSolo(value);
    }
}

    

void
InstrumentParameterBox::slotUpdateAllBoxes()
{
    RG_DEBUG << "InstrumentParameterBox::slotUpdateAllBoxes" << endl;

    std::vector<InstrumentParameterBox*>::iterator it =
        instrumentParamBoxes.begin();

    // To update all open IPBs
    //
    for (; it != instrumentParamBoxes.end(); it++)
    {
        if ((*it) != this && m_selectedInstrument &&
            (*it)->getSelectedInstrument() == m_selectedInstrument)
            (*it)->useInstrument(m_selectedInstrument);
    }
}

void
InstrumentParameterBox::slotInstrumentParametersChanged(Rosegarden::InstrumentId id)
{
    std::vector<InstrumentParameterBox*>::iterator it =
        instrumentParamBoxes.begin();

    blockSignals(true);

    for (; it != instrumentParamBoxes.end(); it++)
    {
	if ((*it)->getSelectedInstrument()) {
	    if ((*it)->getSelectedInstrument()->getId() == id) {
		(*it)->useInstrument((*it)->getSelectedInstrument()); // refresh
	    }
	}	    
    }

    blockSignals(false);
}



void
AudioInstrumentParameterPanel::slotSelectAudioLevel(float dB)
{
    if (m_selectedInstrument == 0)
        return;

    std::cerr << "AudioInstrumentParameterPanel::slotSelectAudioLevel("
	      << dB << ")" << std::endl;

    if (m_selectedInstrument->getType() == Instrument::Audio ||
	m_selectedInstrument->getType() == Instrument::SoftSynth)
    {
	m_selectedInstrument->setLevel(dB);

	Rosegarden::StudioControl::setStudioObjectProperty
	    (Rosegarden::MappedObjectId(m_selectedInstrument->getMappedId()),
	     Rosegarden::MappedAudioFader::FaderLevel,
	     Rosegarden::MappedObjectValue(dB));
    }

    emit updateAllBoxes();
    emit instrumentParametersChanged(m_selectedInstrument->getId());
}

void
AudioInstrumentParameterPanel::slotSelectAudioRecordLevel(float dB)
{
    if (m_selectedInstrument == 0)
        return;

    std::cerr << "AudioInstrumentParameterPanel::slotSelectAudioRecordLevel("
	      << dB << ")" << std::endl;

    //!!! nb shouldn't have a record level fader at all on soft synth box

    if (m_selectedInstrument->getType() == Instrument::Audio)
    {
	m_selectedInstrument->setRecordLevel(dB);

	Rosegarden::StudioControl::setStudioObjectProperty
	    (Rosegarden::MappedObjectId(m_selectedInstrument->getMappedId()),
	     Rosegarden::MappedAudioFader::FaderRecordLevel,
	     Rosegarden::MappedObjectValue(dB));

	emit updateAllBoxes();
	emit instrumentParametersChanged(m_selectedInstrument->getId());
    }
}

void 
AudioInstrumentParameterPanel::slotSetMute(bool value)
{
    RG_DEBUG << "AudioInstrumentParameterPanel::slotSetMute - "
             << "value = " << value << endl;
    m_audioFader->m_muteButton->setOn(value);
    emit instrumentParametersChanged(m_selectedInstrument->getId());
}

void
AudioInstrumentParameterPanel::slotSetSolo(bool value)
{
    RG_DEBUG << "AudioInstrumentParameterPanel::slotSetSolo - "
             << "value = " << value << endl;
    m_audioFader->m_soloButton->setOn(value);
    emit instrumentParametersChanged(m_selectedInstrument->getId());
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
    }
    else
    {
        m_audioFader->m_recordButton->unsetPalette();
    }

    m_audioFader->m_recordButton->setOn(value);
    emit instrumentParametersChanged(m_selectedInstrument->getId());
}


void
AudioInstrumentParameterPanel::slotPluginSelected(Rosegarden::InstrumentId instrumentId,
						  int index, int plugin)
{
    if (!m_selectedInstrument ||
	instrumentId != m_selectedInstrument->getId()) return;

    RG_DEBUG << "AudioInstrumentParameterPanel::slotPluginSelected - "
             << "instrument = " << instrumentId
             << ", index = " << index
             << ", plugin = " << plugin << endl;

    QColor pluginBackgroundColour = Qt::black;
    bool bypassed = false;

    QPushButton *button = 0;
    QString noneText;

    if (index == Rosegarden::Instrument::SYNTH_PLUGIN_POSITION) {
	button = m_synthButton;
	noneText = i18n("<no synth>");
    } else {
	button = m_audioFader->m_plugins[index];
	noneText = i18n("<no plugin>");
    }

    if (!button) return;

    if (plugin == -1) {

	button->setText(noneText);
        QToolTip::add(button, noneText);

    } else {

	Rosegarden::AudioPlugin *pluginClass 
	    = m_doc->getPluginManager()->getPlugin(plugin);

	if (pluginClass)
        {
	    button->setText(pluginClass->getLabel());

            QToolTip::add(button, pluginClass->getLabel());

            pluginBackgroundColour = pluginClass->getColour();
        }
    }

    Rosegarden::AudioPluginInstance *inst = 
        m_selectedInstrument->getPlugin(index);

    if (inst)
        bypassed = inst->isBypassed();

    setButtonColour(index, bypassed, pluginBackgroundColour);
}

void
AudioInstrumentParameterPanel::slotPluginBypassed(Rosegarden::InstrumentId instrumentId,
						  int pluginIndex, bool bp)
{
    if (!m_selectedInstrument ||
	instrumentId != m_selectedInstrument->getId()) return;

    Rosegarden::AudioPluginInstance *inst = 
        m_selectedInstrument->getPlugin(pluginIndex);

    QColor backgroundColour = Qt::black; // default background colour

    if (inst && inst->isAssigned())
    {
        Rosegarden::AudioPlugin *pluginClass 
            = m_doc->getPluginManager()->getPlugin(
                    m_doc->getPluginManager()->
                        getPositionByIdentifier(inst->getIdentifier().c_str()));

        /// Set the colour on the button
        //
        if (pluginClass)
            backgroundColour = pluginClass->getColour();
    }

    setButtonColour(pluginIndex, bp, backgroundColour);
}

// Set the button colour
//
void
AudioInstrumentParameterPanel::setButtonColour(
        int pluginIndex, bool bypassState, const QColor &colour)
{
    RG_DEBUG << "AudioInstrumentParameterPanel::setButtonColour " 
        << "pluginIndex = " << pluginIndex
        << ", bypassState = " << bypassState
        << ", rgb = " << colour.name() << endl;

    QPushButton *button = 0;

    if (pluginIndex == Rosegarden::Instrument::SYNTH_PLUGIN_POSITION) {
	button = m_synthButton;
    } else {
	button = m_audioFader->m_plugins[pluginIndex];
    }

    if (!button) return;

    // Set the bypass colour on the plugin button
    if (bypassState)
    {
	button->
            setPaletteForegroundColor(kapp->palette().
                    color(QPalette::Active, QColorGroup::Button));

	button->
            setPaletteBackgroundColor(kapp->palette().
                    color(QPalette::Active, QColorGroup::ButtonText));
    }
    else if (colour == Qt::black)
    {
	button->
            setPaletteForegroundColor(kapp->palette().
                    color(QPalette::Active, QColorGroup::ButtonText));

	button->
            setPaletteBackgroundColor(kapp->palette().
                    color(QPalette::Active, QColorGroup::Button));
    }
    else
    {
	button->
            setPaletteForegroundColor(Qt::white);

	button->
	    setPaletteBackgroundColor(colour);
    }
}


AudioInstrumentParameterPanel::AudioInstrumentParameterPanel(RosegardenGUIDoc* doc, QWidget* parent)
    : InstrumentParameterPanel(doc, parent),
      m_audioFader(new AudioFaderWidget(this, AudioFaderWidget::FaderBox))
{
    QGridLayout *gridLayout = new QGridLayout(this, 3, 2, 5, 5);

    // Instrument label : first row, all cols
    gridLayout->addMultiCellWidget(m_instrumentLabel, 0, 0, 0, 1, AlignCenter);

    m_synthButton = new QPushButton(this);
    m_synthButton->setText(i18n("<no synth>"));
    QToolTip::add(m_synthButton, i18n("Synth plugin button"));
    connect(m_synthButton, SIGNAL(clicked()), this, SLOT(slotSynthButtonClicked()));
    gridLayout->addMultiCellWidget(m_synthButton, 1, 1, 0, 0, AlignRight);

    m_synthGUIButton = new QPushButton(this);
    m_synthGUIButton->setText(i18n("GUI"));
    connect(m_synthGUIButton, SIGNAL(clicked()), this, SLOT(slotSynthGUIButtonClicked()));
    gridLayout->addMultiCellWidget(m_synthGUIButton, 1, 1, 1, 1, AlignLeft);

    // fader and connect it
    gridLayout->addMultiCellWidget(m_audioFader, 2, 2, 0, 1);

    connect(m_audioFader, SIGNAL(audioChannelsChanged(int)),
            this, SLOT(slotAudioChannels(int)));

    connect(m_audioFader->m_signalMapper, SIGNAL(mapped(int)),
            this, SLOT(slotSelectPlugin(int)));

    connect(m_audioFader->m_fader, SIGNAL(faderChanged(float)),
            this, SLOT(slotSelectAudioLevel(float)));

    connect(m_audioFader->m_recordFader, SIGNAL(faderChanged(float)),
            this, SLOT(slotSelectAudioRecordLevel(float)));

    connect(m_audioFader->m_muteButton, SIGNAL(clicked()),
            this, SLOT(slotMute()));

    connect(m_audioFader->m_soloButton, SIGNAL(clicked()),
            this, SLOT(slotSolo()));

    connect(m_audioFader->m_recordButton, SIGNAL(clicked()),
            this, SLOT(slotRecord()));

    connect(m_audioFader->m_pan, SIGNAL(valueChanged(float)),
            this, SLOT(slotSetPan(float)));

    connect(m_audioFader->m_audioOutput, SIGNAL(changed()),
            this, SLOT(slotAudioRoutingChanged()));

    connect(m_audioFader->m_audioInput, SIGNAL(changed()),
            this, SLOT(slotAudioRoutingChanged()));
}

void
AudioInstrumentParameterPanel::slotSynthButtonClicked()
{
    slotSelectPlugin(Rosegarden::Instrument::SYNTH_PLUGIN_POSITION);
}

void
AudioInstrumentParameterPanel::slotSynthGUIButtonClicked()
{
    emit startPluginGUI(m_selectedInstrument->getId(),
			Rosegarden::Instrument::SYNTH_PLUGIN_POSITION);
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
        emit recordButton(m_selectedInstrument->getId(),
                          m_audioFader->m_recordButton->isOn());
    }
    else
    {
        m_audioFader->m_recordButton->setOn(true);
    }
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
    emit instrumentParametersChanged(m_selectedInstrument->getId());
}

void
AudioInstrumentParameterPanel::setAudioMeter(float dBleft, float dBright)
{
//    RG_DEBUG << "AudioInstrumentParameterPanel::setAudioMeter: (" << dBleft
//	     << "," << dBright << ")" << endl;
    
    if (m_selectedInstrument)
    {
        if (m_selectedInstrument->getAudioChannels() == 1) {
	    m_audioFader->m_vuMeter->setLevel(dBleft);
	} else {
	    m_audioFader->m_vuMeter->setLevel(dBleft, dBright);
	}
    }
}


void
AudioInstrumentParameterPanel::setupForInstrument(Instrument* instrument)
{
    blockSignals(true);

    m_selectedInstrument = instrument;

    m_instrumentLabel->setText(strtoqstr(instrument->getName()));

    m_audioFader->m_recordFader->setFader(instrument->getRecordLevel());
    m_audioFader->m_fader->setFader(instrument->getLevel());

    m_audioFader->slotSetInstrument(&m_doc->getStudio(), instrument);

    int start = 0;
    if (instrument->getType() == Rosegarden::Instrument::SoftSynth) {
	m_synthButton->show();
	m_synthGUIButton->show();
	start = -1;
    } else {
	m_synthButton->hide();
	m_synthGUIButton->hide();
    }

    for (int i = start; i < int(m_audioFader->m_plugins.size()); i++)
    {
	int index;
	QPushButton *button;
	QString noneText;

	if (i == -1) {
	    index = Rosegarden::Instrument::SYNTH_PLUGIN_POSITION;
	    button = m_synthButton;
	    noneText = i18n("<no synth>");
	} else {
	    index = i;
	    button = m_audioFader->m_plugins[i];
	    noneText = i18n("<no plugin>");
	}

	button->show();

        Rosegarden::AudioPluginInstance *inst = instrument->getPlugin(index);

        if (inst && inst->isAssigned())
        {
            Rosegarden::AudioPlugin *pluginClass 
                = m_doc->getPluginManager()->getPlugin(
                        m_doc->getPluginManager()->
                            getPositionByIdentifier(inst->getIdentifier().c_str()));

            if (pluginClass)
            {
                button->setText(pluginClass->getLabel());
                QToolTip::add(button, pluginClass->getLabel());
                setButtonColour(index, inst->isBypassed(), 
                        pluginClass->getColour());
            }
        }
        else
        {
            button->setText(noneText);
            QToolTip::add(button, noneText);
            setButtonColour(index, inst ? inst->isBypassed() : false, Qt::black);
        }
    }

    // Set the number of channels on the fader widget
    //
    m_audioFader->setAudioChannels(instrument->getAudioChannels());

    // Pan - adjusted backwards
    //
    m_audioFader->m_pan->setPosition(instrument->getPan() - 100);

    // Tell fader box whether to include e.g. audio input selection
    // 
    m_audioFader->setIsSynth(instrument->getType() == Rosegarden::Instrument::SoftSynth);

    blockSignals(false);
}

void
AudioInstrumentParameterPanel::slotAudioChannels(int channels)
{
    RG_DEBUG << "AudioInstrumentParameterPanel::slotAudioChannels - "
             << "channels = " << channels << endl;

    m_selectedInstrument->setAudioChannels(channels);

    Rosegarden::StudioControl::setStudioObjectProperty
        (Rosegarden::MappedObjectId(m_selectedInstrument->getMappedId()),
         Rosegarden::MappedAudioFader::Channels,
         Rosegarden::MappedObjectValue(channels));

    emit instrumentParametersChanged(m_selectedInstrument->getId());

}

void
AudioInstrumentParameterPanel::slotAudioRoutingChanged()
{
    if (m_selectedInstrument)
	emit instrumentParametersChanged(m_selectedInstrument->getId());
}

void
AudioInstrumentParameterPanel::slotSelectPlugin(int index)
{
    if (m_selectedInstrument)
	emit selectPlugin(0, m_selectedInstrument->getId(), index);
}



MIDIInstrumentParameterPanel::MIDIInstrumentParameterPanel(RosegardenGUIDoc *doc, QWidget* parent):
    InstrumentParameterPanel(doc, parent),
    m_rotaryFrame(0),
    m_rotaryMapper(new QSignalMapper(this))
{
    m_mainGrid = new QGridLayout(this, 8, 3, 8, 1);

    m_connectionLabel = new QLabel(this);
    m_bankValue = new KComboBox(this);
    m_channelValue = new KComboBox(this);
    m_programValue = new KComboBox(this);
    m_variationValue = new KComboBox(this);
    m_bankCheckBox = new QCheckBox(this);
    m_programCheckBox = new QCheckBox(this);
    m_variationCheckBox = new QCheckBox(this);
    m_percussionCheckBox = new QCheckBox(this);

    m_bankLabel = new QLabel(i18n("Bank"), this);
    m_variationLabel = new QLabel(i18n("Variation"), this);
    QLabel* programLabel = new QLabel(i18n("Program"), this);
    QLabel* channelLabel = new QLabel(i18n("Channel"), this);
    QLabel *percussionLabel = new QLabel(i18n("Percussion"), this);

    // Ensure a reasonable amount of space in the program dropdowns even
    // if no instrument initially selected
    QFontMetrics metrics(m_programValue->font());
    int width = metrics.width("Acoustic Grand Piano 123");
    m_bankValue->setMinimumWidth(width);
    m_programValue->setMinimumWidth(width);

    m_mainGrid->addMultiCellWidget(m_instrumentLabel, 0, 0, 0, 2, AlignCenter);
    m_mainGrid->addMultiCellWidget(m_connectionLabel, 1, 1, 0, 2, AlignCenter);

    m_mainGrid->addWidget(channelLabel, 2, 0, AlignLeft);
    m_mainGrid->addWidget(m_channelValue, 2, 2, AlignRight);

    m_mainGrid->addWidget(percussionLabel, 3, 0, AlignLeft);
    m_mainGrid->addWidget(m_percussionCheckBox, 3, 2, AlignRight);

    m_mainGrid->addWidget(m_bankLabel, 4, 0, AlignLeft);
    m_mainGrid->addWidget(m_bankCheckBox, 4, 1);
    m_mainGrid->addWidget(m_bankValue, 4, 2, AlignRight);

    m_mainGrid->addWidget(programLabel, 5, 0);
    m_mainGrid->addWidget(m_programCheckBox, 5, 1);
    m_mainGrid->addWidget(m_programValue, 5, 2, AlignRight);

    m_mainGrid->addWidget(m_variationLabel, 6, 0);
    m_mainGrid->addWidget(m_variationCheckBox, 6, 1);
    m_mainGrid->addWidget(m_variationValue, 6, 2, AlignRight);

    // Populate channel list
    for (int i = 0; i < 16; i++)
        m_channelValue->insertItem(QString("%1").arg(i+1));

    // Disable these by default - they are activate by their
    // checkboxes
    //
    m_programValue->setDisabled(true);
    m_bankValue->setDisabled(true);
    m_variationValue->setDisabled(true);

    // Only active if we have an Instrument selected
    //
    m_percussionCheckBox->setDisabled(true);
    m_programCheckBox->setDisabled(true);
    m_bankCheckBox->setDisabled(true);
    m_variationCheckBox->setDisabled(true);

    // Connect up the toggle boxes
    //
    connect(m_percussionCheckBox, SIGNAL(toggled(bool)),
	    this, SLOT(slotTogglePercussion(bool)));

    connect(m_programCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotToggleProgramChange(bool)));

    connect(m_bankCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotToggleBank(bool)));

    connect(m_variationCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotToggleVariation(bool)));


    // Connect activations
    //
    connect(m_bankValue, SIGNAL(activated(int)),
            this, SLOT(slotSelectBank(int)));

    connect(m_variationValue, SIGNAL(activated(int)),
            this, SLOT(slotSelectVariation(int)));

    connect(m_programValue, SIGNAL(activated(int)),
            this, SLOT(slotSelectProgram(int)));

    connect(m_channelValue, SIGNAL(activated(int)),
            this, SLOT(slotSelectChannel(int)));



    // don't select any of the options in any dropdown
    m_programValue->setCurrentItem(-1);
    m_bankValue->setCurrentItem(-1);
    m_channelValue->setCurrentItem(-1);
    m_variationValue->setCurrentItem(-1);

    connect(m_rotaryMapper, SIGNAL(mapped(int)),
            this, SLOT(slotControllerChanged(int)));
}

void
MIDIInstrumentParameterPanel::setupForInstrument(Instrument *instrument)
{
    MidiDevice *md = dynamic_cast<MidiDevice*>
	(instrument->getDevice());
    if (!md) {
	RG_DEBUG << "WARNING: MIDIInstrumentParameterPanel::setupForInstrument:"
                 << " No MidiDevice for Instrument "
	         << instrument->getId() << endl;
	return;
    }

    m_selectedInstrument = instrument;

    // Set instrument name
    //
    m_instrumentLabel->setText(strtoqstr(instrument->getPresentationName()));

    // Set Studio Device name
    //
    QString connection(strtoqstr(md->getConnection()));
    if (connection == "") {
	m_connectionLabel->setText(i18n("[ %1 ]").arg(i18n("No connection")));
    } else {
	
	// remove trailing "(duplex)", "(read only)", "(write only)" etc
	connection.replace(QRegExp("\\s*\\([^)0-9]+\\)\\s*$"), "");
	
	QString text = i18n("[ %1 ]").arg(connection);
	QString origText(text);
	
	QFontMetrics metrics(m_connectionLabel->fontMetrics());
	int maxwidth = metrics.width
	    ("Program: [X]   Acoustic Grand Piano 123");// kind of arbitrary!
	
	int hlen = text.length() / 2;
	while (metrics.width(text) > maxwidth && text.length() > 10) {
	    --hlen;
	    text = origText.left(hlen) + "..." + origText.right(hlen);
	}
	
	if (text.length() > origText.length() - 7) text = origText;
	m_connectionLabel->setText(text);
    }

    // Enable all check boxes
    //
    m_percussionCheckBox->setDisabled(false);
    m_programCheckBox->setDisabled(false);
    m_bankCheckBox->setDisabled(false);
    m_variationCheckBox->setDisabled(false);

    // Activate all checkboxes
    //
    m_percussionCheckBox->setChecked(instrument->isPercussion());
    m_programCheckBox->setChecked(instrument->sendsProgramChange());
    m_bankCheckBox->setChecked(instrument->sendsBankSelect());
    m_variationCheckBox->setChecked(instrument->sendsBankSelect());

    // Basic parameters
    //
    m_channelValue->setCurrentItem((int)instrument->getMidiChannel());

    // Check for program change
    //
    populateBankList();
    populateProgramList();
    populateVariationList();

    // Setup the ControlParameters
    //
    setupControllers(md);

    // Set all the positions by controller number
    //
    for (RotaryMap::iterator it = m_rotaries.begin() ;
            it != m_rotaries.end(); ++it)
    {
        Rosegarden::MidiByte value = 0;

        // Special cases
        //
        if (it->first == Rosegarden::MIDI_CONTROLLER_PAN)
            value = int(instrument->getPan());
        else if (it->first == Rosegarden::MIDI_CONTROLLER_VOLUME)
            value = int(instrument->getVolume());
        else
        {
            try
            {
                value = instrument->getControllerValue(
                        Rosegarden::MidiByte(it->first));
            }
            catch(...)
            {
                continue;
            }
        }

        setRotaryToValue(it->first, int(value));
    }
}

void
MIDIInstrumentParameterPanel::setupControllers(MidiDevice *md)
{
    if (!m_rotaryFrame) {
	m_rotaryFrame = new QFrame(this);
	m_mainGrid->addMultiCellWidget(m_rotaryFrame, 7, 7, 0, 2, Qt::AlignHCenter);
	m_rotaryGrid = new QGridLayout(m_rotaryFrame, 10, 3, 8, 1);
	m_rotaryGrid->addItem(new QSpacerItem(10, 4), 0, 1);
    }

    // To cut down on flicker, we avoid destroying and recreating
    // widgets as far as possible here.  If a label already exists,
    // we just set its text; if a rotary exists, we only replace it
    // if we actually need a different one.

    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::ControlList list = md->getControlParameters();

    // sort by IPB position
    //
    std::sort(list.begin(), list.end(),
	      Rosegarden::ControlParameter::ControlPositionCmp());

    int count = 0;
    RotaryMap::iterator rmi = m_rotaries.begin();

    for (Rosegarden::ControlList::iterator it = list.begin();
	 it != list.end(); ++it)
    {
        if (it->getIPBPosition() == -1) continue;

	// Get the knob colour - only if the colour is non-default (>0)
	//
	QColor knobColour = Qt::black; // special case for RosegardenRotary
	if (it->getColourIndex() > 0) {
	    Rosegarden::Colour c =
		comp.getGeneralColourMap().getColourByIndex
		(it->getColourIndex());
	    knobColour = QColor(c.getRed(), c.getGreen(), c.getBlue());
	}

	RosegardenRotary *rotary = 0;

	if (rmi != m_rotaries.end()) {

	    rotary = rmi->second.first;
	    int redraw = 0; // 1 -> position, 2 -> all

	    if (rotary->getMinValue() != it->getMin()) {
		rotary->setMinValue(it->getMin());
		redraw = 1;
	    }
	    if (rotary->getMaxValue() != it->getMax()) {
		rotary->setMaxValue(it->getMax());
		redraw = 1;
	    }
	    if (rotary->getKnobColour() != knobColour) {
		rotary->setKnobColour(knobColour);
		redraw = 2;
	    }
	    if (redraw == 1 || rotary->getPosition() != it->getDefault()) {
		rotary->setPosition(it->getDefault());
		if (redraw == 1) redraw = 0;
	    }
	    if (redraw == 2) {
		rotary->repaint();
	    }

	    QLabel *label = rmi->second.second;
	    label->setText(strtoqstr(it->getName()));

	    ++rmi;

	} else {

	    QHBox *hbox = new QHBox(m_rotaryFrame);
	    hbox->setSpacing(8);

	    float smallStep = 1.0;

	    float bigStep = 5.0;
	    if (it->getMax() - it->getMin() < 10) bigStep = 1.0;
	    else if (it->getMax() - it->getMin() < 20) bigStep = 2.0;

	    rotary = new RosegardenRotary
		(hbox,
		 it->getMin(),
		 it->getMax(),
		 smallStep,
		 bigStep,
		 it->getDefault(),
		 20,
		 RosegardenRotary::NoTicks,
		 false,
		 it->getDefault() == 64); //!!! hacky

	    rotary->setKnobColour(knobColour);

	    // Add a label
	    QLabel *label = new QLabel(strtoqstr(it->getName()), hbox);

	    RG_DEBUG << "Adding new widget at " << (count/2) << "," << (count%2) << endl;

	    // Add the compound widget
	    //
	    m_rotaryGrid->addWidget(hbox, count/2, (count%2) * 2, AlignLeft);
	    hbox->show();

	    // Add to list
	    //
	    m_rotaries.push_back(std::pair<int, RotaryPair>
				 (it->getControllerValue(),
				  RotaryPair(rotary, label)));

	    // Connect
	    //
	    connect(rotary, SIGNAL(valueChanged(float)),
		    m_rotaryMapper, SLOT(map()));

	    rmi = m_rotaries.end();
	}

	// Add signal mapping
	//
	m_rotaryMapper->setMapping(rotary,
				   int(it->getControllerValue()));

        count++;
    }

    if (rmi != m_rotaries.end()) {
	for (RotaryMap::iterator rmj = rmi; rmj != m_rotaries.end(); ++rmj) {
	    delete rmj->second.first;
	    delete rmj->second.second;
	}
	m_rotaries = std::vector<std::pair<int, RotaryPair> >
	    (m_rotaries.begin(), rmi);
    }

    m_rotaryFrame->show();
}


void
MIDIInstrumentParameterPanel::setRotaryToValue(int controller, int value)
{
    /*
    RG_DEBUG << "MIDIInstrumentParameterPanel::setRotaryToValue - "
             << "controller = " << controller
             << ", value = " << value << std::endl;
             */

    for (RotaryMap::iterator it = m_rotaries.begin() ; it != m_rotaries.end(); ++it)
    {
        if (it->first == controller)
        {
            it->second.first->setPosition(float(value));
            return;
        }
    }
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


void
MIDIInstrumentParameterPanel::populateBankList()
{
    if (m_selectedInstrument == 0)
	return;

    m_bankValue->clear();
    m_banks.clear();

    MidiDevice *md = dynamic_cast<MidiDevice*>
	(m_selectedInstrument->getDevice());
    if (!md) {
	RG_DEBUG << "WARNING: MIDIInstrumentParameterPanel::populateBankList:"
                 << " No MidiDevice for Instrument "
                 << m_selectedInstrument->getId() << endl;
	return;
    }

    int currentBank = -1;
    Rosegarden::BankList banks;

    /*
    RG_DEBUG << "MIDIInstrumentParameterPanel::populateBankList: "
             << "variation type is " << md->getVariationType() << endl;
             */

    if (md->getVariationType() == MidiDevice::NoVariations) {

	if (m_bankLabel->isHidden()) {
	    m_bankLabel->show();
	    m_bankCheckBox->show();
	    m_bankValue->show();
	}
	banks = md->getBanks(m_selectedInstrument->isPercussion());

	for (unsigned int i = 0; i < banks.size(); ++i) {
	    if (m_selectedInstrument->getProgram().getBank() == banks[i]) {
		currentBank = i;
	    }
	}

    } else {

	Rosegarden::MidiByteList bytes;
	bool useMSB = (md->getVariationType() == MidiDevice::VariationFromLSB);

	if (useMSB) {
	    bytes = md->getDistinctMSBs(m_selectedInstrument->isPercussion());
	} else {
	    bytes = md->getDistinctLSBs(m_selectedInstrument->isPercussion());
	}
	
	if (bytes.size() < 2) {
	    if (!m_bankLabel->isHidden()) {
		m_bankLabel->hide();
		m_bankCheckBox->hide();
		m_bankValue->hide();
	    }
	} else {
	    if (m_bankLabel->isHidden()) {
		m_bankLabel->show();
		m_bankCheckBox->show();
		m_bankValue->show();
	    }
	}

	if (useMSB) {
	    for (unsigned int i = 0; i < bytes.size(); ++i) {
		Rosegarden::BankList bl = md->getBanksByMSB
		    (m_selectedInstrument->isPercussion(), bytes[i]);
		RG_DEBUG << "MIDIInstrumentParameterPanel::populateBankList: have " << bl.size() << " variations for msb " << bytes[i] << endl;

		if (bl.size() == 0) continue;
		if (m_selectedInstrument->getMSB() == bytes[i]) {
		    currentBank = banks.size();
		}
		banks.push_back(bl[0]);
	    }
	} else {
	    for (unsigned int i = 0; i < bytes.size(); ++i) {
		Rosegarden::BankList bl = md->getBanksByLSB
		    (m_selectedInstrument->isPercussion(), bytes[i]);
		RG_DEBUG << "MIDIInstrumentParameterPanel::populateBankList: have " << bl.size() << " variations for lsb " << bytes[i] << endl;
		if (bl.size() == 0) continue;
		if (m_selectedInstrument->getLSB() == bytes[i]) {
		    currentBank = banks.size();
		}
		banks.push_back(bl[0]);
	    }
	}
    }

    for (Rosegarden::BankList::const_iterator i = banks.begin();
	 i != banks.end(); ++i) {
	m_banks.push_back(*i);
	m_bankValue->insertItem(strtoqstr(i->getName()));
    }
	    
    m_bankValue->setCurrentItem(currentBank);
    m_bankValue->setEnabled(m_selectedInstrument->sendsBankSelect());
}    
	
// Populate program list by bank context
//
void
MIDIInstrumentParameterPanel::populateProgramList()
{
    if (m_selectedInstrument == 0)
        return;

    m_programValue->clear();
    m_programs.clear();

    MidiDevice *md = dynamic_cast<MidiDevice*>
	(m_selectedInstrument->getDevice());
    if (!md) {
	RG_DEBUG << "WARNING: MIDIInstrumentParameterPanel::populateProgramList: No MidiDevice for Instrument "
                 << m_selectedInstrument->getId() << endl;
	return;
    }

    /*
    RG_DEBUG << "MIDIInstrumentParameterPanel::populateProgramList:"
             << " variation type is " << md->getVariationType() << endl;
    */

    Rosegarden::MidiBank bank(m_selectedInstrument->isPercussion(), 0, 0);

    if (m_selectedInstrument->sendsBankSelect()) {
	bank = m_selectedInstrument->getProgram().getBank();
    }

    Rosegarden::ProgramList programs = md->getPrograms(bank);
    for (unsigned int i = 0; i < programs.size(); ++i) {
	std::string programName = programs[i].getName();
	if (programName != "") {
	    m_programValue->insertItem(QString("%1. %2")
				       .arg(programs[i].getProgram() + 1)
				       .arg(strtoqstr(programName)));
	    if (m_selectedInstrument->getProgram() == programs[i]) {
		m_programValue->setCurrentItem(i);
	    }
	    m_programs.push_back(programs[i]);
	}
    }

    m_programValue->setEnabled(m_selectedInstrument->sendsProgramChange());

    // Ensure that stored program change value is same as the one
    // we're now showing (BUG 937371)
    //
    if (m_programs.size())
    {
        m_selectedInstrument->setProgramChange
            ((m_programs[m_programValue->currentItem()]).getProgram());
    }
}

void
MIDIInstrumentParameterPanel::populateVariationList()
{
    if (m_selectedInstrument == 0)
	return;

    m_variationValue->clear();
    m_variations.clear();

    MidiDevice *md = dynamic_cast<MidiDevice*>
	(m_selectedInstrument->getDevice());
    if (!md) {
	RG_DEBUG << "WARNING: MIDIInstrumentParameterPanel::populateVariationList: No MidiDevice for Instrument "
		  << m_selectedInstrument->getId() << endl;
	return;
    }

    /*
    RG_DEBUG << "MIDIInstrumentParameterPanel::populateVariationList:"
             << " variation type is " << md->getVariationType() << endl;
    */

    if (md->getVariationType() == MidiDevice::NoVariations) {
	if (!m_variationLabel->isHidden()) {
	    m_variationLabel->hide();
	    m_variationCheckBox->hide();
	    m_variationValue->hide();
	}
	return;
    } 

    bool useMSB = (md->getVariationType() == MidiDevice::VariationFromMSB);
    Rosegarden::MidiByteList variations;

    if (useMSB) {
	Rosegarden::MidiByte lsb = m_selectedInstrument->getLSB();
	variations = md->getDistinctMSBs(m_selectedInstrument->isPercussion(),
					 lsb);
	RG_DEBUG << "MIDIInstrumentParameterPanel::populateVariationList: have " << variations.size() << " variations for lsb " << lsb << endl;

    } else {
	Rosegarden::MidiByte msb = m_selectedInstrument->getMSB();
	variations = md->getDistinctLSBs(m_selectedInstrument->isPercussion(),
					 msb);
	RG_DEBUG << "MIDIInstrumentParameterPanel::populateVariationList: have " << variations.size() << " variations for msb " << msb << endl;
    }
    
    m_variationValue->setCurrentItem(-1);

    Rosegarden::MidiProgram defaultProgram;

    if (useMSB) {
	defaultProgram = Rosegarden::MidiProgram
	    (Rosegarden::MidiBank(m_selectedInstrument->isPercussion(),
				  0,
				  m_selectedInstrument->getLSB()),
	     m_selectedInstrument->getProgramChange());
    } else {
	defaultProgram = Rosegarden::MidiProgram
	    (Rosegarden::MidiBank(m_selectedInstrument->isPercussion(),
				  m_selectedInstrument->getMSB(),
				  0),
	     m_selectedInstrument->getProgramChange());
    }
    std::string defaultProgramName = md->getProgramName(defaultProgram);

    for (unsigned int i = 0; i < variations.size(); ++i) {

	Rosegarden::MidiProgram program;

	if (useMSB) {
	    program = Rosegarden::MidiProgram
		(Rosegarden::MidiBank(m_selectedInstrument->isPercussion(),
				      variations[i],
				      m_selectedInstrument->getLSB()),
		 m_selectedInstrument->getProgramChange());
	} else {
	    program = Rosegarden::MidiProgram
		(Rosegarden::MidiBank(m_selectedInstrument->isPercussion(),
				      m_selectedInstrument->getMSB(),
				      variations[i]),
		 m_selectedInstrument->getProgramChange());
	}

	std::string programName = md->getProgramName(program);

	if (programName != "") { // yes, that is how you know whether it exists
/*
	    m_variationValue->insertItem(programName == defaultProgramName ?
					 i18n("(default)") :
					 strtoqstr(programName));
*/
	    m_variationValue->insertItem(QString("%1. %2")
					 .arg(variations[i] + 1)
					 .arg(strtoqstr(programName)));
	    if (m_selectedInstrument->getProgram() == program) {
		m_variationValue->setCurrentItem(i);
	    }
	    m_variations.push_back(variations[i]);
	}
    }

    if (m_variations.size() < 2) {
	if (!m_variationLabel->isHidden()) {
	    m_variationLabel->hide();
	    m_variationCheckBox->hide();
	    m_variationValue->hide();
	}
	
    } else {
	//!!! seem to have problems here -- the grid layout doesn't
	//like us adding stuff in the middle so if we go from 1
	//visible row (say program) to 2 (program + variation) the
	//second one overlaps the control knobs

	if (m_variationLabel->isHidden()) {
	    m_variationLabel->show();
	    m_variationCheckBox->show();
	    m_variationValue->show();
	}

	if (m_programValue->width() > m_variationValue->width()) {
	    m_variationValue->setMinimumWidth(m_programValue->width());
	} else {
	    m_programValue->setMinimumWidth(m_variationValue->width());
	}
    }

    m_variationValue->setEnabled(m_selectedInstrument->sendsBankSelect());
}


void
MIDIInstrumentParameterPanel::slotTogglePercussion(bool value)
{
    if (m_selectedInstrument == 0)
    {
	m_percussionCheckBox->setChecked(false);
        emit updateAllBoxes();
        return;
    }

    m_selectedInstrument->setPercussion(value);

    populateBankList();
    populateProgramList();
    populateVariationList();

    sendBankAndProgram();

    emit changeInstrumentLabel(m_selectedInstrument->getId(),
			       strtoqstr(m_selectedInstrument->
					 getProgramName()));
    emit updateAllBoxes();
}


void
MIDIInstrumentParameterPanel::slotToggleBank(bool value)
{
    if (m_selectedInstrument == 0)
    {
        m_bankCheckBox->setChecked(false);
        emit updateAllBoxes();
        return;
    }

    m_variationCheckBox->setChecked(value);
    m_selectedInstrument->setSendBankSelect(value);

    m_bankValue->setDisabled(!value);
    populateBankList();
    populateProgramList();
    populateVariationList();

    sendBankAndProgram();

    emit changeInstrumentLabel(m_selectedInstrument->getId(),
			       strtoqstr(m_selectedInstrument->
					 getProgramName()));
    emit updateAllBoxes();
}

void
MIDIInstrumentParameterPanel::slotToggleProgramChange(bool value)
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
    populateVariationList();

    if (value) sendBankAndProgram();

    emit changeInstrumentLabel(m_selectedInstrument->getId(),
			       strtoqstr(m_selectedInstrument->
					 getProgramName()));
    emit updateAllBoxes();
}

void
MIDIInstrumentParameterPanel::slotToggleVariation(bool value)
{
    if (m_selectedInstrument == 0)
    {
        m_variationCheckBox->setChecked(false);
        emit updateAllBoxes();
        return;
    }

    m_bankCheckBox->setChecked(value);
    m_selectedInstrument->setSendBankSelect(value);

    m_variationValue->setDisabled(!value);
    populateVariationList();

    sendBankAndProgram();

    emit changeInstrumentLabel(m_selectedInstrument->getId(),
			       strtoqstr(m_selectedInstrument->
					 getProgramName()));
    emit updateAllBoxes();
}


void
MIDIInstrumentParameterPanel::slotSelectBank(int index)
{
    if (m_selectedInstrument == 0)
        return;

    MidiDevice *md = dynamic_cast<MidiDevice*>
	(m_selectedInstrument->getDevice());
    if (!md) {
	RG_DEBUG << "WARNING: MIDIInstrumentParameterPanel::slotSelectBank: No MidiDevice for Instrument "
		  << m_selectedInstrument->getId() << endl;
	return;
    }

    const Rosegarden::MidiBank *bank = &m_banks[index];

    if (md->getVariationType() != MidiDevice::VariationFromLSB) {
	m_selectedInstrument->setLSB(bank->getLSB());
    }
    if (md->getVariationType() != MidiDevice::VariationFromMSB) {
	m_selectedInstrument->setMSB(bank->getMSB());
    }

    populateProgramList();

    sendBankAndProgram();

    emit updateAllBoxes();
}

void
MIDIInstrumentParameterPanel::slotSelectProgram(int index)
{
    const Rosegarden::MidiProgram *prg = &m_programs[index];
    if (prg == 0) {
        RG_DEBUG << "program change not found in bank" << endl;
        return;
    }
    m_selectedInstrument->setProgramChange(prg->getProgram());

    sendBankAndProgram();

    populateVariationList();

    emit changeInstrumentLabel(m_selectedInstrument->getId(),
			       strtoqstr(m_selectedInstrument->
					 getProgramName()));
    emit updateAllBoxes();
}

void
MIDIInstrumentParameterPanel::slotSelectVariation(int index)
{
    MidiDevice *md = dynamic_cast<MidiDevice*>
	(m_selectedInstrument->getDevice());
    if (!md) {
	RG_DEBUG << "WARNING: MIDIInstrumentParameterPanel::slotSelectVariation: No MidiDevice for Instrument "
		  << m_selectedInstrument->getId() << endl;
	return;
    }

    if (index < 0 || index > int(m_variations.size())) {
	RG_DEBUG << "WARNING: MIDIInstrumentParameterPanel::slotSelectVariation: index " << index << " out of range" << endl;
	return;
    }

    Rosegarden::MidiByte v = m_variations[index];
    
    if (md->getVariationType() == MidiDevice::VariationFromLSB) {
	m_selectedInstrument->setLSB(v);
    } else if (md->getVariationType() == MidiDevice::VariationFromMSB) {
	m_selectedInstrument->setMSB(v);
    }

    sendBankAndProgram();
}

void
MIDIInstrumentParameterPanel::sendBankAndProgram()
{
    if (m_selectedInstrument == 0)
        return;

    MidiDevice *md = dynamic_cast<MidiDevice*>
	(m_selectedInstrument->getDevice());
    if (!md) {
	RG_DEBUG << "WARNING: MIDIInstrumentParameterPanel::slotSelectBank: No MidiDevice for Instrument "
		  << m_selectedInstrument->getId() << endl;
	return;
    }

    if (m_selectedInstrument->sendsBankSelect()) {

        // Send the bank select message before any PC message
        //
        Rosegarden::MappedEvent mEMSB(m_selectedInstrument->getId(), 
                                      Rosegarden::MappedEvent::MidiController,
                                      Rosegarden::MIDI_CONTROLLER_BANK_MSB,
                                      m_selectedInstrument->getMSB());
        Rosegarden::StudioControl::sendMappedEvent(mEMSB);

        Rosegarden::MappedEvent mELSB(m_selectedInstrument->getId(), 
                                      Rosegarden::MappedEvent::MidiController,
                                      Rosegarden::MIDI_CONTROLLER_BANK_LSB,
                                      m_selectedInstrument->getLSB());
        Rosegarden::StudioControl::sendMappedEvent(mELSB);
    }

    Rosegarden::MappedEvent mE(m_selectedInstrument->getId(), 
                               Rosegarden::MappedEvent::MidiProgramChange,
                               m_selectedInstrument->getProgramChange(),
                               (Rosegarden::MidiByte)0);

    RG_DEBUG << "MIDIInstrumentParameterPanel::sendBankAndProgram - "
             << "sending program change = " 
             << int(m_selectedInstrument->getProgramChange()) 
             << endl;


    // Send the controller change
    //
    Rosegarden::StudioControl::sendMappedEvent(mE);
}


void
MIDIInstrumentParameterPanel::slotControllerChanged(int controllerNumber)
{
    /*
    RG_DEBUG<< "MIDIInstrumentParameterPanel::slotControllerChanged - "
            << "controller = " << controllerNumber << "\n";
    */

    if (m_selectedInstrument == 0)
        return;

    MidiDevice *md = dynamic_cast<MidiDevice*>
	(m_selectedInstrument->getDevice());
    if (!md) return;

    /*
    Rosegarden::ControlParameter *controller = 
	md->getControlParameter(Rosegarden::MidiByte(controllerNumber));
        */

    int value = getValueFromRotary(controllerNumber);

    if (value == -1)
    {
        RG_DEBUG << "MIDIInstrumentParameterPanel::slotControllerChanged - "
                 << "couldn't get value of rotary for controller " 
                 << controllerNumber << "\n";
        return;
    }


    // two special cases
    if (controllerNumber == int(Rosegarden::MIDI_CONTROLLER_PAN))
    {
        float adjValue = value;
        if (m_selectedInstrument->getType() == Instrument::Audio ||
	    m_selectedInstrument->getType() == Instrument::SoftSynth)
            value += 100;

        m_selectedInstrument->setPan(Rosegarden::MidiByte(adjValue));
    }
    else if (controllerNumber == int(Rosegarden::MIDI_CONTROLLER_VOLUME))
    {
        m_selectedInstrument->setVolume(Rosegarden::MidiByte(value));
    }
    else // just set the controller (this will create it on the instrument if
         // it doesn't exist)
    {
        m_selectedInstrument->setControllerValue(Rosegarden::MidiByte(controllerNumber),
                                                 Rosegarden::MidiByte(value));

        //RG_DEBUG << "SET CONTROLLER VALUE (" << controllerNumber << ") = " << value << std::endl;
    }
    /*
    else
    {
        RG_DEBUG << "MIDIInstrumentParameterPanel::slotControllerChanged - "
                 << "no controller retrieved\n";
        return;
    }
    */

    Rosegarden::MappedEvent mE(m_selectedInstrument->getId(), 
                               Rosegarden::MappedEvent::MidiController,
                               (Rosegarden::MidiByte)controllerNumber,
                               (Rosegarden::MidiByte)value);
    Rosegarden::StudioControl::sendMappedEvent(mE);

    emit updateAllBoxes();
    emit instrumentParametersChanged(m_selectedInstrument->getId());
    
}

int
MIDIInstrumentParameterPanel::getValueFromRotary(int rotary)
{
    for (RotaryMap::iterator it = m_rotaries.begin(); it != m_rotaries.end(); ++it)
    {
        if (it->first == rotary)
            return int(it->second.first->getPosition());
    }

    return -1;
}

