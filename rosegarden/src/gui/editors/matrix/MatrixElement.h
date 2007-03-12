
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_MATRIXELEMENT_H_
#define _RG_MATRIXELEMENT_H_

#include "base/ViewElement.h"
#include <qbrush.h>
#include <qcanvas.h>
#include "QCanvasMatrixRectangle.h"

class QColor;


namespace Rosegarden
{

class Event;

class MatrixElement : public ViewElement
{
public:
    MatrixElement(Event *event, bool drum);

    virtual ~MatrixElement();

    void setCanvas(QCanvas* c);

    /**
     * Returns the actual x coordinate of the element on the canvas
     */
    double getCanvasX() const { return m_canvasRect->x(); }

    /**
     * Returns the actual y coordinate of the element on the canvas
     */
    double getCanvasY() const { return m_canvasRect->y(); }

    double getCanvasZ() const { return m_canvasRect->z(); }

    /**
     * Sets the x coordinate of the element on the canvas
     */
    void setCanvasX(double x) { m_canvasRect->setX(x); }

    /**
     * Sets the y coordinate of the element on the canvas
     */
    void setCanvasY(double y) { m_canvasRect->setY(y); }

    void setCanvasZ(double z) { m_canvasRect->setZ(z); }

    /**
     * Sets the width of the rectangle on the canvas
     */
    void setWidth(int w)   { m_canvasRect->setSize(w, m_canvasRect->height()); }
    int getWidth() { return m_canvasRect->width(); }

    /**
     * Sets the height of the rectangle on the canvas
     */
    void setHeight(int h)   { m_canvasRect->setSize(m_canvasRect->width(), h); }
    int getHeight() { return m_canvasRect->height(); }

    /// Returns true if the wrapped event is a note
    bool isNote() const;

    /*
     * Set the colour of the element
     */
    void setColour(const QColor &colour)
        { m_canvasRect->setBrush(QBrush(colour)); }

protected:

    //--------------- Data members ---------------------------------

    QCanvasMatrixRectangle *m_canvasRect;
};


typedef ViewElementList MatrixElementList;


}

#endif
