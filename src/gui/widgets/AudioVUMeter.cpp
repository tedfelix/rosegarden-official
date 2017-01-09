/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
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
                           int height) :
        QWidget(parent),
        m_stereo(stereo)
{
    //setBackgroundMode(Qt::NoBackground);
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
                                   width - m_xoff, height - m_yoff);

    m_meter->move(m_xoff / 2, m_yoff / 2);
}

void
AudioVUMeter::paintEvent(QPaintEvent */* e */)
{
    //###
    // See note in VUMeter.cpp explaining the width/height - 1 issue
    int w = width() - 1;
    int h = height() - 1;
    QPainter paint(this);

    // we'll try giving the area between the border and the actual meter the
    // same tint as the faders, for consistency of appearance

    // first, we'll fill the whole background rect with a 40% alpha version of
    // the border color
    QColor fill = palette().mid().color();
    int H = 0;
    int S = 0;
    int V = 0;
    int A = 0;
    fill.getHsv(&H, &S, &V, &A);
    A = 40;
    fill = QColor::fromHsv(H, S, V, A);
    paint.fillRect(0, 0, w, h, fill);
    
    // now we draw the border outline around it
    paint.setPen(palette().mid().color());
    paint.drawRect(0, 0, w, h);

    paint.setPen(palette().background().color());
    paint.setBrush(palette().background());
    paint.drawRect(1, 1, w - 2, m_yoff / 2 - 1);
    paint.drawRect(1, 1, m_xoff / 2 - 1, h - 2);
    paint.drawRect(w - m_xoff / 2 - 1, 1, m_xoff / 2, h - 2);
    paint.drawRect(1, h - m_yoff / 2 - 1, w - 2, m_yoff / 2);
    paint.end();

//  m_meter->paintEvent(e);
}

AudioVUMeter::AudioVUMeterImpl::AudioVUMeterImpl(QWidget *parent,
        VUMeterType type,
        bool stereo,
        bool hasRecord,
        int width,
        int height) :
        VUMeter(parent, type, stereo, hasRecord, width, height, VUMeter::Vertical)
{}

}
