
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

#ifndef NOTATIONHLAYOUT_H
#define NOTATIONHLAYOUT_H

#include "layoutengine.h"
#include "quantizer.h"
#include "notationelement.h"
#include "staff.h"


/**
  *@author Guillaume Laurent, Chris Cannam, Rich Bown
  */

class NotationHLayout : public LayoutEngine
{
public:
    NotationHLayout(Staff &staff, NotationElementList& elements);
    ~NotationHLayout();

    void preparse(NotationElementList::iterator from,
                  NotationElementList::iterator to);
    void layout();

    struct BarPosition
    {
        NotationElementList::iterator start; // i.e. event following barline
        Event::timeT time;    // absolute time of event at "start"
        int x;                // coordinate for display
        int width;            // theoretical width
        bool fixed;           // user-supplied new-bar or timesig event?
        bool correct;         // false if preceding bar has incorrect duration
        
        BarPosition(NotationElementList::iterator istart,
                    Event::timeT itime, int ix, int iwidth,
                    bool ifixed, bool icorrect) :
            start(istart), time(itime), x(ix), width(iwidth),
            fixed(ifixed), correct(icorrect) { }
    };

    typedef list<BarPosition> BarPositions;

    /// returns the bar positions computed from the last call to layout()
    BarPositions& getBarPositions();
    const BarPositions& getBarPositions() const;

    /// resets the internal position counters of the object
    void reset();

    Quantizer& quantizer() { return m_quantizer; }

protected:

    /*
     * Breaks down a note which doesn't fit in a bar into shorter notes - disabled for now
     */
    //     const vector<unsigned int>& splitNote(unsigned int noteLen);
    void addNewBar(NotationElementList::iterator start,
                   Event::timeT time, int x, int width, bool, bool);

    /// returns the note immediately before 'pos'
    NotationElementList::iterator getPreviousNote(NotationElementList::iterator pos);

    Staff &m_staff;
    Quantizer m_quantizer;
    NotationElementList& m_notationElements;

    unsigned int m_barWidth;
    unsigned int m_barMargin;
    unsigned int m_noteMargin;

    /// maps note types (Whole, Half, etc...) to the width they should take on the bar
    //!!! this will need to be more general as it depends on time sig &c
    // for now we do this:
    int getNoteWidth(Note::Type type, bool dotted = false) {
        return (m_barWidth * Note(type, dotted).getDuration()) /
            Note(Note::WholeNote, false).getDuration();
    }

    BarPositions m_barPositions;
};

// Looks like we don't need this at the moment but I'd rather keep it around just in case
class ElementHPos
{
public:
    ElementHPos(unsigned int p=0) : pos(p) {}
    ElementHPos(unsigned int p, NotationElementList::iterator i) : pos(p), it(i) {}

    ElementHPos& operator=(const ElementHPos &h) { pos = h.pos; it = h.it; return *this; }

    unsigned int pos;
    NotationElementList::iterator it;
};

inline bool operator<(const ElementHPos &h1, const ElementHPos &h2) { return h1.pos < h2.pos; }
inline bool operator>(const ElementHPos &h1, const ElementHPos &h2) { return h1.pos > h2.pos; }
inline bool operator==(const ElementHPos &h1, const ElementHPos &h2) { return h1.pos == h2.pos; }


#endif
