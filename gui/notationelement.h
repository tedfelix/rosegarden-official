
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

#include <multiset.h>
#include "ViewElement.h"
#include "quantizer.h"
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

class NotationElement : public ViewElement
{
public:
    struct NoCanvasItem {};
    
    NotationElement(Event *event);

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
    void setNote(Note);

    /// returns a Note corresponding to the state of the associated event
    Note getNote() const;

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

class kdbgstream;
#ifndef NDEBUG
kdbgstream& operator<<(kdbgstream&, NotationElement&);
#else
inline kndgstream& operator<<(kdbgstream &e, NotationElement&)
{ return e; }
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
class NotationElementList : public multiset<NotationElement*, NotationElementCmp>
{
public:
    NotationElementList() : multiset<NotationElement*, NotationElementCmp>() {};
    ~NotationElementList();

    void erase(iterator pos);

    iterator findPrevious(const string &type, iterator i);
    iterator findNext(const string &type, iterator i);

    // If passed iterator points at a note, return a set of iterators
    // pointing to it and all succeeding notes that have the same
    // absolute time.  If this is the only one, returned vector has
    // size 1; if this is not a note at all, it has size 0.  Results
    // are sorted by pitch, lowest first.  Also returns duration of
    // longest note in chord, and iterator pointing to the last
    // element in the chord (unchanged from value passed in if there's
    // no chord)

    // Be aware that this only finds all notes of a chord if the
    // passed item is the first note in it.
    vector<iterator> findSucceedingChordElements(iterator i,
                                                 iterator &inext,
                                                 Event::timeT &maxDuration,
                                                 bool quantized = false);

    // Discovers whether this note is in a chord (at a position other
    // than the end), i.e. whether findSucceedingChordElements would
    // return a vector of length > 1
    bool hasSucceedingChordElements(iterator i);

private:
    Quantizer m_quantizer;
};

#ifndef NDEBUG
kdbgstream& operator<<(kdbgstream&, NotationElementList&);
#else
inline kndgstream& operator<<(kdbgstream &e, NotationElementList&)
{ return e; }
#endif


// inline bool operator<(NotationElement &e1, NotationElement &e2)
// {
//     return e1.m_x < e2.m_x;
// }


#endif
