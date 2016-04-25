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

#define RG_MODULE_STRING "[AudioFaderBox]"

#include "AudioFaderBox.h"

#include "misc/Debug.h"
#include "AudioRouteMenu.h"
#include "AudioVUMeter.h"
#include "base/AudioLevel.h"
#include "base/Instrument.h"
#include "base/Studio.h"
#include "Fader.h"
#include "gui/general/GUIPalette.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/studio/AudioPluginOSCGUIManager.h"
#include "Rotary.h"
#include "gui/general/IconLoader.h"
#include "VUMeter.h"
#include "gui/widgets/PluginPushButton.h"

#include <QFrame>
#include <QLabel>
#include <QObject>
#include <QPixmap>
#include <QIcon>
#include <QPushButton>
#include <QSignalMapper>
#include <QString>
#include <QToolTip>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLayout>
#include <QDir>


namespace Rosegarden
{

AudioFaderBox::AudioFaderBox(QWidget *parent,
                             QString id,
                             bool haveInOut,
                             const char *name):
        QFrame(parent),
        m_signalMapper(new QSignalMapper(this)),
        m_id(id),
        m_isStereo(false)
{
    setObjectName(name);

    // Plugin box
    //
    PluginPushButton *plugin;
    QWidget *pluginVbox = 0;

    pluginVbox = new QWidget(this);
    QVBoxLayout *pluginVboxLayout = new QVBoxLayout;
    pluginVboxLayout->setSpacing(2);
    pluginVboxLayout->setMargin(0);

    for (int i = 0; i < 5; i++) {
        plugin = new PluginPushButton(pluginVbox);
        pluginVboxLayout->addWidget(plugin);
        plugin->setText(tr("<no plugin>"));

        plugin->setToolTip(tr("Click to load an audio plugin"));

        m_plugins.push_back(plugin);
        m_signalMapper->setMapping(plugin, i);
        connect(plugin, SIGNAL(clicked()),
                m_signalMapper, SLOT(map()));
    }
    pluginVbox->setLayout(pluginVboxLayout);

    m_synthButton = new PluginPushButton(this);
    m_synthButton->setText(tr("<no synth>"));
    m_synthButton->setToolTip(tr("Click to load a synth plugin for playing MIDI"));

    // VU meter and fader
    //
    QWidget *faderHbox = new QWidget(this);
    QHBoxLayout *faderHboxLayout = new QHBoxLayout;
    faderHboxLayout->setMargin(0);

    //@@@ Testing m_vuMeter->height() doesn't work until the meter has
    //actually been shown, in Qt4 -- hardcode this for now
    int vuHeight = 140;

/*@@@
    m_vuMeter = new AudioVUMeter( faderHbox, VUMeter::AudioPeakHoldShort,
            true, true);

    faderHboxLayout->addWidget(m_vuMeter);
*/
    m_recordFader = new Fader(AudioLevel::ShortFader, 20,
                              vuHeight, faderHbox);

    faderHboxLayout->addWidget(m_recordFader);

    m_recordFader->setOutlineColour(GUIPalette::getColour(GUIPalette::RecordFaderOutline));
/*@@@
    delete m_vuMeter; // only used the first one to establish height,
    // actually want it after the record fader in
    // hbox
    */
    m_vuMeter = new AudioVUMeter(faderHbox, VUMeter::AudioPeakHoldShort,
            true, true);

    faderHboxLayout->addWidget(m_vuMeter);

    m_fader = new Fader(AudioLevel::ShortFader, 20,
                        vuHeight, faderHbox);

    faderHboxLayout->addWidget(m_fader);
    faderHbox->setLayout(faderHboxLayout);

    m_fader->setOutlineColour(GUIPalette::getColour(GUIPalette::PlaybackFaderOutline));

    m_monoPixmap = IconLoader().loadPixmap("mono");
    m_stereoPixmap = IconLoader().loadPixmap("stereo");

    m_pan = new Rotary(this, -100.0, 100.0, 1.0, 5.0, 0.0, 22,
                       Rotary::NoTicks, false, true);

    // same as the knob colour on the MIDI pan
    m_pan->setKnobColour(GUIPalette::getColour(GUIPalette::RotaryPastelGreen));

    m_stereoButton = new QPushButton(this);
    m_stereoButton->setIcon(QIcon(m_monoPixmap)); // default is mono
    m_stereoButton->setFixedSize(24, 24);

    connect(m_stereoButton, SIGNAL(clicked()),
            this, SLOT(slotChannelStateChanged()));

    m_synthGUIButton = new QPushButton(this);
    m_synthGUIButton->setText(tr("Editor"));

    if (haveInOut) {
        m_audioInput = new AudioRouteMenu(this,
                                          AudioRouteMenu::In,
                                          AudioRouteMenu::Regular);
        m_audioOutput = new AudioRouteMenu(this,
                                           AudioRouteMenu::Out,
                                           AudioRouteMenu::Regular);

    } else {
        m_pan->setKnobColour(GUIPalette::getColour(GUIPalette::RotaryPastelOrange));

        m_audioInput = 0;
        m_audioOutput = 0;
    }

    m_pan->setToolTip(tr("Set the audio pan position in the stereo field"));
    m_synthGUIButton->setToolTip(tr("Open the synth plugin's native editor"));
    m_stereoButton->setToolTip(tr("Mono or Stereo Instrument"));
    m_recordFader->setToolTip(tr("Record level"));
    m_fader->setToolTip(tr("Playback level"));
    m_vuMeter->setToolTip(tr("Audio level"));

    setContentsMargins(4, 4, 4, 4);
    QGridLayout *grid = new QGridLayout(this);
    setLayout(grid);
    grid->setMargin(0);
    grid->setSpacing(4);

    grid->addWidget(m_synthButton, 0, 0, 1, 3);

    if (haveInOut) {
        m_inputLabel = new QLabel(tr("In:"), this);
        grid->addWidget(m_inputLabel, 0, 0, Qt::AlignRight);
        grid->addWidget(m_audioInput->getWidget(), 0, 1, 1, 2);
        // force "In 1 L" to show full width:
        grid->setColumnStretch(1, 40);
        m_outputLabel = new QLabel(tr("Out:"), this);
        grid->addWidget(m_outputLabel, 0, 3, Qt::AlignRight);
        grid->addWidget(m_audioOutput->getWidget(), 0, 4, 1, 2);
    }
    grid->addWidget(pluginVbox, 2, 0, 0+1, 2- 0+1);
    grid->addWidget(faderHbox, 1, 3, 1+1, 5- 3+1);

    grid->addWidget(m_synthGUIButton, 1, 0);
    grid->addWidget(m_pan, 1, 2);
    grid->addWidget(m_stereoButton, 1, 1);
/*&&&
    for (int i = 0; i < 5; ++i) {
        // Force width
        m_plugins[i]->setFixedWidth(m_plugins[i]->sizeHint().width());
    }
    m_synthButton->setFixedWidth(m_plugins[0]->sizeHint().width());
*/
    m_synthButton->hide();
    m_synthGUIButton->hide();
}

void
AudioFaderBox::setIsSynth(bool isSynth)
{
    if (isSynth) {
        m_inputLabel->hide();
        m_synthButton->show();
        m_synthGUIButton->show();
        m_audioInput->getWidget()->hide();
        m_recordFader->hide();
    } else {
        m_synthButton->hide();
        m_synthGUIButton->hide();
        m_inputLabel->show();
        m_audioInput->getWidget()->show();
        m_recordFader->show();
    }
}

void
AudioFaderBox::slotSetInstrument(Studio *studio,
                                 Instrument *instrument)
{
    if (m_audioInput)
        m_audioInput->slotSetInstrument(studio, instrument);
    if (m_audioOutput)
        m_audioOutput->slotSetInstrument(studio, instrument);
    if (instrument)
        setAudioChannels(instrument->getAudioChannels());
    if (instrument) {

        RG_DEBUG << "AudioFaderBox::slotSetInstrument(" << instrument->getId() << ")";

        setIsSynth(instrument->getType() == Instrument::SoftSynth);
        if (instrument->getType() == Instrument::SoftSynth) {
            bool gui = false;
            RG_DEBUG << "AudioFaderBox::slotSetInstrument(" << instrument->getId() << "): is soft synth";

            gui = RosegardenMainWindow::self()->getPluginGUIManager()->hasGUI
                  (instrument->getId(), Instrument::SYNTH_PLUGIN_POSITION);
            RG_DEBUG << "AudioFaderBox::slotSetInstrument(" << instrument->getId() << "): has gui = " << gui;

            m_synthGUIButton->setEnabled(gui);
        }
    }
}

bool
AudioFaderBox::owns(const QObject *object)
{
    return (object &&
            ((object->parent() == this) ||
             (object->parent() && (object->parent()->parent() == this))));
}

void
AudioFaderBox::setAudioChannels(int channels)
{
    m_isStereo = (channels > 1);

    switch (channels) {
    case 1:
        if (m_stereoButton)
            m_stereoButton->setIcon(QIcon(m_monoPixmap));
        m_isStereo = false;
        break;

    case 2:
        if (m_stereoButton)
            m_stereoButton->setIcon(QIcon(m_stereoPixmap));
        m_isStereo = true;
        break;
    default:
        RG_DEBUG << "AudioFaderBox::setAudioChannels - "
        << "unsupported channel numbers (" << channels
        << ")" << endl;
        return ;
    }

    if (m_audioInput)
        m_audioInput->slotRepopulate();
    if (m_audioOutput)
        m_audioOutput->slotRepopulate();
}

void
AudioFaderBox::slotChannelStateChanged()
{
    if (m_isStereo) {
        setAudioChannels(1);
        emit audioChannelsChanged(1);
    } else {
        setAudioChannels(2);
        emit audioChannelsChanged(2);
    }
}

void
AudioFaderBox::setFont(QFont f)
{
    m_synthButton->setFont(f);
    for (size_t i = 0; i < m_plugins.size(); ++i) m_plugins[i]->setFont(f);
    m_vuMeter->setFont(f);
    m_inputLabel->setFont(f);
    m_outputLabel->setFont(f);
    m_audioInput->getWidget()->setFont(f);
    m_audioOutput->getWidget()->setFont(f);
    m_synthGUIButton->setFont(f);
}


}
