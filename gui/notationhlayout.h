/***************************************************************************
                          notationhlayout.h  -  description
                             -------------------
    begin                : Thu Aug 3 2000
    copyright            : (C) 2000 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef NOTATIONHLAYOUT_H
#define NOTATIONHLAYOUT_H

#include "layoutengine.h"
#include "quantizer.h"
#include "notationelement.h"

/**
  *@author Guillaume Laurent, Chris Cannam, Rich Bown
  */

class NotationHLayout : public LayoutEngine
{
public:
    /**
     * Create a new NotationHLayout object.
     * barWidth is the length of a bar in pixels
     * beatsPerBar is the nb of beats a bar is in the current time sig
     */
    NotationHLayout(NotationElementList& elements,
                    unsigned int barWidth,
                    unsigned int beatsPerBar,
                    unsigned int barMargin,
                    unsigned int noteMargin = 2);

    void layout(NotationElementList::iterator from, NotationElementList::iterator to);

    typedef list<unsigned int> barpositions;

    /// returns the bar positions computed from the last call to layout()
    barpositions& barPositions();
    const barpositions& barPositions() const;

    /// resets the internal position counters of the object
    void reset();

    Quantizer& quantizer() { return m_quantizer; }

protected:

    /*
     * Breaks down a note which doesn't fit in a bar into shorter notes - disabled for now
     */
    //     const vector<unsigned int>& splitNote(unsigned int noteLen);

    void initNoteWidthTable();

    unsigned int barTimeAtPos(NotationElementList::iterator pos);
    void addNewBar(unsigned int barPos);

    Quantizer m_quantizer;

    NotationElementList& m_notationElements;

    unsigned int m_barWidth;
    unsigned int m_timeUnitsPerBar;
    unsigned int m_beatsPerBar;
    unsigned int m_barMargin;
    /// minimal space between two notes
    unsigned int m_noteMargin;

    unsigned int m_nbTimeUnitsInCurrentBar;
    unsigned int m_previousNbTimeUnitsInCurrentBar;
    unsigned int m_previousAbsoluteTime;
    double m_previousPos;
    double m_currentPos;

    typedef vector<unsigned int> NoteWidthTable;

    /// maps note types (Whole, Half, etc...) to the width they should take on the bar
    NoteWidthTable m_noteWidthTable;

    barpositions m_barPositions;

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
