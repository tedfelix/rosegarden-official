
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
#include "notationelement.h"
#include "staff.h"
#include "Track.h"

/**
 * Horizontal notation layout
 *
 * computes the Y coordinate of notation elements
 */
class NotationHLayout : public LayoutEngine
{
public:
    NotationHLayout(Staff &staff, NotationElementList& elements);
    ~NotationHLayout();

    void preparse(const Rosegarden::Track::BarPositionList &barPositions,
		  int firstBar, int lastBar);
    void layout();

    struct BarData
    {
	int barNo;	      // of corresponding BarPosition in Track
        NotationElementList::iterator start; // i.e. event following barline
        int x;                // coordinate for display of barline
        int idealWidth;       // theoretical width of bar following barline
        int fixedWidth;       // minimum possible width of bar following barline
        
        BarData(int ibarno, NotationElementList::iterator istart,
		int ix, int iwidth, int fwidth) :
            barNo(ibarno), start(istart), x(ix), idealWidth(iwidth),
            fixedWidth(fwidth) { }
    };

    typedef std::vector<BarData> BarDataList;

    /// returns the bar positions computed from the last call to preparse()
    BarDataList& getBarData() { return m_barData; }
    const BarDataList& getBarData() const { return m_barData; }

    /// resets the internal position counters of the object
    void reset();

    double getTotalWidth() { return m_totalWidth; }

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
	    
    void addNewBar(int barNo, NotationElementList::iterator start,
                   int width, int fwidth);

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
                         int totalCount,
                         const Rosegarden::TimeSignature &timeSignature) const;

    BarDataList m_barData;

    double m_totalWidth;

};

#endif
