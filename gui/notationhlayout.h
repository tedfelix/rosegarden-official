
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
//#include "quantizer.h"
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
        Rosegarden::timeT time;    // absolute time of event at "start"
        int x;                // coordinate for display
        int idealWidth;       // theoretical width
        int fixedWidth;       // minimum possible width
        bool fixed;           // user-supplied new-bar or timesig event?
        bool correct;         // false if preceding bar has incorrect duration
        
        BarPosition(NotationElementList::iterator istart,
                    Rosegarden::timeT itime,
                    int ix, int iwidth, int fwidth,
                    bool ifixed, bool icorrect) :
            start(istart), time(itime), x(ix), idealWidth(iwidth),
            fixedWidth(fwidth), fixed(ifixed), correct(icorrect) { }
    };

    typedef std::vector<BarPosition> BarPositions;

    /// returns the bar positions computed from the last call to preparse()
    BarPositions& getBarPositions();
    const BarPositions& getBarPositions() const;

    /// resets the internal position counters of the object
    void reset();

protected:
    class AccidentalTable : public std::vector<Rosegarden::Accidental>
    {
    public:
	AccidentalTable(Rosegarden::Key, Rosegarden::Clef);
	Rosegarden::Accidental getDisplayAccidental(Rosegarden::Accidental,
						    int height) const;
	void update(Rosegarden::Accidental, int height);
    private:
	Rosegarden::Key m_key;
	Rosegarden::Clef m_clef;
    };
	    
    void addNewBar(NotationElementList::iterator start,
                   Rosegarden::timeT time,
                   int x, int width, int fwidth,
                   bool, bool);

    /// returns the note immediately before 'pos'
    NotationElementList::iterator getPreviousNote(NotationElementList::iterator pos);

    Staff &m_staff;
    NotationElementList& m_notationElements;

    unsigned int m_barMargin;
    unsigned int m_noteMargin;

    int getMinWidth(const NotePixmapFactory &, const NotationElement &) const;
    int getComfortableGap(const NotePixmapFactory &npf,
                          Rosegarden::Note::Type type) const;
    int getIdealBarWidth(int fixedWidth,
                         NotationElementList::iterator shortest,
                         const NotePixmapFactory &npf, int shortCount,
                         const Rosegarden::TimeSignature &timeSignature) const;

    BarPositions m_barPositions;
};

#ifdef NOT_DEFINED

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

#endif
