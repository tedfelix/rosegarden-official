// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#ifndef MATRIXELEMENT_H
#define MATRIXELEMENT_H

#include "ViewElement.h"
#include <qcanvas.h>
#include <qbrush.h>

class MatrixElement;

/**
 * A QCanvasRectangle referencing a MatrixElement
 */
class QCanvasMatrixRectangle : public QCanvasRectangle
{
public:
    QCanvasMatrixRectangle(MatrixElement&, QCanvas*);

    virtual ~QCanvasMatrixRectangle();
    
    MatrixElement& getMatrixElement() { return m_matrixElement; }
    
protected:
    //--------------- Data members ---------------------------------

    MatrixElement& m_matrixElement;

};

class MatrixElement : public Rosegarden::ViewElement
{
public:
    MatrixElement(Rosegarden::Event *event);

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

    /**
     * Sets the x coordinate of the element on the canvas
     */
    void setCanvasX(double x) { m_canvasRect->setX(x); }

    /**
     * Sets the y coordinate of the element on the canvas
     */
    void setCanvasY(double y) { m_canvasRect->setY(y); }

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

    QCanvasMatrixRectangle* m_canvasRect;
};


typedef Rosegarden::ViewElementList MatrixElementList;

#endif
