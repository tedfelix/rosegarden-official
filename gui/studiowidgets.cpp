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

#include <kapp.h>
#include <klocale.h>
#include <kstddirs.h>
#include <kconfig.h>
#include <kcombobox.h>

#include <qlayout.h>
#include <qvbox.h>
#include <qtooltip.h>

#include "Instrument.h"
#include "studiowidgets.h"
#include "colours.h"
#include "constants.h"
#include "rosedebug.h"

// ---------------- AudioFaderWidget ------------------
//

AudioFaderWidget::AudioFaderWidget(QWidget *parent,
                                   const char *name,
                                   bool vertical):
    QWidget(parent, name),
    m_signalMapper(new QSignalMapper(this)),
    m_isStereo(false)
{
    // Plugin box
    //
    QPushButton *plugin;
    QVBox *pluginVbox = new QVBox(this);
    pluginVbox->setSpacing(2);

    for (int i = 0; i < 5; i++)
    {
        plugin = new QPushButton(pluginVbox);
        plugin->setText(i18n("<no plugin>"));

        // Force width
        plugin->setFixedWidth(plugin->width());
        QToolTip::add(plugin, i18n("Audio plugin button"));

        m_plugins.push_back(plugin);
        m_signalMapper->setMapping(plugin, i);
        connect(plugin, SIGNAL(clicked()),
                m_signalMapper, SLOT(map()));
    }

    // VU meter and fader
    //
    m_vuMeter = new AudioVUMeter(this);
    QToolTip::add(m_vuMeter, i18n("Audio VU Meter"));

    m_fader = new RosegardenFader(this);
    m_fader->setTickmarks(QSlider::Right);
    m_fader->setTickInterval(10);
    m_fader->setPageStep(10);
    m_fader->setMinValue(0);
    m_fader->setMaxValue(127);
    m_fader->setFixedHeight(m_vuMeter->height());
    QToolTip::add(m_fader, i18n("Audio Fader"));

    // Stereo, solo, mute and pan
    //
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    m_monoPixmap.load(QString("%1/misc/mono.xpm").arg(pixmapDir));
    m_stereoPixmap.load(QString("%1/misc/stereo.xpm").arg(pixmapDir));

    m_pan = new RosegardenRotary(this, -100.0, 100.0, 1.0, 5.0, 0.0, 24);
    QToolTip::add(m_pan,
                  i18n("Set the audio pan position in the stereo field"));

    // same as the knob colour on the MIDI pan
    m_pan->setKnobColour(RosegardenGUIColours::RotaryPastelGreen);

    m_stereoButton = new QPushButton(this);
    m_stereoButton->setPixmap(m_monoPixmap); // default is mono
    m_stereoButton->setFixedSize(24, 24);
    QToolTip::add(m_stereoButton, i18n("Mono or Stereo Audio Instrument"));

    connect(m_stereoButton, SIGNAL(clicked()),
            this, SLOT(slotChannelStateChanged()));

    m_muteButton = new QPushButton(this);
    m_muteButton->setText("M");
    m_muteButton->setToggleButton(true);

    QToolTip::add(m_muteButton, i18n("Mute the Track to which this Instrument is attached."));

    m_soloButton = new QPushButton(this);
    m_soloButton->setText("S");
    m_soloButton->setToggleButton(true);

    QToolTip::add(m_soloButton, i18n("Solo the Track to which this Instrument is attached."));

    m_recordButton = new QPushButton(this);
    m_recordButton->setText("R");
    m_recordButton->setToggleButton(true);

    QToolTip::add(m_recordButton,
                  i18n("Arm recording for this audio Instrument"));

    QLabel *inputLabel = new QLabel(i18n("Audio Input"), this);
    m_audioInput = new KComboBox(this);

    QLabel *panLabel = new QLabel(i18n("Pan"), this);
    
    // Sort out the layout accordingly
    //
    QGridLayout *grid;
   
    if (vertical == true)
    {
        grid = new QGridLayout(this, 7, 2, 6, 6);

        grid->addMultiCellWidget(pluginVbox, 0, 0, 0, 1, AlignCenter);

        grid->addWidget(m_vuMeter,           1, 0, AlignCenter);
        grid->addWidget(m_fader,             1, 1, AlignCenter);

        grid->addWidget(m_stereoButton,      2, 0, AlignCenter);
        grid->addWidget(m_pan,               2, 1, AlignCenter);

        grid->addWidget(m_muteButton,        3, 0, AlignCenter);
        grid->addWidget(m_soloButton,        3, 1, AlignCenter);

        grid->addWidget(m_recordButton,      4, 0, AlignCenter);

        //grid->addWidget(inputLabel,          5, 0, AlignCenter);
        grid->addWidget(m_audioInput,        4, 1, AlignCenter);
        //grid->addWidget(m_audioOutput,       5, 1, AlignCenter);
    }
    else
    {
        grid = new QGridLayout(this, 10, 5, 6, 10);

        grid->addMultiCellWidget(pluginVbox,    0, 8, 0, 1, AlignCenter);

        grid->addMultiCellWidget(m_vuMeter,     0, 8, 2, 2, AlignCenter);
        grid->addMultiCellWidget(m_fader,       0, 8, 3, 3, AlignCenter);

        //grid->addWidget(m_pan,                  1, 4, AlignCenter);

        grid->addWidget(m_muteButton,           2, 4, AlignCenter);
        grid->addWidget(m_soloButton,           3, 4, AlignCenter);

        grid->addWidget(m_recordButton,         4, 4, AlignCenter);
        grid->addWidget(m_stereoButton,         5, 4, AlignCenter);

        grid->addMultiCellWidget(panLabel,      8, 8, 0, 1, AlignCenter);
        grid->addMultiCellWidget(m_pan,         8, 8, 2, 4, AlignCenter);

        grid->addWidget(inputLabel,             9, 0, AlignCenter);
        grid->addMultiCellWidget(m_audioInput,  9, 9, 1, 4, AlignCenter);

        //grid->addWidget(outputLabel,            10, 0, AlignCenter);
        //grid->addMultiCellWidget(m_audioOutput, 10, 10, 1, 4, AlignCenter);
    }

}


void
AudioFaderWidget::setAudioChannels(int channels)
{
    switch (channels)
    {
        case 1:
            m_stereoButton->setPixmap(m_monoPixmap);
            m_isStereo = false;
            break;

        case 2:
            m_stereoButton->setPixmap(m_stereoPixmap);
            m_isStereo = true;
            break;
        default:
            RG_DEBUG << "AudioFaderWidget::setAudioChannels - "
                     << "unsupported channel numbers (" << channels
                     << ")" << std::endl;
	    return;
    }

    // Populate audio inputs accordingly
    //
    KConfig* config = kapp->config();
    config->setGroup(Rosegarden::SequencerOptionsConfigGroup);

    int jackAudioInputs = config->readNumEntry("jackaudioinputs", 2);
    QString inputName;

    // clear existing entries
    m_audioInput->clear();

    for (int i = 0; i < jackAudioInputs; i+= channels)
    {
        if (channels == 1)
            inputName = QString("Input %1").arg(i + 1);
        else
        {
            if ((i + 1) > jackAudioInputs) break;
            inputName = QString("Input %1-%2").arg(i + 1).arg(i + 2);
        }

        m_audioInput->insertItem(inputName);
    }
}

void
AudioFaderWidget::slotChannelStateChanged()
{
    if (m_isStereo)
    {
        setAudioChannels(1);
        emit audioChannelsChanged(1);
    }
    else
    {
        setAudioChannels(2);
        emit audioChannelsChanged(2);
    }
}


