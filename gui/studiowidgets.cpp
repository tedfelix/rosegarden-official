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

#include <qvbox.h>

#include "studiowidgets.h"


// ---------------- AudioFaderWidget ------------------
//

AudioFaderWidget::AudioFaderWidget(QWidget *parent, const char *name):
    QWidget(parent, name)
{
    QVBox *vbox = new QVBox(this);

    // Plugin box
    //

    QPushButton *plugin;
    for (int i = 0; i < 5; i++)
    {
        plugin = new QPushButton(vbox);
        plugin->setText(i18n("<plugin>"));
        m_plugins.push_back(plugin);
    }

    // VU meter and fader
    //
    QHBox *hbox = new QHBox(vbox);
    m_vuMeter = new AudioVUMeter(hbox);
    m_fader = new QSlider(Qt::Vertical, hbox);

    // Stereo, solo, mute and pan
    //
    hbox = new QHBox(vbox);
    m_pan = new RosegardenRotary(hbox);
    m_stereoButton = new QPushButton(hbox);

    hbox = new QHBox(vbox);
    m_muteButton = new QPushButton(hbox);
    m_soloButton = new QPushButton(hbox);




}


void
AudioFaderWidget::paintEvent(QPaintEvent *e)
{
    QPainter paint(this);

    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalize());

    paint.setPen(kapp->palette().color(QPalette::Active, QColorGroup::Dark));

    /*
    if (m_knobColour != Qt::black)
        paint.setBrush(m_knobColour);
    else
        paint.setBrush(
                kapp->palette().color(QPalette::Active, QColorGroup::Base));

    paint.drawEllipse(0, 0, m_size, m_size);

    drawPosition();
    */
}

void
AudioFaderWidget::mousePressEvent(QMouseEvent *e)
{
}

void
AudioFaderWidget::mouseReleaseEvent(QMouseEvent *e)
{
}

void
AudioFaderWidget::mouseMoveEvent(QMouseEvent *e)
{
}

void
AudioFaderWidget::wheelEvent(QWheelEvent *e)
{
}


