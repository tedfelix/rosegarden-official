// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.2
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

#include <vector>

#include <qpainter.h>

#include "qcanvassimplesprite.h"
#include "rosestrings.h"
#include "rosedebug.h"

QCanvasSimpleSprite::QCanvasSimpleSprite(QPixmap *pixmap, QCanvas *canvas)
    : QCanvasSprite(0, canvas),
      m_pixmapArray(0)
{
    m_pixmapArray = makePixmapArray(pixmap);
    setSequence(m_pixmapArray);
}

QCanvasSimpleSprite::QCanvasSimpleSprite(QCanvasPixmap *pixmap, QCanvas *canvas)
    : QCanvasSprite(0, canvas),
      m_pixmapArray(0)
{
    m_pixmapArray = makePixmapArray(pixmap);
    setSequence(m_pixmapArray);
}

QCanvasSimpleSprite::QCanvasSimpleSprite(const QString &pixmapfile,
                                         QCanvas *canvas)
    : QCanvasSprite(0, canvas),
      m_pixmapArray(0)
{
    m_pixmapArray = makePixmapArray(pixmapfile);
    setSequence(m_pixmapArray);
}

QCanvasSimpleSprite::QCanvasSimpleSprite(QCanvas *canvas)
    : QCanvasSprite(0, canvas),
      m_pixmapArray(0)
{
    QCanvasPixmapArray* tmpArray = makePixmapArray(new QPixmap());
    setSequence(tmpArray);
}


QCanvasSimpleSprite::~QCanvasSimpleSprite()
{
    PixmapArrayGC::registerForDeletion(m_pixmapArray);
    m_pixmapArray = 0;

    // We can't delete m_pixmapArray or we get a core dump.
    //
    // The reason I think is that after the QCanvasSprite is deleted,
    // it is removed from the QCanvas, which therefore needs the
    // pixmaps to know how to update itself (the crash is in
    // QCanvas::removeChunks(), usually).
    //
    // So instead we have to do this GCish
    // thingy. PixmapArrayGC::deleteAll() is called by
    // NotationView::redoLayout
}

QCanvasPixmapArray*
QCanvasSimpleSprite::makePixmapArray(QPixmap *pixmap)
{
    QList<QPixmap> pixlist;
    pixlist.setAutoDelete(true); // the QCanvasPixmapArray creates its
                                 // own copies of the pixmaps, so we
                                 // can delete the one we're passed
    pixlist.append(pixmap);

    QList<QPoint> spotlist;
    spotlist.setAutoDelete(true);
    spotlist.append(new QPoint(0,0));

    return new QCanvasPixmapArray(pixlist, spotlist);
}

QCanvasPixmapArray*
QCanvasSimpleSprite::makePixmapArray(QCanvasPixmap *pixmap)
{
    QList<QPixmap> pixlist;
    pixlist.setAutoDelete(true); // the QCanvasPixmapArray creates its
                                 // own copies of the pixmaps, so we
                                 // can delete the one we're passed
    pixlist.append(pixmap);

    QList<QPoint> spotlist;
    spotlist.setAutoDelete(true);
    spotlist.append(new QPoint(pixmap->offsetX(),pixmap->offsetY()));

    return  new QCanvasPixmapArray(pixlist, spotlist);
}

QCanvasPixmapArray*
QCanvasSimpleSprite::makePixmapArray(const QString &pixmapfile)
{
    return new QCanvasPixmapArray(pixmapfile);
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

QCanvasNotationSprite::~QCanvasNotationSprite()
{
}

//////////////////////////////////////////////////////////////////////

void PixmapArrayGC::registerForDeletion(QCanvasPixmapArray* array)
{
    m_pixmapArrays.push_back(array);
}

void PixmapArrayGC::deleteAll()
{
    RG_DEBUG << "PixmapArrayGC::deleteAll() : "
             << m_pixmapArrays.size() << " pixmap arrays to delete\n";

    for (unsigned int i = 0; i < m_pixmapArrays.size(); ++i)
        delete m_pixmapArrays[i];
    
    m_pixmapArrays.clear();
}

std::vector<QCanvasPixmapArray*> PixmapArrayGC::m_pixmapArrays;

//////////////////////////////////////////////////////////////////////
