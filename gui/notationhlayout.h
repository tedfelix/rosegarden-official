
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
 * computes the X coordinates of notation elements
 */
class NotationHLayout : public LayoutEngine
{
public:
    NotationHLayout();
    ~NotationHLayout();

    /**
     * Precomputes layout data
     *
     * The data is put in BarDataList
     */
    void preparse(Staff &staff, int firstBar = -1, int lastBar = -1);

    /// Tries to harmonize the bar positions for all the staves
    void reconcileBars();

    /// Effectiveley perform layout
    void layout();

    /**
     * Inner class for bar data, used by preparse()
     */
    struct BarData
    {
        int barNo;              // of corresponding BarPosition in Track
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

    /**
     * Returns the bar positions for a given staff, provided that
     * staff has been preparsed since the last reset
     */
    BarDataList& getBarData(Staff &staff);
    const BarDataList& getBarData(Staff &staff) const;

    /**
     * Resets any internal position counters the object may have
     */
    void reset();

    /**
     * Returns the total length of all elements once layout is done
     * This is the x-coord of the end of the last element on the longest staff
     */
    double getTotalWidth() { return m_totalWidth; }

protected:
    typedef std::map<Staff *, BarDataList> BarDataMap;

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

    void layout(BarDataMap::iterator);

    void addNewBar(Staff &staff, int barNo, NotationElementList::iterator start,
                   int width, int fwidth);

    int getMinWidth(const NotePixmapFactory &, const NotationElement &) const;

    int getComfortableGap(const NotePixmapFactory &npf,
                          Rosegarden::Note::Type type) const;

    int getIdealBarWidth(Staff &staff, int fixedWidth,
                         NotationElementList::iterator shortest,
                         const NotePixmapFactory &npf, int shortCount,
                         int totalCount,
                         const Rosegarden::TimeSignature &timeSignature) const;

    BarDataMap m_barData;

    double m_totalWidth;
};

#endif
