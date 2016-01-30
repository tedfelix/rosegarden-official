/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[AudioInstrumentParameterPanel]"

#include "AudioInstrumentParameterPanel.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/AudioPluginInstance.h"
#include "base/Instrument.h"
#include "base/InstrumentStaticSignals.h"
#include "base/MidiProgram.h"
#include "document/RosegardenDocument.h"
#include "gui/studio/AudioPluginManager.h"
#include "gui/studio/AudioPlugin.h"
#include "gui/studio/StudioControl.h"
#include "gui/widgets/AudioVUMeter.h"
#include "gui/widgets/Fader.h"
#include "gui/widgets/Rotary.h"
#include "gui/widgets/AudioRouteMenu.h"
#include "gui/widgets/SqueezedLabel.h"
#include "InstrumentParameterPanel.h"
#include "sound/MappedCommon.h"
#include "sound/MappedStudio.h"
#include "gui/widgets/PluginPushButton.h"

#include <QColor>
#include <QFrame>
#include <QLabel>
#include <QPalette>
#include <QPixmap>
#include <QPushButton>
#include <QString>
#include <QToolTip>
#include <QWidget>
#include <QLayout>
#include <QSignalMapper>
#include <QApplication>



namespace Rosegarden
{

AudioInstrumentParameterPanel::AudioInstrumentParameterPanel(
        RosegardenDocument *doc, QWidget *parent) :
    InstrumentParameterPanel(doc, parent),
    m_audioFader(this),
    m_aliasButton(this)
{
    setObjectName("Audio Instrument Parameter Panel");

    QFont font;
    font.setPointSize(font.pointSize() * 90 / 100);
    font.setBold(false);

    // Widgets

    // Alias button
    // we should have a change alias button here too (remember, it changes the
    // alias by itself, so we just have to update to reflect the change)
    m_aliasButton.setFixedSize(10, 6); // golden rectangle
    m_aliasButton.setToolTip(tr("Click to rename this instrument"));

    // Instrument label
    QFontMetrics metrics(font);
    int width25 = metrics.width("1234567890123456789012345");
    m_instrumentLabel.setFont(font);
    m_instrumentLabel.setFixedWidth(width25);
    m_instrumentLabel.setAlignment(Qt::AlignCenter);
    m_instrumentLabel.setToolTip(tr("Click the button above to rename this instrument"));
    m_instrumentLabel.setText("REFRESH BUG!"); // no tr(); temporary internal string

    // Audio Fader Box
    m_audioFader.setFont(font);
    connect(&m_audioFader, SIGNAL(audioChannelsChanged(int)),
            this, SLOT(slotAudioChannels(int)));
    connect(m_audioFader.m_signalMapper, SIGNAL(mapped(int)),
            this, SLOT(slotSelectPlugin(int)));
    connect(m_audioFader.m_fader, SIGNAL(faderChanged(float)),
            this, SLOT(slotSelectAudioLevel(float)));
    connect(m_audioFader.m_recordFader, SIGNAL(faderChanged(float)),
            this, SLOT(slotSelectAudioRecordLevel(float)));
    connect(m_audioFader.m_pan, SIGNAL(valueChanged(float)),
            this, SLOT(slotSetPan(float)));
    connect(m_audioFader.m_synthButton, SIGNAL(clicked()),
            this, SLOT(slotSynthButtonClicked()));
    connect(m_audioFader.m_synthGUIButton, SIGNAL(clicked()),
            this, SLOT(slotSynthGUIButtonClicked()));

    // Layout

    QGridLayout *gridLayout = new QGridLayout(this);
    gridLayout->setSpacing(5);
    gridLayout->setMargin(0);
    // The alias button and the label are in 0,0.  The only difference is the
    // alignment.
    gridLayout->addWidget(&m_aliasButton, 0, 0, 1, 2, Qt::AlignLeft);
    gridLayout->addWidget(&m_instrumentLabel, 0, 0, 1, 2, Qt::AlignCenter);

    gridLayout->addWidget(&m_audioFader, 1, 0, 1, 2);

    // Make row 2 fill up the rest of the space.
    gridLayout->setRowStretch(2, 1);

    // Panel

    setLayout(gridLayout);
    setContentsMargins(5, 7, 5, 2);

    connect(Instrument::getStaticSignals().data(),
            SIGNAL(changed(Instrument *)),
            this,
            SLOT(slotInstrumentChanged(Instrument *)));
}

void
AudioInstrumentParameterPanel::slotSelectAudioLevel(float dB)
{
    if (getSelectedInstrument() == 0)
        return ;

    if (getSelectedInstrument()->getType() == Instrument::Audio ||
            getSelectedInstrument()->getType() == Instrument::SoftSynth) {
        getSelectedInstrument()->setLevel(dB);
        getSelectedInstrument()->changed();

        // ??? Perhaps it would be better for StudioControl to monitor
        //     the Instrument objects (InstrumentStaticSignals::changed())
        //     and update the properties whenever the Instrument changes.
        //     Then this StudioControl code which is spread all over this
        //     class and duplicated in AudioMixerWindow would go away.
        StudioControl::setStudioObjectProperty
        (MappedObjectId(getSelectedInstrument()->getMappedId()),
         MappedAudioFader::FaderLevel,
         MappedObjectValue(dB));
    }
}

void
AudioInstrumentParameterPanel::slotSelectAudioRecordLevel(float dB)
{
    if (getSelectedInstrument() == 0)
        return ;

    //    std::cerr << "AudioInstrumentParameterPanel::slotSelectAudioRecordLevel("
    //          << dB << ")" << std::endl;

    if (getSelectedInstrument()->getType() == Instrument::Audio) {
        getSelectedInstrument()->setRecordLevel(dB);
        getSelectedInstrument()->changed();

        StudioControl::setStudioObjectProperty
        (MappedObjectId(getSelectedInstrument()->getMappedId()),
         MappedAudioFader::FaderRecordLevel,
         MappedObjectValue(dB));
    }
}

void
AudioInstrumentParameterPanel::slotPluginSelected(InstrumentId instrumentId,
        int index, int plugin)
{
    if (!getSelectedInstrument() ||
        instrumentId != getSelectedInstrument()->getId()) {
        return;
    }

    RG_DEBUG << "AudioInstrumentParameterPanel::slotPluginSelected - "
             << "instrument = " << instrumentId
             << ", index = " << index
             << ", plugin = " << plugin << endl;

    QColor pluginBackgroundColour = QColor(Qt::black);
    bool bypassed = false;

    PluginPushButton *button = 0;
    QString noneText;

    // updates synth gui button &c:
    m_audioFader.slotSetInstrument(&m_doc->getStudio(), getSelectedInstrument());

    if (index == (int)Instrument::SYNTH_PLUGIN_POSITION) {
        button = m_audioFader.m_synthButton;
        noneText = tr("<no synth>");
    } else {
        button = m_audioFader.m_plugins[index];
        noneText = tr("<no plugin>");
    }

    if (!button)
        return ;

    if (plugin == -1) {

        button->setText(noneText);
        button->setToolTip(noneText);

    } else {

        AudioPlugin *pluginClass = m_doc->getPluginManager()->getPlugin(plugin);

        if (pluginClass) {
            button->setText(pluginClass->getLabel());

            button->setToolTip(pluginClass->getLabel());

            pluginBackgroundColour = pluginClass->getColour();
        }
    }

    AudioPluginInstance *inst =
        getSelectedInstrument()->getPlugin(index);

    if (inst)
        bypassed = inst->isBypassed();

    setButtonColour(index, bypassed, pluginBackgroundColour);

    if (index == (int)Instrument::SYNTH_PLUGIN_POSITION) {
        // A new plugin has been selected.  Let everyone know.
        // This makes sure the TrackParameterBox and TrackButtons
        // are updated to show the name of the plugin.
        // ??? Yeah, but.  It would be better if the part of this that
        //     actually changed the Instrument (AudioPluginDialog maybe?)
        //     called changed() when it made the actual changes.  The
        //     emitters of pluginSelected() would be a place to start.
        getSelectedInstrument()->changed();
    }
}

void
AudioInstrumentParameterPanel::slotPluginBypassed(InstrumentId instrumentId,
        int pluginIndex, bool bp)
{
    if (!getSelectedInstrument() ||
            instrumentId != getSelectedInstrument()->getId())
        return ;

    AudioPluginInstance *inst =
        getSelectedInstrument()->getPlugin(pluginIndex);

    QColor backgroundColour = QColor(Qt::black); // default background colour

    if (inst && inst->isAssigned()) {
        AudioPlugin *pluginClass
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

void
AudioInstrumentParameterPanel::setButtonColour(
    int pluginIndex, bool bypassState, const QColor &colour)
{
    RG_DEBUG << "AudioInstrumentParameterPanel::setButtonColour "
    << "pluginIndex = " << pluginIndex
    << ", bypassState = " << bypassState
    << ", rgb = " << colour.name() << endl;

    PluginPushButton *button = 0;

    if (pluginIndex == int(Instrument::SYNTH_PLUGIN_POSITION)) {
        button = m_audioFader.m_synthButton;
    } else {
        button = m_audioFader.m_plugins[pluginIndex];
    }

    if (!button)
        return ;

    // Set the plugin active, plugin bypassed, or stock color.  For the moment
    // this is still figured using the old "colour" parameter that is still
    // passed around, so this conversion is a bit hacky, and we really should
    // (//!!!) create some enabled/disabled/active state for the plugins
    // themselves that doesn't depend on colo(u)r for calculation.
    if (bypassState) {
        button->setState(PluginPushButton::Bypassed);
    } else if (colour == QColor(Qt::black)) {
        button->setState(PluginPushButton::Normal);
    } else {
        button->setState(PluginPushButton::Active);
    }
}

void
AudioInstrumentParameterPanel::slotSynthButtonClicked()
{
    RG_DEBUG << "AudioInstrumentParameterPanel::slotSynthButtonClicked()" << endl;
    slotSelectPlugin(Instrument::SYNTH_PLUGIN_POSITION);
}

void
AudioInstrumentParameterPanel::slotSynthGUIButtonClicked()
{
    emit showPluginGUI(getSelectedInstrument()->getId(),
                       Instrument::SYNTH_PLUGIN_POSITION);
}

void
AudioInstrumentParameterPanel::slotSetPan(float pan)
{
    RG_DEBUG << "AudioInstrumentParameterPanel::slotSetPan - "
    << "pan = " << pan << endl;

    StudioControl::setStudioObjectProperty
    (MappedObjectId(getSelectedInstrument()->getMappedId()),
     MappedAudioFader::Pan,
     MappedObjectValue(pan));

    getSelectedInstrument()->setPan(MidiByte(pan + 100.0));
    getSelectedInstrument()->changed();
}

void
AudioInstrumentParameterPanel::setAudioMeter(float dBleft, float dBright,
        float recDBleft, float recDBright)
{
    //    RG_DEBUG << "AudioInstrumentParameterPanel::setAudioMeter: (" << dBleft
    //             << "," << dBright << ")" << endl;

    if (getSelectedInstrument()) {
        // Always set stereo, because we have to reflect what's happening
        // with the pan setting even on mono tracks
        m_audioFader.m_vuMeter->setLevel(dBleft, dBright);
        m_audioFader.m_vuMeter->setRecordLevel(recDBleft, recDBright);
    }
}

void
AudioInstrumentParameterPanel::setupForInstrument(Instrument* instrument)
{
    blockSignals(true);

    RG_DEBUG << "AudioInstrumentParameterPanel[" << this << "]::setupForInstrument(" << instrument << ")" << endl;

    QString l = QString::fromStdString(instrument->getAlias());
    if (l.isEmpty()) l = instrument->getLocalizedPresentationName();

    setSelectedInstrument(instrument);
    m_instrumentLabel.setText(l);

    m_aliasButton.setInstrument(instrument);

    m_audioFader.m_recordFader->setFader(instrument->getRecordLevel());
    m_audioFader.m_fader->setFader(instrument->getLevel());

    m_audioFader.slotSetInstrument(&m_doc->getStudio(), instrument);

    int start = 0;

    if (instrument->getType() == Instrument::SoftSynth)
        start = -1;

    for (int i = start; i < int(m_audioFader.m_plugins.size()); i++) {
        int index;
        PluginPushButton *button;
        QString noneText;

        if (i == -1) {
            index = Instrument::SYNTH_PLUGIN_POSITION;
            button = m_audioFader.m_synthButton;
            noneText = tr("<no synth>");
        } else {
            index = i;
            button = m_audioFader.m_plugins[i];
            noneText = tr("<no plugin>");
        }

        button->show();

        AudioPluginInstance *inst = instrument->getPlugin(index);

        if (inst && inst->isAssigned()) {
            AudioPlugin *pluginClass
            = m_doc->getPluginManager()->getPlugin(
                  m_doc->getPluginManager()->
                  getPositionByIdentifier(inst->getIdentifier().c_str()));

            if (pluginClass) {
                button->setText(pluginClass->getLabel());
                button->setToolTip(pluginClass->getLabel());
                setButtonColour(index, inst->isBypassed(),
                                pluginClass->getColour());
            }
        } else {
            button->setText(noneText);
            button->setToolTip(noneText);
            setButtonColour(index, inst ? inst->isBypassed() : false, QColor(Qt::black));
        }
    }

    // Set the number of channels on the fader widget
    //
    m_audioFader.setAudioChannels(instrument->getAudioChannels());

    // Pan - adjusted backwards
    //
    m_audioFader.m_pan->setPosition(instrument->getPan() - 100);

    // Tell fader box whether to include e.g. audio input selection
    //
    m_audioFader.setIsSynth(instrument->getType() == Instrument::SoftSynth);

    blockSignals(false);
}

void
AudioInstrumentParameterPanel::slotAudioChannels(int channels)
{
    RG_DEBUG << "AudioInstrumentParameterPanel::slotAudioChannels - "
    << "channels = " << channels << endl;

    getSelectedInstrument()->setAudioChannels(channels);
    getSelectedInstrument()->changed();

    StudioControl::setStudioObjectProperty
    (MappedObjectId(getSelectedInstrument()->getMappedId()),
     MappedAudioFader::Channels,
     MappedObjectValue(channels));

}

void
AudioInstrumentParameterPanel::slotSelectPlugin(int index)
{
    if (getSelectedInstrument()) {
        emit selectPlugin(0, getSelectedInstrument()->getId(), index);
    }
}

void
AudioInstrumentParameterPanel::slotInstrumentChanged(Instrument *instrument)
{
    if (!instrument)
        return;

    if (!getSelectedInstrument())
        return;

    // If this isn't a change for the Instrument we are displaying, bail.
    if (getSelectedInstrument()->getId() != instrument->getId())
        return;

    // Update the parameters on the widgets
    setupForInstrument(instrument);
}


}
