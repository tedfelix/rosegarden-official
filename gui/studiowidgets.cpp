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

#include <kapp.h>
#include <klocale.h>
#include <kstddirs.h>

#include <qlayout.h>
#include <qvbox.h>

#include "studiowidgets.h"


// ---------------- AudioFaderWidget ------------------
//

AudioFaderWidget::AudioFaderWidget(QWidget *parent, const char *name):
    QWidget(parent, name),
    m_signalMapper(new QSignalMapper(this))
{
    QGridLayout *grid = new QGridLayout(this,
                                        7, 2,
                                        6, 6);
    // Plugin box
    //
    QPushButton *plugin;
    QVBox *vbox = new QVBox(this);
    vbox->setSpacing(2);

    grid->addMultiCellWidget(vbox, 0, 0, 0, 1, AlignCenter);

    for (int i = 0; i < 5; i++)
    {
        plugin = new QPushButton(vbox);
        plugin->setText(i18n("<no plugin>"));
        m_plugins.push_back(plugin);
        m_signalMapper->setMapping(plugin, i);
        connect(plugin, SIGNAL(clicked()),
                m_signalMapper, SLOT(map()));
    }

    // VU meter and fader
    //
    m_vuMeter = new AudioVUMeter(this);
    m_fader = new RosegardenFader(this);

    m_fader->setTickmarks(QSlider::Right);
    m_fader->setTickInterval(10);
    m_fader->setPageStep(10);
    m_fader->setMinValue(0);
    m_fader->setMaxValue(127);
    m_fader->setFixedHeight(m_vuMeter->height());

    grid->addMultiCellWidget(m_vuMeter, 1, 1, 0, 0, AlignCenter);
    grid->addMultiCellWidget(m_fader,   1, 1, 1, 1, AlignCenter);

    // Stereo, solo, mute and pan
    //
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    m_monoPixmap.load(QString("%1/misc/mono.xpm").arg(pixmapDir));
    m_stereoPixmap.load(QString("%1/misc/stereo.xpm").arg(pixmapDir));

    m_pan = new RosegardenRotary(this);
    m_stereoButton = new QPushButton(this);
    m_stereoButton->setPixmap(m_monoPixmap);
    m_stereoButton->setFixedSize(22, 22);

    grid->addMultiCellWidget(m_stereoButton,  2, 2, 0, 0, AlignCenter);
    grid->addMultiCellWidget(m_pan,           2, 2, 1, 1, AlignCenter);

    m_muteButton = new QPushButton(this);
    m_soloButton = new QPushButton(this);
    m_muteButton->setText("M");
    m_soloButton->setText("S");

    grid->addMultiCellWidget(m_muteButton, 3, 3, 0, 0, AlignCenter);
    grid->addMultiCellWidget(m_soloButton, 3, 3, 1, 1, AlignCenter);

    m_recordButton = new QPushButton(this);
    m_recordButton->setText("R");

    grid->addMultiCellWidget(m_recordButton, 4, 4, 0, 0, AlignCenter);
}


