/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
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
#include "gui/application/RosegardenMainWindow.h"
#include "gui/studio/AudioPluginManager.h"
#include "gui/studio/AudioPlugin.h"
#include "gui/studio/StudioControl.h"
#include "gui/widgets/AudioVUMeter.h"
#include "gui/widgets/Fader.h"
#include "gui/widgets/Rotary.h"
#include "gui/widgets/SqueezedLabel.h"
#include "InstrumentParameterPanel.h"
#include "sound/MappedCommon.h"
#include "sound/MappedStudio.h"
#include "sound/Midi.h"
#include "gui/widgets/PluginPushButton.h"
#include "gui/widgets/InstrumentAliasButton.h"
#include "gui/widgets/AudioFaderBox.h"

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

AudioInstrumentParameterPanel::AudioInstrumentParameterPanel(QWidget *parent) :
    InstrumentParameterPanel(parent)
{
    setObjectName("Audio Instrument Parameter Panel");

    QFont font;
    font.setPointSize(font.pointSize() * 90 / 100);
    font.setBold(false);

    // Widgets

    // Alias button
    // ??? Remove this alias button.  Instead, make the label clickable.
    //     Use AudioMixerWindow2 as a guide.  It allows changing of the
    //     Instrument alias by clicking on the Label.  SqueezedLabel will
    //     need to be upgraded to offer a clicked() signal like Label does.
    //     Once this is removed, remove InstrumentAliasButton from
    //     the sourcebase since this is the last user.
    m_aliasButton = new InstrumentAliasButton(this);
    m_aliasButton->setFixedSize(10, 6); // golden rectangle
    m_aliasButton->setToolTip(tr("Click to rename this instrument"));
    connect(m_aliasButton, SIGNAL(changed()),
            SLOT(slotAliasChanged()));

    // Instrument label
    QFontMetrics metrics(font);
    int width25 = metrics.width("1234567890123456789012345");
    m_instrumentLabel->setFont(font);
    m_instrumentLabel->setFixedWidth(width25);
    m_instrumentLabel->setAlignment(Qt::AlignCenter);
    m_instrumentLabel->setToolTip(tr("Click the button above to rename this instrument"));
    m_instrumentLabel->setText("REFRESH BUG!"); // no tr(); temporary internal string

    // Audio Fader Box
    m_audioFader = new AudioFaderBox(this);
    m_audioFader->setFont(font);
    connect(m_audioFader, SIGNAL(audioChannelsChanged(int)),
            this, SLOT(slotAudioChannels(int)));
    connect(m_audioFader->m_signalMapper, SIGNAL(mapped(int)),
            this, SLOT(slotSelectPlugin(int)));
    connect(m_audioFader->m_fader, SIGNAL(faderChanged(float)),
            this, SLOT(slotSelectAudioLevel(float)));
    connect(m_audioFader->m_recordFader, SIGNAL(faderChanged(float)),
            this, SLOT(slotSelectAudioRecordLevel(float)));
    connect(m_audioFader->m_pan, SIGNAL(valueChanged(float)),
            this, SLOT(slotSetPan(float)));
    connect(m_audioFader->m_synthButton, SIGNAL(clicked()),
            this, SLOT(slotSynthButtonClicked()));
    connect(m_audioFader->m_synthGUIButton, SIGNAL(clicked()),
            this, SLOT(slotSynthGUIButtonClicked()));

    // Layout

    QGridLayout *gridLayout = new QGridLayout(this);
    gridLayout->setSpacing(5);
    gridLayout->setMargin(0);
    // The alias button and the label are in 0,0.  The only difference is the
    // alignment.
    gridLayout->addWidget(m_aliasButton, 0, 0, 1, 2, Qt::AlignLeft);
    gridLayout->addWidget(m_instrumentLabel, 0, 0, 1, 2, Qt::AlignCenter);

    gridLayout->addWidget(m_audioFader, 1, 0, 1, 2);

    // Make row 2 fill up the rest of the space.
    gridLayout->setRowStretch(2, 1);

    // Panel

    setLayout(gridLayout);
    setContentsMargins(5, 7, 5, 2);

    // Connections

    connect(RosegardenMainWindow::self(),
                SIGNAL(documentChanged(RosegardenDocument *)),
            SLOT(slotNewDocument(RosegardenDocument *)));

    // Connect for high-frequency control change notifications.
    connect(Instrument::getStaticSignals().data(),
                SIGNAL(controlChange(Instrument *, int)),
            SLOT(slotControlChange(Instrument *, int)));

    connect(RosegardenMainWindow::self(),
                SIGNAL(pluginSelected(InstrumentId, int, int)),
            SLOT(slotPluginSelected(InstrumentId, int, int)));

    connect(RosegardenMainWindow::self(),
                SIGNAL(pluginBypassed(InstrumentId, int, bool)),
            SLOT(slotPluginBypassed(InstrumentId, int, bool)));
}

void
AudioInstrumentParameterPanel::slotSelectAudioLevel(float dB)
{
    if (getSelectedInstrument() == 0)
        return ;

    if (getSelectedInstrument()->getType() == Instrument::Audio ||
            getSelectedInstrument()->getType() == Instrument::SoftSynth) {
        getSelectedInstrument()->setLevel(dB);
        Instrument::emitControlChange(getSelectedInstrument(),
                                      MIDI_CONTROLLER_VOLUME);
        RosegardenMainWindow::self()->getDocument()->setModified();
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
        // ??? Another potential candidate for high-frequency update
        //     treatment like the controlChange() signal.  Since no one
        //     else seems to display this value, we could just set
        //     the doc modified flag without sending a notification.
        //     Or we could introduce a new high-frequency update
        //     notification for this and the only one receiving it
        //     would be whoever should do the setStudioObjectProperty()
        //     below.  See the controlChange() signal and handlers for
        //     more ideas.
        RosegardenMainWindow::self()->getDocument()->slotDocumentModified();

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

    //RG_DEBUG << "slotPluginSelected() - " << "instrument = " << instrumentId << ", index = " << index << ", plugin = " << plugin;

    QColor pluginBackgroundColour = QColor(Qt::black);
    bool bypassed = false;

    PluginPushButton *button = 0;
    QString noneText;

    // updates synth gui button &c:
    m_audioFader->slotSetInstrument(
            &(RosegardenMainWindow::self()->getDocument()->getStudio()),
            getSelectedInstrument());

    if (index == (int)Instrument::SYNTH_PLUGIN_POSITION) {
        button = m_audioFader->m_synthButton;
        noneText = tr("<no synth>");
    } else {
        button = m_audioFader->m_plugins[index];
        noneText = tr("<no plugin>");
    }

    if (!button)
        return ;

    if (plugin == -1) {

        button->setText(noneText);
        button->setToolTip(noneText);

    } else {

        AudioPlugin *pluginClass =
                RosegardenMainWindow::self()->getDocument()->
                    getPluginManager()->getPlugin(plugin);

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
}

void
AudioInstrumentParameterPanel::slotPluginBypassed(InstrumentId instrumentId,
        int pluginIndex, bool bp)
{
    if (!getSelectedInstrument() ||
            instrumentId != getSelectedInstrument()->getId())
        return ;

    //RG_DEBUG << "slotPluginBypassed()";

    AudioPluginInstance *inst =
        getSelectedInstrument()->getPlugin(pluginIndex);

    QColor backgroundColour = QColor(Qt::black); // default background colour

    if (inst && inst->isAssigned()) {
        AudioPluginManager *pluginMgr =
                RosegardenMainWindow::self()->getDocument()->getPluginManager();
        AudioPlugin *pluginClass = pluginMgr->getPlugin(
                pluginMgr->getPositionByIdentifier(
                        inst->getIdentifier().c_str()));

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
    //RG_DEBUG << "setButtonColour() " << "pluginIndex = " << pluginIndex << ", bypassState = " << bypassState << ", rgb = " << colour.name();

    PluginPushButton *button = 0;

    if (pluginIndex == int(Instrument::SYNTH_PLUGIN_POSITION)) {
        button = m_audioFader->m_synthButton;
    } else {
        button = m_audioFader->m_plugins[pluginIndex];
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
    //RG_DEBUG << "slotSynthButtonClicked()";

    slotSelectPlugin(Instrument::SYNTH_PLUGIN_POSITION);
}

void
AudioInstrumentParameterPanel::slotSynthGUIButtonClicked()
{
    //RG_DEBUG << "slotSynthGUIButtonClicked()";

    // Launch the Synth Plugin's parameter editor GUI.
    RosegardenMainWindow::self()->slotShowPluginGUI(
            getSelectedInstrument()->getId(),
            Instrument::SYNTH_PLUGIN_POSITION);
}

void
AudioInstrumentParameterPanel::slotSetPan(float pan)
{
    getSelectedInstrument()->setPan(MidiByte(pan + 100.0));
    Instrument::emitControlChange(getSelectedInstrument(),
                                  MIDI_CONTROLLER_PAN);
    RosegardenMainWindow::self()->getDocument()->setModified();
}

void
AudioInstrumentParameterPanel::setAudioMeter(float dBleft, float dBright,
        float recDBleft, float recDBright)
{
    //RG_DEBUG << "setAudioMeter: (" << dBleft << "," << dBright << ")";

    if (getSelectedInstrument()) {
        // Always set stereo, because we have to reflect what's happening
        // with the pan setting even on mono tracks
        m_audioFader->m_vuMeter->setLevel(dBleft, dBright);
        m_audioFader->m_vuMeter->setRecordLevel(recDBleft, recDBright);
    }
}

void
AudioInstrumentParameterPanel::setupForInstrument(Instrument* instrument)
{
    blockSignals(true);

    //RG_DEBUG << "this: " << this << " setupForInstrument(" << instrument << ")";

    QString l = QString::fromStdString(instrument->getAlias());
    if (l.isEmpty()) l = instrument->getLocalizedPresentationName();

    setSelectedInstrument(instrument);
    m_instrumentLabel->setText(l);

    m_aliasButton->setInstrument(instrument);

    m_audioFader->m_recordFader->setFader(instrument->getRecordLevel());
    m_audioFader->m_fader->setFader(instrument->getLevel());

    m_audioFader->slotSetInstrument(
            &(RosegardenMainWindow::self()->getDocument()->getStudio()),
            instrument);

    int start = 0;

    if (instrument->getType() == Instrument::SoftSynth)
        start = -1;

    for (int i = start; i < int(m_audioFader->m_plugins.size()); i++) {
        int index;
        PluginPushButton *button;
        QString noneText;

        if (i == -1) {
            index = Instrument::SYNTH_PLUGIN_POSITION;
            button = m_audioFader->m_synthButton;
            noneText = tr("<no synth>");
        } else {
            index = i;
            button = m_audioFader->m_plugins[i];
            noneText = tr("<no plugin>");
        }

        button->show();

        AudioPluginInstance *inst = instrument->getPlugin(index);

        if (inst && inst->isAssigned()) {
            AudioPluginManager *pluginMgr =
                    RosegardenMainWindow::self()->getDocument()->
                        getPluginManager();
            AudioPlugin *pluginClass = pluginMgr->getPlugin(
                    pluginMgr->getPositionByIdentifier(
                        inst->getIdentifier().c_str()));

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
    m_audioFader->setAudioChannels(instrument->getAudioChannels());

    // Pan - adjusted backwards
    //
    m_audioFader->m_pan->setPosition(instrument->getPan() - 100);

    // Tell fader box whether to include e.g. audio input selection
    //
    m_audioFader->setIsSynth(instrument->getType() == Instrument::SoftSynth);

    blockSignals(false);
}

void
AudioInstrumentParameterPanel::slotAudioChannels(int channels)
{
    //RG_DEBUG << "slotAudioChannels() " << "channels = " << channels;

    getSelectedInstrument()->setAudioChannels(channels);
    RosegardenMainWindow::self()->getDocument()->slotDocumentModified();

    StudioControl::setStudioObjectProperty
    (MappedObjectId(getSelectedInstrument()->getMappedId()),
     MappedAudioFader::Channels,
     MappedObjectValue(channels));

}

void
AudioInstrumentParameterPanel::slotSelectPlugin(int index)
{
    if (getSelectedInstrument()) {
        // Launch the plugin dialog.
        RosegardenMainWindow::self()->slotShowPluginDialog(
                0, getSelectedInstrument()->getId(), index);
    }
}

void
AudioInstrumentParameterPanel::slotAliasChanged()
{
    RosegardenMainWindow::self()->getDocument()->slotDocumentModified();

    // ??? This is wrong.  We should connect to the
    //     RosegardenDocument::documentModified() signal and refresh
    //     everything in response to it.
    setupForInstrument(getSelectedInstrument());
}

void
AudioInstrumentParameterPanel::slotNewDocument(RosegardenDocument *doc)
{
    connect(doc, SIGNAL(documentModified(bool)),
            SLOT(slotDocumentModified(bool)));
}

void
AudioInstrumentParameterPanel::slotDocumentModified(bool)
{
    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();

    // Get the selected Track's Instrument.
    InstrumentId instrumentId =
            doc->getComposition().getSelectedInstrumentId();

    Instrument *instrument = NULL;

    // If an instrument has been selected.
    if (instrumentId != NoInstrument)
        instrument = doc->getStudio().getInstrumentById(instrumentId);

    if (!instrument) {
        setSelectedInstrument(NULL);
        return;
    }

    if (instrument->getType() != Instrument::Audio  &&
        instrument->getType() != Instrument::SoftSynth) {
        setSelectedInstrument(NULL);
        return;
    }

    setSelectedInstrument(instrument);

    // Update the parameters on the widgets
    setupForInstrument(instrument);
}

void
AudioInstrumentParameterPanel::slotControlChange(Instrument *instrument, int cc)
{
    if (!instrument)
        return;

    if (!getSelectedInstrument())
        return;

    // If this isn't a change for the Instrument we are displaying, bail.
    if (getSelectedInstrument()->getId() != instrument->getId())
        return;

    // Just update the relevant cc widget.
    if (cc == MIDI_CONTROLLER_VOLUME) {

        // Update only the volume slider.
        m_audioFader->m_fader->setFader(instrument->getLevel());

    } else if (cc == MIDI_CONTROLLER_PAN) {

        // Update only the pan rotary.
        m_audioFader->m_pan->setPosition(instrument->getPan() - 100);

    }
}


}
