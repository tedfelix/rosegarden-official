/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_NOTECHARACTER_H
#define RG_NOTECHARACTER_H

#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QPoint>
#include <QPolygon>


class QPainter;

namespace Rosegarden
{

class NoteCharacterDrawRep : public QPolygon
{
public:
    NoteCharacterDrawRep(int size = 0) : QPolygon(size) { }
};


/**
 * NoteCharacter knows how to draw a character from a font.  It may be
 * optimised for screen (using QPixmap underneath to produce
 * low-resolution colour or greyscale glyphs) or printer (using some
 * internal representation to draw in high-resolution monochrome on a
 * print device).  You can use screen characters on a printer and vice
 * versa, but the performance and quality might not be as good.
 *
 * NoteCharacter objects are always constructed by the NoteFont, never
 * directly.
 */

class NoteCharacter
{
public:
    NoteCharacter();
    NoteCharacter(const NoteCharacter &);
    NoteCharacter &operator=(const NoteCharacter &);
    ~NoteCharacter();

    int getWidth() const;
    int getHeight() const;
    
    QPixmap getPixmap() const;
    QPoint getHotspot() const;
    QGraphicsPixmapItem *makeItem() const;

    void draw(QPainter *painter, int x, int y) const;

private:
    friend class NoteFont;
    NoteCharacter(QPixmap pixmap, QPoint hotspot, NoteCharacterDrawRep *rep);

    QPoint m_hotspot;
    QPixmap m_pixmap; 
    NoteCharacterDrawRep *m_rep; // I don't own this, it's a reference to a static in the NoteFont
};
    


}

#endif
