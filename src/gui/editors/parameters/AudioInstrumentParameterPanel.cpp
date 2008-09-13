/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "AudioInstrumentParameterPanel.h"
#include <QLayout>
#include <QApplication>

#include <klocale.h>
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/AudioPluginInstance.h"
#include "base/Instrument.h"
#include "base/MidiProgram.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/studio/AudioPluginManager.h"
#include "gui/studio/AudioPlugin.h"
#include "gui/studio/StudioControl.h"
#include "gui/widgets/AudioFaderBox.h"
#include "gui/widgets/AudioVUMeter.h"
#include "gui/widgets/Fader.h"
#include "gui/widgets/Rotary.h"
#include "gui/widgets/AudioRouteMenu.h"
#include "InstrumentParameterPanel.h"
#include "sound/MappedCommon.h"
#include "sound/MappedStudio.h"
#include <QColor>
#include <QFrame>
#include <QLabel>
#include <QPalette>
#include <QPixmap>
#include <QPushButton>
#include <QString>
#include <QToolTip>
#include <QWidget>
#include <QSignalMapper>


namespace Rosegarden
{

void
AudioInstrumentParameterPanel::slotSelectAudioLevel(float dB)
{
    if (m_selectedInstrument == 0)
        return ;

    if (m_selectedInstrument->getType() == Instrument::Audio ||
            m_selectedInstrument->getType() == Instrument::SoftSynth) {
        m_selectedInstrument->setLevel(dB);

        StudioControl::setStudioObjectProperty
        (MappedObjectId(m_selectedInstrument->getMappedId()),
         MappedAudioFader::FaderLevel,
         MappedObjectValue(dB));
    }

    emit updateAllBoxes();
    emit instrumentParametersChanged(m_selectedInstrument->getId());
}

void
AudioInstrumentParameterPanel::slotSelectAudioRecordLevel(float dB)
{
    if (m_selectedInstrument == 0)
        return ;

    //    std::cerr << "AudioInstrumentParameterPanel::slotSelectAudioRecordLevel("
    //	      << dB << ")" << std::endl;

    if (m_selectedInstrument->getType() == Instrument::Audio) {
        m_selectedInstrument->setRecordLevel(dB);

        StudioControl::setStudioObjectProperty
        (MappedObjectId(m_selectedInstrument->getMappedId()),
         MappedAudioFader::FaderRecordLevel,
         MappedObjectValue(dB));

        emit updateAllBoxes();
        emit instrumentParametersChanged(m_selectedInstrument->getId());
    }
}

void
AudioInstrumentParameterPanel::slotPluginSelected(InstrumentId instrumentId,
        int index, int plugin)
{
    if (!m_selectedInstrument ||
            instrumentId != m_selectedInstrument->getId())
        return ;

    RG_DEBUG << "AudioInstrumentParameterPanel::slotPluginSelected - "
    << "instrument = " << instrumentId
    << ", index = " << index
    << ", plugin = " << plugin << endl;

    QColor pluginBackgroundColour = QColor(Qt::black);
    bool bypassed = false;

    QPushButton *button = 0;
    QString noneText;

    // updates synth gui button &c:
    m_audioFader->slotSetInstrument(&m_doc->getStudio(), m_selectedInstrument);

    if (index == (int)Instrument::SYNTH_PLUGIN_POSITION) {
        button = m_audioFader->m_synthButton;
        noneText = i18n("<no synth>");
    } else {
        button = m_audioFader->m_plugins[index];
        noneText = i18n("<no plugin>");
    }

    if (!button)
        return ;

    if (plugin == -1) {

        button->setText(noneText);
        QToolTip::add
            (button, noneText);

    } else {

        AudioPlugin *pluginClass = m_doc->getPluginManager()->getPlugin(plugin);

        if (pluginClass) {
            button->setText(pluginClass->getLabel());

            QToolTip::add
                (button, pluginClass->getLabel());

            pluginBackgroundColour = pluginClass->getColour();
        }
    }

    AudioPluginInstance *inst =
        m_selectedInstrument->getPlugin(index);

    if (inst)
        bypassed = inst->isBypassed();

    setButtonColour(index, bypassed, pluginBackgroundColour);

    if (index == (int)Instrument::SYNTH_PLUGIN_POSITION) {
        emit changeInstrumentLabel(instrumentId, button->text());
    }
}

void
AudioInstrumentParameterPanel::slotPluginBypassed(InstrumentId instrumentId,
        int pluginIndex, bool bp)
{
    if (!m_selectedInstrument ||
            instrumentId != m_selectedInstrument->getId())
        return ;

    AudioPluginInstance *inst =
        m_selectedInstrument->getPlugin(pluginIndex);

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

    QPushButton *button = 0;

    if (pluginIndex == Instrument::SYNTH_PLUGIN_POSITION) {
        button = m_audioFader->m_synthButton;
    } else {
        button = m_audioFader->m_plugins[pluginIndex];
    }

    if (!button)
        return ;

    // Set the bypass colour on the plugin button
    if (bypassState) {
        button->
        setPaletteForegroundColor(qApp->palette().
                                  color(QPalette::Active, QColorGroup::Button));

        button->
        setPaletteBackgroundColor(qApp->palette().
                                  color(QPalette::Active, QColorGroup::ButtonText));
    } else if (colour == QColor(Qt::black)) {
        button->
        setPaletteForegroundColor(qApp->palette().
                                  color(QPalette::Active, QColorGroup::ButtonText));

        button->
        setPaletteBackgroundColor(qApp->palette().
                                  color(QPalette::Active, QColorGroup::Button));
    } else {
        button->
        setPaletteForegroundColor(QColor(Qt::white));

        button->
        setPaletteBackgroundColor(colour);
    }
}

AudioInstrumentParameterPanel::AudioInstrumentParameterPanel(RosegardenGUIDoc* doc, QWidget* parent)
        : InstrumentParameterPanel(doc, parent),
        m_audioFader(new AudioFaderBox(this))
{
    QGridLayout *gridLayout = new QGridLayout(this, 3, 2, 5, 5);

    // Instrument label : first row, all cols
    gridLayout->addWidget(m_instrumentLabel, 0, 0, 0- 0+1, 1- 1, Qt::AlignCenter);

    // fader and connect it
    gridLayout->addWidget(m_audioFader, 1, 0, 0+1, 1- 1);

    gridLayout->setRowStretch(2, 1);

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

    connect(m_audioFader->m_audioOutput, SIGNAL(changed()),
            this, SLOT(slotAudioRoutingChanged()));

    connect(m_audioFader->m_audioInput, SIGNAL(changed()),
            this, SLOT(slotAudioRoutingChanged()));

    connect(m_audioFader->m_synthButton, SIGNAL(clicked()),
            this, SLOT(slotSynthButtonClicked()));

    connect(m_audioFader->m_synthGUIButton, SIGNAL(clicked()),
            this, SLOT(slotSynthGUIButtonClicked()));
}

void
AudioInstrumentParameterPanel::slotSynthButtonClicked()
{
    slotSelectPlugin(Instrument::SYNTH_PLUGIN_POSITION);
}

void
AudioInstrumentParameterPanel::slotSynthGUIButtonClicked()
{
    emit showPluginGUI(m_selectedInstrument->getId(),
                       Instrument::SYNTH_PLUGIN_POSITION);
}

void
AudioInstrumentParameterPanel::slotSetPan(float pan)
{
    RG_DEBUG << "AudioInstrumentParameterPanel::slotSetPan - "
    << "pan = " << pan << endl;

    StudioControl::setStudioObjectProperty
    (MappedObjectId(m_selectedInstrument->getMappedId()),
     MappedAudioFader::Pan,
     MappedObjectValue(pan));

    m_selectedInstrument->setPan(MidiByte(pan + 100.0));
    emit instrumentParametersChanged(m_selectedInstrument->getId());
}

void
AudioInstrumentParameterPanel::setAudioMeter(float dBleft, float dBright,
        float recDBleft, float recDBright)
{
    //    RG_DEBUG << "AudioInstrumentParameterPanel::setAudioMeter: (" << dBleft
    //	     << "," << dBright << ")" << endl;

    if (m_selectedInstrument) {
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

    m_selectedInstrument = instrument;

    m_instrumentLabel->setText(strtoqstr(instrument->getName()));

    m_audioFader->m_recordFader->setFader(instrument->getRecordLevel());
    m_audioFader->m_fader->setFader(instrument->getLevel());

    m_audioFader->slotSetInstrument(&m_doc->getStudio(), instrument);

    int start = 0;

    if (instrument->getType() == Instrument::SoftSynth)
        start = -1;

    for (int i = start; i < int(m_audioFader->m_plugins.size()); i++) {
        int index;
        QPushButton *button;
        QString noneText;

        if (i == -1) {
            index = Instrument::SYNTH_PLUGIN_POSITION;
            button = m_audioFader->m_synthButton;
            noneText = i18n("<no synth>");
        } else {
            index = i;
            button = m_audioFader->m_plugins[i];
            noneText = i18n("<no plugin>");
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
                QToolTip::add
                    (button, pluginClass->getLabel());
                setButtonColour(index, inst->isBypassed(),
                                pluginClass->getColour());
            }
        } else {
            button->setText(noneText);
            QToolTip::add
                (button, noneText);
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
    RG_DEBUG << "AudioInstrumentParameterPanel::slotAudioChannels - "
    << "channels = " << channels << endl;

    m_selectedInstrument->setAudioChannels(channels);

    StudioControl::setStudioObjectProperty
    (MappedObjectId(m_selectedInstrument->getMappedId()),
     MappedAudioFader::Channels,
     MappedObjectValue(channels));

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
    if (m_selectedInstrument) {
        emit selectPlugin(0, m_selectedInstrument->getId(), index);
    }
}

}
#include "AudioInstrumentParameterPanel.moc"
