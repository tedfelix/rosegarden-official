/***************************************************************************
                          qcanvassimplesprite.cpp  -  description
                             -------------------
    begin                : Thu Aug 3 2000
    copyright            : (C) 2000 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

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
    // don't delete m_pixmapArray, it's owned by QCanvasSprite
}

QCanvasPixmapArray*
QCanvasSimpleSprite::makePixmapArray(QCanvasSimpleSprite *self,
                                     QPixmap *pixmap)
{
    QList<QPixmap> pixlist;
    pixlist.append(pixmap);

    QList<QPoint> spotlist;
    spotlist.setAutoDelete(true);
    spotlist.append(new QPoint(0,0));

    self->m_pixmapArray = new QCanvasPixmapArray(pixlist, spotlist);

    return self->m_pixmapArray;
}

QCanvasPixmapArray*
QCanvasSimpleSprite::makePixmapArray(QCanvasSimpleSprite *self,
                                     QCanvasPixmap *pixmap)
{
    QList<QPixmap> pixlist;
    pixlist.append(pixmap);

    QList<QPoint> spotlist;
    spotlist.setAutoDelete(true);
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
