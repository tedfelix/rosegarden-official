
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

#ifndef _NOTATION_GROUP_H_
#define _NOTATION_GROUP_H_

#include "NotationTypes.h"
#include "notationelement.h"
#include "notepixmapfactory.h"


// A "notation set" is an object made up from a contiguous set of
// elements from an element list.  Examples of notation sets include
// Chord and BeamedGroup.

// To construct a set requires (at least) a NotationElementList
// reference plus an iterator into that list.  The constructor (or
// more precisely the initialise() method called by the constructor)
// then scans the surrounding area of the list for the maximal set of
// contiguous elements before and after the passed-in iterator such
// that all elements are in the same set (i.e. Chord, BeamedGroup etc)
// as the one that the passed-in iterator pointed to.

// The extents of the set within the list can then be discovered via
// getInitialElement() and getFinalElement().  If the iterator passed
// in to the constructor was at end() or did not point to an element
// that could be a member of this kind of set, getInitialElement()
// will return end(); if the passed-in iterator pointed to the only
// member of this set, getInitialElement() and getFinalElement() will
// be equal.

// These classes are not intended to be stored anywhere; all they
// contain is iterators into the main element list, and those might
// not persist.  Instead you should create these on-the-fly when you
// want, for example, to consider a note as part of a chord; and then
// you should let them expire when you've finished with them.


class NotationSet
{
public:
    typedef NotationElementList::iterator NELIterator;

    virtual ~NotationSet() { }

    virtual NELIterator getInitialElement() const  { return m_initial;  }
    virtual NELIterator getFinalElement() const    { return m_final;    }

    virtual NELIterator getLongestElement() const  { return m_longest;  }
    virtual NELIterator getShortestElement() const { return m_shortest; }

    virtual NELIterator getHighestNote() const     { return m_highest;  }
    virtual NELIterator getLowestNote() const      { return m_lowest;   }

protected:
    NotationSet(const NotationElementList &nel, NELIterator elementInSet,
                bool quantize);
    void initialise();

    virtual bool test(const NELIterator &i) = 0;
    virtual void sample(const NELIterator &i);

    const NotationElementList &getList() const { return m_nel; }

private:
    Rosegarden::Event::timeT duration(const NELIterator &i, bool quantized);

    const NotationElementList &m_nel;
    NELIterator m_initial, m_final, m_shortest, m_longest, m_highest, m_lowest;
    bool m_quantized;
    NELIterator m_baseIterator;
};



class Chord : public NotationSet,
              public std::vector<NotationElementList::iterator>
{
public:
    Chord(const NotationElementList &nel, NELIterator elementInChord,
          bool quantized = true);
    virtual ~Chord() { }

protected:
    virtual void sample(const NELIterator &i);
    virtual bool test(const NELIterator &i);

private:
    Rosegarden::Event::timeT m_time;
};



class NotationGroup : public NotationSet
{
public:
    enum Type { Beamed, Tupled, Grace };

    // If the iterator passed in to the constructor points at an
    // element with a GroupNo property, the resulting Group will
    // contain iterators pointing to it and all surrounding elements
    // that have the same GroupNo, sorted in ascending order of
    // absolute time.  If no other surrounding elements have the same
    // GroupNo as this one, we will have size 1; if this iterator
    // doesn't point to an element with a GroupNo at all, we will have
    // size 0.
    
    NotationGroup(const NotationElementList &nel, NELIterator elementInGroup);
    virtual ~NotationGroup() { }

    Type getGroupType() const { return m_type; }
    
    struct Beam
    {                           // if a beam has a line equation y = mx + c,
        double gradient;        // -- then this is m
        double startHeight;     // -- and this is c (in height-on-staff units,
        bool aboveNotes;        //                   relative to 1st notehead)
    };

    Beam calculateBeam(const NotePixmapFactory&,
                       const Rosegarden::Clef&,
                       const Rosegarden::Key&,
                       int width);

protected:
    virtual void sample(const NELIterator &i);
    virtual bool test(const NELIterator &i);

private:
    int height(const NELIterator&,
               const Rosegarden::Clef&,
               const Rosegarden::Key&);

    long m_groupNo;
    Type m_type;
};

#endif
