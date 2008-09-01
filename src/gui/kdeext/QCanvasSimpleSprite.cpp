// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <Q3Canvas>
#include <Q3CanvasPixmap>
#include <Q3CanvasPixmapArray>
#include <Q3CanvasSprite>
#include <vector>
#include "misc/Debug.h"

#include <QPainter>

#include "QCanvasSimpleSprite.h"

namespace Rosegarden {

QCanvasSimpleSprite::QCanvasSimpleSprite(QPixmap *pixmap, Q3Canvas *canvas)
        : Q3CanvasSprite(0, canvas),
        m_pixmapArray(0)
{
    m_pixmapArray = makePixmapArray(pixmap);
    setSequence(m_pixmapArray);
}

QCanvasSimpleSprite::QCanvasSimpleSprite(Q3CanvasPixmap *pixmap, Q3Canvas *canvas)
        : Q3CanvasSprite(0, canvas),
        m_pixmapArray(0)
{
    m_pixmapArray = makePixmapArray(pixmap);
    setSequence(m_pixmapArray);
}

QCanvasSimpleSprite::QCanvasSimpleSprite(const QString &pixmapfile,
        Q3Canvas *canvas)
        : Q3CanvasSprite(0, canvas),
        m_pixmapArray(0)
{
    m_pixmapArray = makePixmapArray(pixmapfile);
    setSequence(m_pixmapArray);
}

QCanvasSimpleSprite::QCanvasSimpleSprite(Q3Canvas *canvas)
        : Q3CanvasSprite(0, canvas),
        m_pixmapArray(0)
{
    Q3CanvasPixmapArray* tmpArray = makePixmapArray(new QPixmap());
    setSequence(tmpArray);
}


QCanvasSimpleSprite::~QCanvasSimpleSprite()
{
    PixmapArrayGC::registerForDeletion(m_pixmapArray);
    m_pixmapArray = 0;

    // We can't delete m_pixmapArray or we get a core dump.
    //
    // The reason I think is that after the Q3CanvasSprite is deleted,
    // it is removed from the Q3Canvas, which therefore needs the
    // pixmaps to know how to update itself (the crash is in
    // Q3Canvas::removeChunks(), usually).
    //
    // So instead we have to do this GCish
    // thingy. PixmapArrayGC::deleteAll() is called by
    // NotationView::redoLayout
}

Q3CanvasPixmapArray*
QCanvasSimpleSprite::makePixmapArray(QPixmap *pixmap)
{
    QList<QPixmap> pixlist;
    pixlist.setAutoDelete(true); // the Q3CanvasPixmapArray creates its
    // own copies of the pixmaps, so we
    // can delete the one we're passed
    pixlist.append(pixmap);

    QList<QPoint> spotlist;
    spotlist.setAutoDelete(true);
    spotlist.append(new QPoint(0, 0));

    return new Q3CanvasPixmapArray(pixlist, spotlist);
}

Q3CanvasPixmapArray*
QCanvasSimpleSprite::makePixmapArray(Q3CanvasPixmap *pixmap)
{
    QList<QPixmap> pixlist;
    pixlist.setAutoDelete(true); // the Q3CanvasPixmapArray creates its
    // own copies of the pixmaps, so we
    // can delete the one we're passed
    pixlist.append(pixmap);

    QList<QPoint> spotlist;
    spotlist.setAutoDelete(true);
    spotlist.append(new QPoint(pixmap->offsetX(), pixmap->offsetY()));

    return new Q3CanvasPixmapArray(pixlist, spotlist);
}

Q3CanvasPixmapArray*
QCanvasSimpleSprite::makePixmapArray(const QString &pixmapfile)
{
    return new Q3CanvasPixmapArray(pixmapfile);
}

//////////////////////////////////////////////////////////////////////

QCanvasNotationSprite::QCanvasNotationSprite(NotationElement& n,
        QPixmap* pixmap,
        Q3Canvas* canvas)
        : QCanvasSimpleSprite(pixmap, canvas),
        m_notationElement(n)
{}

QCanvasNotationSprite::QCanvasNotationSprite(NotationElement& n,
        Q3CanvasPixmap* pixmap,
        Q3Canvas* canvas)
        : QCanvasSimpleSprite(pixmap, canvas),
        m_notationElement(n)

{}

QCanvasNotationSprite::~QCanvasNotationSprite()
{}


QCanvasNonElementSprite::QCanvasNonElementSprite(QPixmap *pixmap,
        Q3Canvas *canvas) :
        QCanvasSimpleSprite(pixmap, canvas)
{}

QCanvasNonElementSprite::QCanvasNonElementSprite(Q3CanvasPixmap *pixmap,
        Q3Canvas *canvas) :
        QCanvasSimpleSprite(pixmap, canvas)
{}

QCanvasNonElementSprite::~QCanvasNonElementSprite()
{}

QCanvasTimeSigSprite::QCanvasTimeSigSprite(double layoutX,
        QPixmap *pixmap,
        Q3Canvas *canvas) :
        QCanvasNonElementSprite(pixmap, canvas),
        m_layoutX(layoutX)
{}

QCanvasTimeSigSprite::QCanvasTimeSigSprite(double layoutX,
        Q3CanvasPixmap *pixmap,
        Q3Canvas *canvas) :
        QCanvasNonElementSprite(pixmap, canvas),
        m_layoutX(layoutX)
{}

QCanvasTimeSigSprite::~QCanvasTimeSigSprite()
{}


QCanvasStaffNameSprite::QCanvasStaffNameSprite(QPixmap *pixmap,
        Q3Canvas *canvas) :
        QCanvasNonElementSprite(pixmap, canvas)
{}

QCanvasStaffNameSprite::QCanvasStaffNameSprite(Q3CanvasPixmap *pixmap,
        Q3Canvas *canvas) :
        QCanvasNonElementSprite(pixmap, canvas)
{}

QCanvasStaffNameSprite::~QCanvasStaffNameSprite()
{}


//////////////////////////////////////////////////////////////////////

void PixmapArrayGC::registerForDeletion(Q3CanvasPixmapArray* array)
{
    m_pixmapArrays.push_back(array);
}

void PixmapArrayGC::deleteAll()
{
    RG_DEBUG << "PixmapArrayGC::deleteAll() : "
    << m_pixmapArrays.size() << " pixmap arrays to delete\n";

    static unsigned long total = 0;

    for (unsigned int i = 0; i < m_pixmapArrays.size(); ++i) {
        Q3CanvasPixmapArray *array = m_pixmapArrays[i];
        QPixmap *pixmap = array->image(0);
        if (pixmap) {
            total += pixmap->width() * pixmap->height();
            //	    NOTATION_DEBUG << "PixmapArrayGC::deleteAll(): " << pixmap->width() << "x" << pixmap->height() << " (" << (pixmap->width()*pixmap->height()) << " px, " << total << " total)" << endl;
        }
        delete m_pixmapArrays[i];
    }

    m_pixmapArrays.clear();
}

std::vector<Q3CanvasPixmapArray*> PixmapArrayGC::m_pixmapArrays;

}

//////////////////////////////////////////////////////////////////////
