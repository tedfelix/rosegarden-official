
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
class NotationElementList : public multiset<NotationElement*, NotationElementCmp>
{
public:
    NotationElementList() : multiset<NotationElement*, NotationElementCmp>() {};
    ~NotationElementList();

    void erase(iterator pos);

    iterator findPrevious(const std::string &type, iterator i);
    iterator findNext(const std::string &type, iterator i);

    // Discovers whether this note is in a chord at some position
    // other than at the end, i.e. it is true if you could construct a
    // Chord object (see below) from the passed iterator and the
    // getFinalNote() method on that object would succeed and return
    // something other than this iterator.  Obvious really
    bool hasSucceedingChordElements(iterator i);

private:
    Quantizer m_quantizer;
};

#ifndef NDEBUG
kdbgstream& operator<<(kdbgstream&, NotationElementList&);
#else
inline kndbgstream& operator<<(kndbgstream &e, NotationElementList&) { return e; }
#endif

#ifdef NOT_DEFINED
// A temporary collection of pointers to things in the list.  Don't
// store Chords, just create them, query them and throw them away.

class Chord : public std::vector<NotationElementList::iterator>
{
public:
    typedef NotationElementList::iterator NELIterator;

    // If the iterator passed in to the constructor points at a note,
    // the resulting Chord will contain iterators pointing to it and
    // all surrounding notes that have the same absolute time, sorted
    // in ascending order of pitch.  If no other surrounding notes
    // have the same absolute time as this one, we will have size 1;
    // if this iterator doesn't point to a note at all, we will have
    // size 0.

    Chord(const NotationElementList &nel, NELIterator noteInChord,
          bool quantized = false);
    virtual ~Chord();

    // These are the initial and final iterators in the chord in the
    // order in which they appear in the original list, not the order
    // in which they appear after sorting in this class

    NELIterator getInitialNote()  { return m_initial;  }
    NELIterator getFinalNote()    { return m_final;    }

    NELIterator getLongestNote()  { return m_longest;  }
    NELIterator getShortestNote() { return m_shortest; }

    NELIterator getHighestNote() {
        if (size() == 0) return m_nel.end();
        else return (*this)[size()-1];
    }

    NELIterator getLowestNote() {
        if (size() == 0) return m_nel.end();
        return (*this)[0];
    }

private:
    void sample(const NELIterator &i);
    Rosegarden::Event::timeT duration(const NELIterator &i, bool quantized);

    class ChordMembershipTest {
    public:
        ChordMembershipTest(Chord &c, const NELIterator &i) :
            m_chord(c),
            m_time((*i)->getAbsoluteTime()) {
        }
        bool operator()(const NELIterator &i) {
            return ((*i)->isNote() && ((*i)->getAbsoluteTime() == m_time));
        }
        void sample(const NELIterator &i) {
            m_chord.sample(i);
        }
    private:
        Chord &m_chord;
        Rosegarden::Event::timeT m_time;
    };

    const NotationElementList &m_nel;
    NELIterator m_initial, m_final, m_shortest, m_longest;
    bool m_quantized;
};
#endif

// inline bool operator<(NotationElement &e1, NotationElement &e2)
// {
//     return e1.m_x < e2.m_x;
// }


#endif
