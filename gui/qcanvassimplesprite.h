// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#include <qwmatrix.h>
#include <qcanvas.h>

#include "notepixmapfactory.h"

/**
 * A QCanvasSprite with 1 frame only
 */
class QCanvasSimpleSprite : public QCanvasSprite
{
public:
    QCanvasSimpleSprite(QPixmap*, QCanvas*);
    QCanvasSimpleSprite(QCanvasPixmap*, QCanvas*);
    QCanvasSimpleSprite(const QString &pixmapfile, QCanvas*);

    // For lazy pixmap rendering, when we get around looking at it
    QCanvasSimpleSprite(QCanvas*);

    virtual ~QCanvasSimpleSprite();

protected:
    static QCanvasPixmapArray* makePixmapArray(QPixmap *pixmap);

    static QCanvasPixmapArray* makePixmapArray(QCanvasPixmap *pixmap);

    static QCanvasPixmapArray* makePixmapArray(const QString &pixmapfile);

    //--------------- Data members ---------------------------------

    QCanvasPixmapArray* m_pixmapArray;
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

    virtual ~QCanvasNotationSprite();
    
    NotationElement& getNotationElement() { return m_notationElement; }

protected:
    //--------------- Data members ---------------------------------

    NotationElement& m_notationElement;
};

/**
 * A QCanvasSprite used for a time signature
 */
class QCanvasTimeSigSprite : public QCanvasSimpleSprite
{
public:
    QCanvasTimeSigSprite(double layoutX, QPixmap *, QCanvas *);
    QCanvasTimeSigSprite(double layoutX, QCanvasPixmap *, QCanvas *);
    virtual ~QCanvasTimeSigSprite();

    void setLayoutX(double layoutX) { m_layoutX = layoutX; }
    double getLayoutX() const { return m_layoutX; }

protected:
    double m_layoutX;
};

/**
 * A QCanvasSprite used for a staff name
 */
class QCanvasStaffNameSprite : public QCanvasSimpleSprite
{
public:
    QCanvasStaffNameSprite(QPixmap *, QCanvas *);
    QCanvasStaffNameSprite(QCanvasPixmap *, QCanvas *);
    virtual ~QCanvasStaffNameSprite();
};

/**
 * A GC for QCanvasPixmapArray which have to be deleted seperatly
 */
class PixmapArrayGC
{
public:
    static void registerForDeletion(QCanvasPixmapArray*);
    static void deleteAll();
    
protected:
    //--------------- Data members ---------------------------------

    static std::vector<QCanvasPixmapArray*> m_pixmapArrays;
};

#endif
