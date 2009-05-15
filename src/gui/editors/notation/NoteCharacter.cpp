/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
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


namespace Rosegarden
{

NoteCharacter::NoteCharacter() :
    m_hotspot(0, 0),
    m_rep(0)
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

        painter->drawPixmap(x, y, m_pixmap);

    } else {

        abort();

        NoteCharacterDrawRep a(m_rep->size());

        for (unsigned int i = 0; i < m_rep->size(); ++i) {
            QPoint p(m_rep->point(i));
            a.setPoint(i, p.x() + x, p.y() + y);
        }

        painter->drawLineSegments(a);
    }
}

}
