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


#include <QPaintEvent>
#include "CountdownBar.h"

#include "gui/general/GUIPalette.h"
#include <QFrame>
#include <QPainter>
#include <QWidget>


namespace Rosegarden
{

CountdownBar::CountdownBar(QWidget *parent, int width, int height):
        QFrame(parent), m_width(width), m_height(height), m_position(0)
{
    resize(m_width, m_height);
    repaint();
}

void
CountdownBar::paintEvent(QPaintEvent *e)
{
    QPainter p(this);

    p.setClipRegion(e->region());
    p.setClipRect(e->rect().normalized());

    p.setPen(GUIPalette::getColour(GUIPalette::AudioCountdownBackground));
    p.setBrush(GUIPalette::getColour(GUIPalette::AudioCountdownBackground));
    p.drawRect(0, 0, m_position, m_height);
    p.setPen(GUIPalette::getColour(GUIPalette::AudioCountdownForeground));
    p.setBrush(GUIPalette::getColour(GUIPalette::AudioCountdownForeground));
    p.drawRect(m_position, 0, m_width, m_height);
}

void
CountdownBar::setPosition(int position)
{
    m_position = position;
    repaint();
}

}
