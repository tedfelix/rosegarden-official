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

#include "qcanvassimplesprite.h"

QCanvasSimpleSprite::QCanvasSimpleSprite(QPixmap *pixmap, QCanvas *canvas)
    : QCanvasSprite(makePixmapArray(this, pixmap), canvas)
{
}

QCanvasSimpleSprite::QCanvasSimpleSprite(QCanvasPixmap *pixmap, QCanvas *canvas)
    : QCanvasSprite(makePixmapArray(this, pixmap), canvas)
{
}

QCanvasSimpleSprite::QCanvasSimpleSprite(const QString &pixmapfile,
                                         QCanvas *canvas)
    : QCanvasSprite(makePixmapArray(this, pixmapfile), canvas)
{
}

QCanvasSimpleSprite::~QCanvasSimpleSprite()
{
    // don't delete m_pixmapArray or I get a core dump.

    // TODO: However it doesn't seems that the QCanvasSprite actually
    // deletes it. Something fishy going on here.
}

QCanvasPixmapArray*
QCanvasSimpleSprite::makePixmapArray(QCanvasSimpleSprite *self,
                                     QPixmap *pixmap)
{
    QList<QPixmap> pixlist;
    pixlist.setAutoDelete(TRUE);
    pixlist.append(pixmap);

    QList<QPoint> spotlist;
    spotlist.setAutoDelete(TRUE);
    spotlist.append(new QPoint(0,0));

    self->m_pixmapArray = new QCanvasPixmapArray(pixlist, spotlist);

    return self->m_pixmapArray;
}

QCanvasPixmapArray*
QCanvasSimpleSprite::makePixmapArray(QCanvasSimpleSprite *self,
                                     QCanvasPixmap *pixmap)
{
    QList<QPixmap> pixlist;
    pixlist.setAutoDelete(TRUE);
    pixlist.append(pixmap);

    QList<QPoint> spotlist;
    spotlist.setAutoDelete(TRUE);
    spotlist.append(new QPoint(pixmap->offsetX(),pixmap->offsetY()));

    self->m_pixmapArray = new QCanvasPixmapArray(pixlist, spotlist);

    return self->m_pixmapArray;
}

QCanvasPixmapArray*
QCanvasSimpleSprite::makePixmapArray(QCanvasSimpleSprite *self,
                                     const QString &pixmapfile)
{
    self->m_pixmapArray = new QCanvasPixmapArray(pixmapfile);
    return self->m_pixmapArray;
}

//////////////////////////////////////////////////////////////////////

QCanvasNotationSprite::QCanvasNotationSprite(NotationElement& n,
                                             QPixmap* pixmap,
                                             QCanvas* canvas)
    : QCanvasSimpleSprite(pixmap, canvas),
      m_notationElement(n)
{
}

QCanvasNotationSprite::QCanvasNotationSprite(NotationElement& n,
                                             QCanvasPixmap* pixmap,
                                             QCanvas* canvas)
    : QCanvasSimpleSprite(pixmap, canvas),
      m_notationElement(n)
{
}
