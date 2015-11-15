/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[TextRuler]"

#include <QPaintEvent>
#include "TextRuler.h"

#include "base/Event.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/NotationTypes.h"
#include "base/RulerScale.h"
#include "base/Segment.h"
#include "gui/general/GUIPalette.h"
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QRect>
#include <QSize>
#include <QWidget>


namespace Rosegarden
{

TextRuler::TextRuler(RulerScale *rulerScale,
                     Segment *segment,
                     int height,
                     QWidget *parent) :
        QWidget(parent),
        m_height(height),
        m_currentXOffset(0),
        m_width( -1),
        m_segment(segment),
        m_rulerScale(rulerScale),
        m_font("helvetica", 12),
        m_fontMetrics(m_font)
{
    m_mySegmentMaybe = (m_segment->getComposition() != 0);
//    setBackgroundColor(GUIPalette::getColour(GUIPalette::TextRulerBackground));

    m_font.setPixelSize(10);
}

TextRuler::~TextRuler()
{
    if (m_mySegmentMaybe && !m_segment->getComposition()) {
        delete m_segment;
    }
}

void
TextRuler::slotScrollHoriz(int x)
{
    int w = width();
    // int h = height();
    int dx = x - ( -m_currentXOffset);
    m_currentXOffset = -x;

    if (dx > w*3 / 4 || dx < -w*3 / 4) {
        update();
        return ;
    }

//    if (dx > 0) { // moving right, so the existing stuff moves left
//        bitBlt(this, 0, 0, this, dx, 0, w - dx, h);
//        repaint(w - dx, 0, dx, h);
//    } else {      // moving left, so the existing stuff moves right
//        bitBlt(this, -dx, 0, this, 0, 0, w + dx, h);
//        repaint(0, 0, -dx, h);
//    }
}

QSize
TextRuler::sizeHint() const
{
    double width =
        m_rulerScale->getBarPosition(m_rulerScale->getLastVisibleBar()) +
        m_rulerScale->getBarWidth(m_rulerScale->getLastVisibleBar());

    QSize res(std::max(int(width), m_width), m_height);

    return res;
}

QSize
TextRuler::minimumSizeHint() const
{
    double firstBarWidth = m_rulerScale->getBarWidth(0);
    QSize res = QSize(int(firstBarWidth), m_height);
    return res;
}

void
TextRuler::paintEvent(QPaintEvent* e)
{
    QPainter paint(this);
    paint.setPen(GUIPalette::getColour(GUIPalette::TextRulerForeground));

    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalized());

    QRect clipRect = paint.clipRegion().boundingRect();

    timeT from = m_rulerScale->getTimeForX
                 (clipRect.x() - m_currentXOffset - 100);
    timeT to = m_rulerScale->getTimeForX
               (clipRect.x() + clipRect.width() - m_currentXOffset + 100);

    for (Segment::iterator i = m_segment->findTime(from);
            i != m_segment->findTime(to) && i != m_segment->end(); ++i) {

        if (!(*i)->isa(Text::EventType))
            continue;

        std::string text;
        if (!(*i)->get
                <String>(Text::TextPropertyName, text)) {
            RG_DEBUG
            << "Warning: TextRuler::paintEvent: No text in text event"
            << endl;
            continue;
        }

        QRect bounds = m_fontMetrics.boundingRect(strtoqstr(text));

        double x = m_rulerScale->getXForTime((*i)->getAbsoluteTime()) +
                   m_currentXOffset - bounds.width() / 2;

        int y = height() / 2 + bounds.height() / 2;

        paint.drawText(static_cast<int>(x), y, strtoqstr(text));
    }
}

}
