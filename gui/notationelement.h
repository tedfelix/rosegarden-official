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

#ifndef NOTATIONELEMENT_H
#define NOTATIONELEMENT_H

#include <set>
#include "ViewElement.h"
#include "NotationTypes.h"

class QCanvasItem;

/**
 * The Notation H and V layout is performed on a
 * NotationElementList. Once this is done, each NotationElement is
 * affected a QCanvasItem which is set at these coords.
 *
 * @see NotationView#showElements()
 */

class NotationElement : public Rosegarden::ViewElement
{
public:
    struct NoCanvasItem {};
    
    NotationElement(Rosegarden::Event *event);

    ~NotationElement();

    /**
     * Returns the X coordinate of the element, as computed by the
     * layout. This is not the coordinate of the associated canvas
     * item.
     *
     * @see getCanvasX()
     */
    double getLayoutX() { return m_x; }

    /**
     * Returns the Y coordinate of the element, as computed by the
     * layout. This is not the coordinate of the associated canvas
     * item.
     *
     * @see getCanvasY()
     */
    double getLayoutY() { return m_y; }

    /// returns the x pos of the associated canvas item
    double getCanvasX() throw (NoCanvasItem);

    /// returns the y pos of the associated canvas item
    double getCanvasY() throw (NoCanvasItem);

    /**
     * Sets the X coordinate which was computed by the layout engine
     * @see getLayoutX()
     */
    void setLayoutX(double x) { m_x = x; }

    /**
     * Sets the Y coordinate which was computed by the layout engine
     * @see getLayoutY()
     */
    void setLayoutY(double y) { m_y = y; }

    /// Returns true if the wrapped event is a rest
    bool isRest() const;

    /// Returns true if the wrapped event is a note
    bool isNote() const;

    /**
     * Sets the canvas item representing this notation element on screen.
     *
     * The canvas item will have its coords set to the ones of the
     * notation element (as set by the H/V layout) + the offset
     *
     * NOTE: The object takes ownership of its canvas item.
     */
    void setCanvasItem(QCanvasItem *e, double dxoffset, double dyoffset);

    void removeCanvasItem();

    /**
     * Reset the position of the canvas item (which is assumed to
     * exist already) to the stored x and y coordinates with the given
     * offset.  For use when x and y have changed, as well as when
     * the offset has changed.
     */
    void reposition(double dxoffset, double dyoffset);

    bool isSelected();
    void setSelected(bool selected);

    /// Returns the associated canvas item
    QCanvasItem* getCanvasItem() { return m_canvasItem; }

protected:
    //--------------- Data members ---------------------------------

    double m_x;
    double m_y;

    QCanvasItem *m_canvasItem;
};


typedef Rosegarden::ViewElementList<NotationElement> NotationElementList;


#endif
