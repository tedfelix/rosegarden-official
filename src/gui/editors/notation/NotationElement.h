
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_NOTATIONELEMENT_H_
#define _RG_NOTATIONELEMENT_H_

#include "base/Exception.h"
#include "base/ViewElement.h"
#include <vector>
#include "base/Event.h"


class QCanvasItem;
class ItemList;


namespace Rosegarden
{

class Event;


/**
 * The Notation H and V layout is performed on a
 * NotationElementList. Once this is done, each NotationElement is
 * affected a QCanvasItem which is set at these coords.
 *
 * @see NotationView#showElements()
 */

class NotationElement : public ViewElement
{
public:
    typedef Exception NoCanvasItem;
    
    NotationElement(Event *event);

    ~NotationElement();

    virtual timeT getViewAbsoluteTime() const;
    virtual timeT getViewDuration() const;

    void getLayoutAirspace(double &x, double &width) {
        x = m_airX;
        width = m_airWidth;
    }

    void getCanvasAirspace(double &x, double &width) {
        x = m_airX - getLayoutX() + getCanvasX();
        width = m_airWidth;
    }

    /// returns the x pos of the associated canvas item
    double getCanvasX();

    /// returns the y pos of the associated canvas item
    double getCanvasY();

    /**
     * Sets the X coordinate and width of the space "underneath"
     * this element, i.e. the extents within which a mouse click
     * or some such might be considered to be interested in this
     * element as opposed to any other.  These are layout coords
     */
    void setLayoutAirspace(double x, double width) {
        m_airX = x; m_airWidth = width;
    }

    /// Returns true if the wrapped event is a rest
    bool isRest() const;

    /// Returns true if the wrapped event is a note
    bool isNote() const;

    /// Returns true if the wrapped event is a tuplet
    bool isTuplet() const;

    /// Returns true if the wrapped event is a grace note
    bool isGrace() const;

    /**
     * Sets the canvas item representing this notation element on screen.
     *
     * NOTE: The object takes ownership of its canvas item.
     */
    void setCanvasItem(QCanvasItem *e, double canvasX, double canvasY);

    /**
     * Add an extra canvas item associated with this element, for
     * example where an element has been split across two or more
     * staff rows.
     * 
     * The element will take ownership of these canvas items and
     * delete them when it deletes the main canvas item.
     */
    void addCanvasItem(QCanvasItem *e, double canvasX, double canvasY);

    /**
     * Remove the main canvas item and any additional ones.
     */
    void removeCanvasItem();

    /**
     * Reset the position of the canvas item (which is assumed to
     * exist already).
     */
    void reposition(double canvasX, double canvasY);

    /**
     * Return true if setCanvasItem has been called more recently
     * than reposition.  If true, any code that positions this
     * element will probably not need to regenerate its sprite as
     * well, even if other indications suggest otherwise.
     */
    bool isRecentlyRegenerated() { return m_recentlyRegenerated; }

    bool isSelected();
    void setSelected(bool selected);

    /**
     * Return true if the element is a note which lies at the exactly
     * same place than another note.
     * Only valid after NotationVLayout::scanStaff() call.
     * Only a returned true is meaningful (when 2 notes are colliding, the
     * first element returns false and the second one returns true).
     */
    bool isColliding() { return m_isColliding; }

    void setIsColliding(bool value) { m_isColliding = value; }

    /// Returns the associated canvas item
    QCanvasItem* getCanvasItem() { return m_canvasItem; }

protected:
    //--------------- Data members ---------------------------------

    double m_airX;
    double m_airWidth;
    bool m_recentlyRegenerated;
    bool m_isColliding;

    QCanvasItem *m_canvasItem;

    typedef std::vector<QCanvasItem *> ItemList;
    ItemList *m_extraItems;
};

typedef ViewElementList NotationElementList;



}

#endif
