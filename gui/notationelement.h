
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
class NotationElementList;

/**
 * The Notation H and V layout is performed on a
 * NotationElementList. Once this is done, each NotationElement is
 * affected a QCanvasItem which is set at these coords.
 *
 * @see see NotationView::showElements()
 * @author Guillaume Laurent, Chris Cannam, Rich Bown
 */

class NotationElement : public Rosegarden::ViewElement
{
public:
    struct NoCanvasItem {};
    
    NotationElement(Rosegarden::Event *event);

    ~NotationElement();

    double getLayoutX() { return m_x; }
    double getLayoutY() { return m_y; }

    /// returns the x pos of the associated canvas item
    double getEffectiveX() throw (NoCanvasItem);

    /// returns the y pos of the associated canvas item
    double getEffectiveY() throw (NoCanvasItem);

    void setLayoutX(double x) { m_x = x; }
    void setLayoutY(double y) { m_y = y; }

    /// sets the associated event's note type, note dottedness, and duration
    void setNote(Rosegarden::Note);

    /// returns a Note corresponding to the state of the associated event
    Rosegarden::Note getNote() const;

    bool isRest() const;
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

    QCanvasItem* canvasItem() { return m_canvasItem; }

protected:
    double m_x;
    double m_y;

    QCanvasItem *m_canvasItem;
};


#ifndef NDEBUG
class kdbgstream;
kdbgstream& operator<<(kdbgstream&, NotationElement&);
#else
class kndbgstream;
inline kndbgstream& operator<<(kndbgstream &e, NotationElement&) { return e; }
#endif

class NotationElementCmp
{
public:
    bool operator()(const NotationElement *e1, const NotationElement *e2) const
    {
        return  *e1 < *e2;
    }
};


/**
 * This class owns the NotationElements its items are pointing at
 */
class NotationElementList : public std::multiset<NotationElement*,
                                                 NotationElementCmp>
{
public:
    NotationElementList() : std::multiset<NotationElement*, 
                                          NotationElementCmp>() {};
    ~NotationElementList();

    void erase(iterator pos);
    void erase(iterator from, iterator to);
    void eraseSingle(NotationElement*);

    iterator findPrevious(const std::string &type, iterator i);
    iterator findNext(const std::string &type, iterator i);

    /**
     * Returns an iterator pointing to that specific element,
     * end() otherwise
     */
    iterator findSingle(NotationElement*);

    /**
     * Returns first iterator pointing at or after the given time,
     * end() if time is beyond the end of the track
     */ 
    iterator findTime(Rosegarden::timeT time) const;

private:
};

#ifndef NDEBUG
kdbgstream& operator<<(kdbgstream&, NotationElementList&);
#else
inline kndbgstream& operator<<(kndbgstream &e, NotationElementList&) { return e; }
#endif


#endif
