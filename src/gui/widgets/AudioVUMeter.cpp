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


#include "AudioVUMeter.h"

#include "gui/rulers/VelocityColour.h"
#include <QColor>
#include <QLabel>
#include <QPainter>
#include <QTimer>
#include <QWidget>
#include "VUMeter.h"


namespace Rosegarden
{

AudioVUMeter::AudioVUMeter(QWidget *parent,
                           VUMeter::VUMeterType type,
                           bool stereo,
                           bool hasRecord,
                           int width,
                           int height,
                           const char *name) :
        QWidget(parent, name),
        m_stereo(stereo)
{
    setBackgroundMode(Qt::NoBackground);
    setFixedSize(width, height);

    // This offset is intended to match that for the height of the
    // button pixmap in Fader (in studiowidgets.cpp, which
    // is probably where this class should be too)

    m_yoff = height / 7;
    m_yoff /= 10;
    ++m_yoff;
    m_yoff *= 10;
    ++m_yoff;

    // This one is _not_ intended to match that for the button width

    m_xoff = width / 4;
    if (m_xoff % 2 == 1)
        ++m_xoff;

    m_meter = new AudioVUMeterImpl(this, type, stereo, hasRecord,
                                   width - m_xoff, height - m_yoff, name);

    m_meter->move(m_xoff / 2, m_yoff / 2);
}

void
AudioVUMeter::paintEvent(QPaintEvent *e)
{
    QPainter paint(this);
    paint.setPen(colorGroup().mid());
    paint.drawRect(0, 0, width(), height());

    paint.setPen(colorGroup().background());
    paint.setBrush(colorGroup().background());
    paint.drawRect(1, 1, width() - 2, m_yoff / 2 - 1);
    paint.drawRect(1, 1, m_xoff / 2 - 1, height() - 2);
    paint.drawRect(width() - m_xoff / 2 - 1, 1, m_xoff / 2, height() - 2);
    paint.drawRect(1, height() - m_yoff / 2 - 1, width() - 2, m_yoff / 2);
    paint.end();

    m_meter->paintEvent(e);
}

AudioVUMeter::AudioVUMeterImpl::AudioVUMeterImpl(QWidget *parent,
        VUMeterType type,
        bool stereo,
        bool hasRecord,
        int width,
        int height,
        const char *name) :
        VUMeter(parent, type, stereo, hasRecord, width, height, VUMeter::Vertical, name)
{}

}
