
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

#ifndef _NOTATION_SETS_H_
#define _NOTATION_SETS_H_

#include "NotationTypes.h"
#include "notationelement.h"
#include "notationproperties.h"
#include "notepixmapfactory.h"

class Staff;


/**
 * A "notation set" is an object made up from a contiguous set of
 * elements from an element list.  Examples of notation sets include
 * the Chord and BeamedGroup.
 *
 * To construct a set requires (at least) a NotationElementList
 * reference plus an iterator into that list.  The constructor (or
 * more precisely the initialise() method called by the constructor)
 * then scans the surrounding area of the list for the maximal set of
 * contiguous elements before and after the passed-in iterator such
 * that all elements are in the same set (i.e. Chord, BeamedGroup etc)
 * as the one that the passed-in iterator pointed to.
 *
 * The extents of the set within the list can then be discovered via
 * getInitialElement() and getFinalElement().  If the iterator passed
 * in to the constructor was at end() or did not point to an element
 * that could be a member of this kind of set, getInitialElement()
 * will return end(); if the passed-in iterator pointed to the only
 * member of this set, getInitialElement() and getFinalElement() will
 * be equal.
 *
 * These classes are not intended to be stored anywhere; all they
 * contain is iterators into the main element list, and those might
 * not persist.  Instead you should create these on-the-fly when you
 * want, for example, to consider a note as part of a chord; and then
 * you should let them expire when you've finished with them.
 */

class NotationSet // abstract base
{
public:
    typedef NotationElementList::iterator NELIterator;

    virtual ~NotationSet() { }

    /**
     * getInitialElement() returns end() if there are no elements in
     * the set.  getInitialElement() == getFinalElement() if there is
     * only one element in the set
     */
    NELIterator getInitialElement() const  { return m_initial;  }
    NELIterator getFinalElement() const    { return m_final;    }

    /// only return note elements; will return end() if there are none
    NELIterator getInitialNote() const;
    NELIterator getFinalNote() const;

    /**
     * only elements with duration > 0 are candidates for shortest and
     * longest; these will return end() if there are no such elements
     */
    NELIterator getLongestElement() const  { return m_longest;  }
    NELIterator getShortestElement() const { return m_shortest; }

    /// these will return end() if there are no note elements in the set
    NELIterator getHighestNote() const     { return m_highest;  }
    NELIterator getLowestNote() const      { return m_lowest;   }

protected:
    NotationSet(const NotationElementList &nel, NELIterator elementInSet,
                bool quantize);
    void initialise();

    virtual bool test(const NELIterator &i) = 0;
    virtual void sample(const NELIterator &i);

    const NotationElementList &getList() const { return m_nel; }

    Rosegarden::timeT durationOf(const NELIterator &i, bool quantized);

private:
    const NotationElementList &m_nel;
    NELIterator m_initial, m_final, m_shortest, m_longest, m_highest, m_lowest;
    bool m_quantized;
    NELIterator m_baseIterator;
};


/**
 * Chord is subclassed from a vector of notation list iterators; this
 * vector contains iterators pointing at all the notes in the chord,
 * in ascending order of pitch.  (You can also track through all the
 * notes in the chord by iterating from getInitialElement() to
 * getFinalElement(), but this will only get them in the order in
 * which they appear in the original notation list.)
 */

class Chord : public NotationSet,
              public std::vector<NotationElementList::iterator>
{
public:
    Chord(const NotationElementList &nel, NELIterator elementInChord,
          const Rosegarden::Clef &clef = Rosegarden::Clef::DefaultClef,
          const Rosegarden::Key &key = Rosegarden::Key::DefaultKey,
          bool quantized = true);

    virtual ~Chord();

    virtual bool hasStalkUp() const;
    virtual bool hasShiftedNoteHeads() const;
    virtual bool isNoteHeadShifted(const NELIterator &itr) const;

protected:
    virtual bool test(const NELIterator&);
    virtual void sample(const NELIterator&);

private:
    int height(const NELIterator&) const;
    const Rosegarden::Clef &m_clef;
    const Rosegarden::Key &m_key;
    Rosegarden::timeT m_time;
};



/// Several sorts of "Beamed Group"

class NotationGroup : public NotationSet
{
public:
    enum Type { Beamed, Tupled, Grace };

    /**
     * If the iterator passed in to the constructor points at an
     * element with a GroupNo property, the resulting Group will
     * contain iterators pointing to it and all surrounding elements
     * that have the same GroupNo, sorted in ascending order of
     * absolute time.  If no other surrounding elements have the same
     * GroupNo as this one, we will have size 1; if this iterator
     * doesn't point to an element with a GroupNo at all, we will have
     * size 0.
     */
    NotationGroup(const NotationElementList &nel, NELIterator elementInGroup,
                  const Rosegarden::Clef &clef, const Rosegarden::Key &key);

    virtual ~NotationGroup();

    Type getGroupType() const { return m_type; }

    /**
     * Writes beam data into each note in the group.  Notes'
     * layout x coordinates must already have been set.
     */
    void applyBeam(Staff &);

protected:
    virtual bool test(const NELIterator &i);
    virtual void sample(const NELIterator &i);

private:
    struct Beam
    {                           // if a beam has a line equation y = mx + c,
        int  gradient;          // -- then this is m*100 (i.e. a percentage)
        int  startY;            // -- and this is c
        bool aboveNotes;
        bool necessary;
    };

    Beam calculateBeam(Staff &);

    int height(const NELIterator&) const;
    const Rosegarden::Clef &m_clef;
    const Rosegarden::Key &m_key;
    int m_weightAbove, m_weightBelow;
    long m_groupNo;
    Type m_type;
};

#endif
