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

#ifndef QCANVASSIMPLESPRITE_H
#define QCANVASSIMPLESPRITE_H

#include <Q3Canvas>
#include <Q3CanvasPixmap>
#include <Q3CanvasPixmapArray>
#include <Q3CanvasSprite>
#include <QMatrix>
#include <Q3Canvas>

#include "gui/editors/notation/NotePixmapFactory.h"

namespace Rosegarden {

/**
 * A Q3CanvasSprite with 1 frame only
 */
class QCanvasSimpleSprite : public Q3CanvasSprite
{
public:
    QCanvasSimpleSprite(QPixmap*, Q3Canvas*);
    QCanvasSimpleSprite(Q3CanvasPixmap*, Q3Canvas*);
    QCanvasSimpleSprite(const QString &pixmapfile, Q3Canvas*);

    // For lazy pixmap rendering, when we get around looking at it
    QCanvasSimpleSprite(Q3Canvas*);

    virtual ~QCanvasSimpleSprite();

protected:
    static Q3CanvasPixmapArray* makePixmapArray(QPixmap *pixmap);

    static Q3CanvasPixmapArray* makePixmapArray(Q3CanvasPixmap *pixmap);

    static Q3CanvasPixmapArray* makePixmapArray(const QString &pixmapfile);

    //--------------- Data members ---------------------------------

    Q3CanvasPixmapArray* m_pixmapArray;
};

class NotationElement;

/**
 * A Q3CanvasSprite referencing a NotationElement
 */
class QCanvasNotationSprite : public QCanvasSimpleSprite
{
public:
    QCanvasNotationSprite(Rosegarden::NotationElement&, QPixmap*, Q3Canvas*);
    QCanvasNotationSprite(Rosegarden::NotationElement&, Q3CanvasPixmap*, Q3Canvas*);

    virtual ~QCanvasNotationSprite();
    
    Rosegarden::NotationElement& getNotationElement() { return m_notationElement; }

protected:
    //--------------- Data members ---------------------------------

    Rosegarden::NotationElement& m_notationElement;
};

class QCanvasNonElementSprite : public QCanvasSimpleSprite
{
public:
    QCanvasNonElementSprite(QPixmap *, Q3Canvas *);
    QCanvasNonElementSprite(Q3CanvasPixmap *, Q3Canvas *);
    virtual ~QCanvasNonElementSprite();
};

/**
 * A Q3CanvasSprite used for a time signature
 */
class QCanvasTimeSigSprite : public QCanvasNonElementSprite
{
public:
    QCanvasTimeSigSprite(double layoutX, QPixmap *, Q3Canvas *);
    QCanvasTimeSigSprite(double layoutX, Q3CanvasPixmap *, Q3Canvas *);
    virtual ~QCanvasTimeSigSprite();

    void setLayoutX(double layoutX) { m_layoutX = layoutX; }
    double getLayoutX() const { return m_layoutX; }

protected:
    double m_layoutX;
};

/**
 * A Q3CanvasSprite used for a staff name
 */
class QCanvasStaffNameSprite : public QCanvasNonElementSprite
{
public:
    QCanvasStaffNameSprite(QPixmap *, Q3Canvas *);
    QCanvasStaffNameSprite(Q3CanvasPixmap *, Q3Canvas *);
    virtual ~QCanvasStaffNameSprite();
};

/**
 * A GC for Q3CanvasPixmapArray which have to be deleted seperatly
 */
class PixmapArrayGC
{
public:
    static void registerForDeletion(Q3CanvasPixmapArray*);
    static void deleteAll();
    
protected:
    //--------------- Data members ---------------------------------

    static std::vector<Q3CanvasPixmapArray*> m_pixmapArrays;
};

}

#endif
