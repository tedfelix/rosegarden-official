// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#ifndef QCANVASSIMPLESPRITE_H
#define QCANVASSIMPLESPRITE_H

#include <qcanvas.h>

/**
 * A QCanvasSprite with 1 frame only
 */
class QCanvasSimpleSprite : public QCanvasSprite
{
public:
    QCanvasSimpleSprite(QPixmap*, QCanvas*);
    QCanvasSimpleSprite(QCanvasPixmap*, QCanvas*);
    QCanvasSimpleSprite(const QString &pixmapfile, QCanvas*);

    virtual ~QCanvasSimpleSprite();

protected:
    QCanvasPixmapArray *m_pixmapArray;

    static QCanvasPixmapArray* makePixmapArray(QCanvasSimpleSprite*,
                                               QPixmap *pixmap);

    static QCanvasPixmapArray* makePixmapArray(QCanvasSimpleSprite*,
                                               QCanvasPixmap *pixmap);

    static QCanvasPixmapArray* makePixmapArray(QCanvasSimpleSprite*,
                                               const QString &pixmapfile);

};

class NotationElement;

/**
 * A QCanvasSprite referencing a NotationElement
 */
class QCanvasNotationSprite : public QCanvasSimpleSprite
{
public:
    QCanvasNotationSprite(NotationElement&, QPixmap*, QCanvas*);
    QCanvasNotationSprite(NotationElement&, QCanvasPixmap*, QCanvas*);
    
    NotationElement& getNotationElement() { return m_notationElement; }
    
protected:
    NotationElement& m_notationElement;

};


#endif
