/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "AudioFaderBox.h"

#include <klocale.h>
#include <kstddirs.h>
#include "misc/Debug.h"
#include "AudioRouteMenu.h"
#include "AudioVUMeter.h"
#include "base/AudioLevel.h"
#include "base/Instrument.h"
#include "base/Studio.h"
#include "Fader.h"
#include "gui/general/GUIPalette.h"
#include "Rotary.h"
#include <kglobal.h>
#include <qframe.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qobject.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qsignalmapper.h>
#include <qstring.h>
#include <qtooltip.h>
#include <qvbox.h>
#include <qwidget.h>
#include "VUMeter.h"


namespace Rosegarden
{

AudioFaderBox::AudioFaderBox(QWidget *parent,
                             QString id,
                             bool haveInOut,
                             const char *name):
        QFrame(parent, name),
        m_signalMapper(new QSignalMapper(this)),
        m_id(id),
        m_isStereo(false)
{
    // Plugin box
    //
    QPushButton *plugin;
    QVBox *pluginVbox = 0;

    pluginVbox = new QVBox(this);
    pluginVbox->setSpacing(2);

    for (int i = 0; i < 5; i++) {
        plugin = new QPushButton(pluginVbox);
        plugin->setText(i18n("<no plugin>"));

        QToolTip::add
            (plugin, i18n("Audio plugin button"));

        m_plugins.push_back(plugin);
        m_signalMapper->setMapping(plugin, i);
        connect(plugin, SIGNAL(clicked()),
                m_signalMapper, SLOT(map()));
    }

    m_synthButton = new QPushButton(this);
    m_synthButton->setText(i18n("<no synth>"));
    QToolTip::add
        (m_synthButton, i18n("Synth plugin button"));

    // VU meter and fader
    //
    QHBox *faderHbox = new QHBox(this);

    m_vuMeter = new AudioVUMeter(faderHbox, VUMeter::AudioPeakHoldShort,
                                 true, true);

    m_recordFader = new Fader(AudioLevel::ShortFader,
                              20, m_vuMeter->height(), faderHbox);

    m_recordFader->setOutlineColour(GUIPalette::getColour(GUIPalette::RecordFaderOutline));

    delete m_vuMeter; // only used the first one to establish height,
    // actually want it after the record fader in
    // hbox
    m_vuMeter = new AudioVUMeter(faderHbox, VUMeter::AudioPeakHoldShort,
                                 true, true);

    m_fader = new Fader(AudioLevel::ShortFader,
                        20, m_vuMeter->height(), faderHbox);

    m_fader->setOutlineColour(GUIPalette::getColour(GUIPalette::PlaybackFaderOutline));

    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    m_monoPixmap.load(QString("%1/misc/mono.xpm").arg(pixmapDir));
    m_stereoPixmap.load(QString("%1/misc/stereo.xpm").arg(pixmapDir));

    m_pan = new Rotary(this, -100.0, 100.0, 1.0, 5.0, 0.0, 22,
                       Rotary::NoTicks, false, true);

    // same as the knob colour on the MIDI pan
    m_pan->setKnobColour(GUIPalette::getColour(GUIPalette::RotaryPastelGreen));

    m_stereoButton = new QPushButton(this);
    m_stereoButton->setPixmap(m_monoPixmap); // default is mono
    m_stereoButton->setFixedSize(24, 24);

    connect(m_stereoButton, SIGNAL(clicked()),
            this, SLOT(slotChannelStateChanged()));

    m_synthGUIButton = new QPushButton(this);
    m_synthGUIButton->setText(i18n("Editor"));

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

    QToolTip::add
        (m_pan, i18n("Set the audio pan position in the stereo field"));
    QToolTip::add
        (m_synthGUIButton, i18n("Open synth plugin's native editor"));
    QToolTip::add
        (m_stereoButton, i18n("Mono or Stereo Instrument"));
    QToolTip::add
        (m_recordFader, i18n("Record level"));
    QToolTip::add
        (m_fader, i18n("Playback level"));
    QToolTip::add
        (m_vuMeter, i18n("Audio level"));

    QGridLayout *grid = new QGridLayout(this, 3, 6, 4, 4);

    grid->addMultiCellWidget(m_synthButton, 0, 0, 0, 2);

    if (haveInOut) {
        m_inputLabel = new QLabel(i18n("In:"), this);
        grid->addWidget(m_inputLabel, 0, 0, AlignRight);
        grid->addMultiCellWidget(m_audioInput->getWidget(), 0, 0, 1, 2);
        m_outputLabel = new QLabel(i18n("Out:"), this);
        grid->addWidget(m_outputLabel, 0, 3, AlignRight);
        grid->addMultiCellWidget(m_audioOutput->getWidget(), 0, 0, 4, 5);
    }

    grid->addMultiCellWidget(pluginVbox, 2, 2, 0, 2);
    grid->addMultiCellWidget(faderHbox, 1, 2, 3, 5);

    grid->addWidget(m_synthGUIButton, 1, 0);
    grid->addWidget(m_pan, 1, 2);
    grid->addWidget(m_stereoButton, 1, 1);

    for (int i = 0; i < 5; ++i) {
        // Force width
        m_plugins[i]->setFixedWidth(m_plugins[i]->width());
    }
    m_synthButton->setFixedWidth(m_plugins[0]->width());

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

        RG_DEBUG << "AudioFaderBox::slotSetInstrument(" << instrument->getId() << ")" << endl;

        setIsSynth(instrument->getType() == Instrument::SoftSynth);
        if (instrument->getType() == Instrument::SoftSynth) {
            bool gui = false;
            RG_DEBUG << "AudioFaderBox::slotSetInstrument(" << instrument->getId() << "): is soft synth" << endl;
#ifdef HAVE_LIBLO

            gui = RosegardenGUIApp::self()->getPluginGUIManager()->hasGUI
                  (instrument->getId(), Instrument::SYNTH_PLUGIN_POSITION);
            RG_DEBUG << "AudioFaderBox::slotSetInstrument(" << instrument->getId() << "): has gui = " << gui << endl;
#endif

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
            m_stereoButton->setPixmap(m_monoPixmap);
        m_isStereo = false;
        break;

    case 2:
        if (m_stereoButton)
            m_stereoButton->setPixmap(m_stereoPixmap);
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

}
