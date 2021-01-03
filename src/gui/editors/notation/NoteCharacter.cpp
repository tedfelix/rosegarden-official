/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "NoteCharacter.h"

#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QBitmap>

#include <iostream>


namespace Rosegarden
{

NoteCharacter::NoteCharacter() :
    m_hotspot(0, 0),
    m_rep(nullptr)
{}

NoteCharacter::NoteCharacter(QPixmap pixmap,
                             QPoint hotspot, NoteCharacterDrawRep *rep) :
    m_hotspot(hotspot),
    m_pixmap(pixmap),
    m_rep(rep)
{}

NoteCharacter::NoteCharacter(const NoteCharacter &c) :
    m_hotspot(c.m_hotspot),
    m_pixmap(c.m_pixmap),
    m_rep(c.m_rep)
{
    // nothing else
}

NoteCharacter &
NoteCharacter::operator=(const NoteCharacter &c)
{
    if (&c == this) return * this;
    m_hotspot = c.m_hotspot;
    m_pixmap = c.m_pixmap;
    m_rep = c.m_rep;
    return *this;
}

NoteCharacter::~NoteCharacter()
{
}

int
NoteCharacter::getWidth() const
{
    return m_pixmap.width();
}

int
NoteCharacter::getHeight() const
{
    return m_pixmap.height();
}

QPoint
NoteCharacter::getHotspot() const
{
    return m_hotspot;
}

QPixmap
NoteCharacter::getPixmap() const
{
    return m_pixmap;
}

QGraphicsPixmapItem *
NoteCharacter::makeItem() const
{
    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(m_pixmap);
    item->setOffset(QPointF(-m_hotspot.x(), -m_hotspot.y()));
    return item;
}

void
NoteCharacter::draw(QPainter *painter, int x, int y) const
{
    if (!m_rep) {

//std::cout << "Pay attention Michael:  !m_rep tested true.  Executing painter->drawPixmap(x, y, m_pixmap)..." << std::endl;

        painter->drawPixmap(x, y, m_pixmap);

    } else {

std::cout << "Pay attention Michael:  !m_rep tested FALSE.  Aborting...  This is the big kaboom abort, right?" << std::endl;
        abort();
std::cout << "Pay attention Michael:  No, evidently not.  What the hell?  If that wasn't the big kaboom abort, what was it?" << std::endl;

/*
 *
 * There's a QT3 problem in this code block, but since it comes immediately
 * after an abort (WTF?) I'm thinking there's probably no point in rewriting it,
 * because odds are this code never runs anyway.
 *
 * We'll find out soon enough, I guess.
 *
 * For reference:
 *
 *

void QPainter::drawLineSegments ( const QPolygon & polygon, int index = 0, int count = -1 )

Draws count separate lines from points defined by the polygon, starting at polygon[index] (index defaults to 0). If count is -1 (the default) all points until the end of the array are used.

Use drawLines() combined with QPolygon::constData() instead.

For example, if you have code like

 QPainter painter(this);
 painter.drawLineSegments(polygon, index, count);

you can rewrite it as

 int lineCount = (count == -1) ?  (polygon.size() - index) / 2  : count;

 QPainter painter(this);
 painter.drawLines(polygon.constData() + index * 2, lineCount);


 *
 *
        NoteCharacterDrawRep a(m_rep->size());

        for (int i = 0; i < m_rep->size(); ++i) {
            QPoint p(m_rep->point(i));
            a.setPoint(i, p.x() + x, p.y() + y);
        }

        painter->drawLineSegments(a);
*/
    }
}

}
