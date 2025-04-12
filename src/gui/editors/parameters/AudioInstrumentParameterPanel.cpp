/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

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
#include "gui/widgets/InputDialog.h"
#include "gui/widgets/Rotary.h"
#include "gui/widgets/SqueezedLabel.h"
#include "InstrumentParameterPanel.h"
#include "sound/MappedCommon.h"
#include "sound/MappedStudio.h"
#include "sound/Midi.h"
#include "gui/widgets/PluginPushButton.h"
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

    // Instrument label

    // Adjust the background color to make it look like an edit box.
    m_instrumentLabel->setAutoFillBackground(true);
    QPalette palette = m_instrumentLabel->palette();
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Window, QColor(0x70, 0x70, 0x70));
    m_instrumentLabel->setPalette(palette);

    m_instrumentLabel->setFont(font);
    m_instrumentLabel->setAlignment(Qt::AlignCenter);
    // Normally, SqueezedLabel hijacks the tooltip.  This tells it not to.
    m_instrumentLabel->allowToolTip();
    m_instrumentLabel->setToolTip(tr("Click to rename this instrument."));
    // No tr(); temporary internal debugging string.
    m_instrumentLabel->setText("REFRESH BUG!");
    connect(m_instrumentLabel, &SqueezedLabel::clicked,
            this, &AudioInstrumentParameterPanel::slotLabelClicked);

    // Audio Fader Box
    m_audioFader = new AudioFaderBox(this);
    m_audioFader->setFont(font);
    connect(m_audioFader, &AudioFaderBox::audioChannelsChanged,
            this, &AudioInstrumentParameterPanel::slotAudioChannels);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    connect(m_audioFader->m_signalMapper, &QSignalMapper::mappedInt,
            this, &AudioInstrumentParameterPanel::slotSelectPlugin);
#else
    connect(m_audioFader->m_signalMapper, SIGNAL(mapped(int)),
            this, SLOT(slotSelectPlugin(int)));
#endif
    connect(m_audioFader->m_fader, &Fader::faderChanged,
            this, &AudioInstrumentParameterPanel::slotSelectAudioLevel);
    connect(m_audioFader->m_recordFader, &Fader::faderChanged,
            this, &AudioInstrumentParameterPanel::slotSelectAudioRecordLevel);
    connect(m_audioFader->m_pan, &Rotary::valueChanged,
            this, &AudioInstrumentParameterPanel::slotSetPan);
    connect(m_audioFader->m_synthButton, &QAbstractButton::clicked,
            this, &AudioInstrumentParameterPanel::slotSynthButtonClicked);
    connect(m_audioFader->m_synthGUIButton, &QAbstractButton::clicked,
            this, &AudioInstrumentParameterPanel::slotSynthGUIButtonClicked);

    // Layout

    QGridLayout *gridLayout = new QGridLayout(this);
    gridLayout->setSpacing(5);
    gridLayout->setContentsMargins(0, 0, 0, 0);

    gridLayout->addWidget(m_instrumentLabel, 0, 0);
    gridLayout->addWidget(m_audioFader, 1, 0);

    // Make row 2 fill up the rest of the space.
    gridLayout->setRowStretch(2, 1);

    // Panel

    setLayout(gridLayout);
    setContentsMargins(5, 7, 5, 2);

    // Connections

    connect(RosegardenMainWindow::self(),
                &RosegardenMainWindow::documentLoaded,
            this, &AudioInstrumentParameterPanel::slotDocumentLoaded);

    // Connect for high-frequency control change notifications.
    connect(Instrument::getStaticSignals().data(),
                &InstrumentStaticSignals::controlChange,
            this, &AudioInstrumentParameterPanel::slotControlChange);

    connect(RosegardenMainWindow::self(),
                &RosegardenMainWindow::pluginSelected,
            this, &AudioInstrumentParameterPanel::slotPluginSelected);

    connect(RosegardenMainWindow::self(),
                &RosegardenMainWindow::pluginBypassed,
            this, &AudioInstrumentParameterPanel::slotPluginBypassed);
}

void
AudioInstrumentParameterPanel::slotSelectAudioLevel(float dB)
{
    if (getSelectedInstrument() == nullptr)
        return ;

    if (getSelectedInstrument()->getType() == Instrument::Audio ||
            getSelectedInstrument()->getType() == Instrument::SoftSynth) {
        getSelectedInstrument()->setLevel(dB);
        Instrument::emitControlChange(getSelectedInstrument(),
                                      MIDI_CONTROLLER_VOLUME);
        RosegardenDocument::currentDocument->setModified();
    }
}

void
AudioInstrumentParameterPanel::slotSelectAudioRecordLevel(float dB)
{
    if (getSelectedInstrument() == nullptr)
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
        RosegardenDocument::currentDocument->slotDocumentModified();

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

    PluginPushButton *button = nullptr;
    QString noneText;

    // updates synth gui button &c:
    m_audioFader->slotSetInstrument(
            &(RosegardenDocument::currentDocument->getStudio()),
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

        QSharedPointer<AudioPlugin> pluginClass =
                RosegardenDocument::currentDocument->
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
        QSharedPointer<AudioPluginManager> pluginMgr =
                RosegardenDocument::currentDocument->getPluginManager();
        QSharedPointer<AudioPlugin> pluginClass = pluginMgr->getPlugin(
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

    PluginPushButton *button = nullptr;

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
    RosegardenDocument::currentDocument->setModified();
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

    QString alias = QString::fromStdString(instrument->getAlias());
    if (alias.isEmpty()) alias = instrument->getLocalizedPresentationName();

    setSelectedInstrument(instrument);

    m_instrumentLabel->setText(alias);

    m_audioFader->m_recordFader->setFader(instrument->getRecordLevel());
    m_audioFader->m_fader->setFader(instrument->getLevel());

    m_audioFader->slotSetInstrument(
            &(RosegardenDocument::currentDocument->getStudio()),
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
            QSharedPointer<AudioPluginManager> pluginMgr =
                    RosegardenDocument::currentDocument->
                        getPluginManager();
            QSharedPointer<AudioPlugin> pluginClass = pluginMgr->getPlugin(
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
    m_audioFader->setAudioChannels(instrument->getNumAudioChannels());

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

    getSelectedInstrument()->setNumAudioChannels(channels);
    RosegardenDocument::currentDocument->slotDocumentModified();

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
                nullptr, getSelectedInstrument()->getId(), index);
    }
}

void
AudioInstrumentParameterPanel::slotDocumentLoaded(RosegardenDocument *doc)
{
    connect(doc, &RosegardenDocument::documentModified,
            this, &AudioInstrumentParameterPanel::slotDocumentModified);
}

void
AudioInstrumentParameterPanel::slotDocumentModified(bool)
{
    RosegardenDocument *doc = RosegardenDocument::currentDocument;

    // Get the selected Track's Instrument.
    InstrumentId instrumentId =
            doc->getComposition().getSelectedInstrumentId();

    Instrument *instrument = nullptr;

    // If an instrument has been selected.
    if (instrumentId != NoInstrument)
        instrument = doc->getStudio().getInstrumentById(instrumentId);

    if (!instrument) {
        setSelectedInstrument(nullptr);
        return;
    }

    if (instrument->getType() != Instrument::Audio  &&
        instrument->getType() != Instrument::SoftSynth) {
        setSelectedInstrument(nullptr);
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

void AudioInstrumentParameterPanel::slotLabelClicked()
{
    const QString oldAlias = m_instrumentLabel->text();
    bool ok = false;

    QString newAlias = InputDialog::getText(
            this,  // parent
            tr("Rosegarden"),  // title
            tr("Enter instrument alias:"),  // label
            LineEdit::Normal,  // mode (echo)
            oldAlias,  // text
            &ok);  // ok

    // Cancelled?  Bail.
    if (!ok)
        return;

    // No change?  Bail.
    if (newAlias == oldAlias)
        return;

    RosegardenDocument *doc = RosegardenDocument::currentDocument;

    // Get the selected Track's Instrument.
    InstrumentId instrumentId =
            doc->getComposition().getSelectedInstrumentId();

    Instrument *instrument = nullptr;

    // If an instrument has been selected.
    if (instrumentId != NoInstrument)
        instrument = doc->getStudio().getInstrumentById(instrumentId);

    if (!instrument)
        return;

    // ??? A command would be better.  Then the user can undo.
    //     See SegmentLabelCommand.
    instrument->setAlias(newAlias.toStdString());

    doc->slotDocumentModified();
}


}
