
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

    //!!! hardly ideal interface, as it means the calling code and
    //this code must both look up the bar position list even though
    //probably only one of us needs it
    void preparse(Staff &staff, int firstBar, int lastBar);

    //!!! This one needs to work across more than one staff, and use
    //data collated from preparses of all those staffs.  (Preparse
    //should be okay working on one staff at a time.)
    void layout(Staff &staff);

    /**
     * Compute bar data and cached properties.
     */
    void preparse(const Rosegarden::Track::BarPositionList &barPositions,
                  int firstBar, int lastBar);

    /**
     * Performs the final layout of all notation elements
     */
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

    /// Returns the bar positions computed from the last call to preparse()
    BarDataList& getBarData() { return m_barData; }
    const BarDataList& getBarData() const { return m_barData; }

    /// Resets the internal position counters of the object
    void reset();

    //!!! dubious.
    /**
     * Returns the total length of all elements once layout is done
     *
     * This is simply the X position of the last element.
     */
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

    int getMinWidth(const NotePixmapFactory &, const NotationElement &) const;

    int getComfortableGap(const NotePixmapFactory &npf,
                          Rosegarden::Note::Type type) const;
    int getIdealBarWidth(Staff &staff, int fixedWidth,
                         NotationElementList::iterator shortest,
                         const NotePixmapFactory &npf, int shortCount,
                         int totalCount,
                         const Rosegarden::TimeSignature &timeSignature) const;

    BarDataList m_barData;

    double m_totalWidth;

    Staff *m_lastStaffPreparsed; //!!! Because we haven't finished implementing the general case yet
};

#endif
