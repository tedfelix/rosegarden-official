// -*- c-basic-offset: 4 -*-

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
#include "FastVector.h"

/**
 * Horizontal notation layout
 *
 * computes the X coordinates of notation elements
 */

class NotationHLayout : public HorizontalLayoutEngine
{
public:
    NotationHLayout();
    virtual ~NotationHLayout();

    /**
     * Precomputes layout data for a single staff.  The resulting data
     * is stored in the BarDataMap, keyed from the staff reference;
     * the entire map is then used by reconcileBars() and layout().
     * The map should be cleared (by calling reset()) before a full
     * set of staffs is preparsed.
     */
    virtual void scanStaff(Staff &staff);

    /**
     * Resets internal data stores, notably the BarDataMap that is
     * used to retain the data computed by scanStaff().
     */
    virtual void reset();

    /**
     * Resets internal data stores, notably the given staff's entry
     * in the BarDataMap used to retain the data computed by scanStaff().
     */
    virtual void resetStaff(Staff &staff);

    /**
     * Lays out all staffs that have been scanned
     */
    virtual void finishLayout();

    /**
     * Returns the total length of all elements once layout is done
     * This is the x-coord of the end of the last element on the longest staff
     */
    virtual double getTotalWidth() { return m_totalWidth; }

    /**
     * Returns the total number of bar lines on the given staff
     */
    virtual unsigned int getBarLineCount(Staff &staff);

    /**
     * Returns the x-coordinate of the given bar number (zero-based)
     * on the given staff
     */
    virtual double getBarLineX(Staff &staff, unsigned int barNo);

    /**
     * Returns the number that should be displayed next to the
     * specified bar line, if we're showing numbers
     */
    virtual int getBarLineDisplayNumber(Staff &staff, unsigned int barNo);

    /**
     * Returns true if the specified bar line is in the right place,
     * i.e. if the bar preceding it has the correct length
     */
    virtual bool isBarLineCorrect(Staff &staff, unsigned int barNo);

protected:
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
	bool correct;         // bar preceding barline has correct duration
        bool needsLayout;
        
        BarData(int ibarno, NotationElementList::iterator istart,
                int ix, int iwidth, int fwidth, bool icorrect) :
            barNo(ibarno), start(istart), x(ix), idealWidth(iwidth),
            fixedWidth(fwidth), correct(icorrect), needsLayout(true) { }
    };

    typedef FastVector<BarData> BarDataList;
    typedef std::map<Staff *, BarDataList> BarDataMap;

    BarDataMap m_barData;

    /**
     * Returns the bar positions for a given staff, provided that
     * staff has been preparsed since the last reset
     */
    BarDataList& getBarData(Staff &staff);
    const BarDataList& getBarData(Staff &staff) const;

    /// Tries to harmonize the bar positions for all the staves
    void reconcileBars();

    void layout(BarDataMap::iterator);

    void addNewBar
    (Staff &staff, int barNo, NotationElementList::iterator start,
     int width, int fwidth, bool correct);

    int getIdealBarWidth
    (Staff &staff, int fixedWidth, NotationElementList::iterator shortest,
     const NotePixmapFactory &npf, int shortCount, int totalCount,
     const Rosegarden::TimeSignature &timeSignature) const;

    long positionRest
    (Staff &staff, const NotePixmapFactory &npf,
     const NotationElementList::iterator &, const BarDataList::iterator &,
     const Rosegarden::TimeSignature &);

    long positionNote
    (Staff &staff, const NotePixmapFactory &npf,
     const NotationElementList::iterator &, const BarDataList::iterator &,
     const Rosegarden::TimeSignature &, const Rosegarden::Clef &clef,
     const Rosegarden::Key &key, bool &haveAccidentalInThisChord);

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


    int getMinWidth(const NotePixmapFactory &, const NotationElement &) const;

    int getComfortableGap(const NotePixmapFactory &npf,
                          Rosegarden::Note::Type type) const;

    double m_totalWidth;
};

#endif
