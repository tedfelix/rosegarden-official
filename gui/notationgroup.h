
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

// Structurally this has a lot in common with Chord; it's just a
// collection of pointers to things in the main list.  Don't store
// these, just create them, query them and throw them away.

class NotationGroup : public vector<NotationElementList::iterator>
{
public:
    typedef NotationElementList::iterator NELIterator;
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
    virtual ~NotationGroup();

    Type getGroupType() { return m_type; }
    
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

private:
    class GroupMembershipTest {
    public:
        GroupMembershipTest(const NELIterator &i) {
            if (!(*i)->event()->get<Rosegarden::Int>("GroupNo", m_groupNo))
                m_groupNo = -1;
        }
        bool operator()(const NELIterator &i) {
            long n;
            return ((*i)->event()->get<Rosegarden::Int>("GroupNo", n) && n == m_groupNo);
        }
    private:
        long m_groupNo;
    };

    int height(const NELIterator&,
               const Rosegarden::Clef&,
               const Rosegarden::Key&);

    const NotationElementList &m_nel;
    Type m_type;
};

#endif
