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

#include "notationelement.h"
#include "notepixmapfactory.h"

#include "Staff.h"
#include "LayoutEngine.h"
#include "FastVector.h"

#include <vector>


/**
 * Horizontal notation layout
 *
 * computes the X coordinates of notation elements
 */

class NotationHLayout : public Rosegarden::HorizontalLayoutEngine<NotationElement>
{
public:
    NotationHLayout(NotePixmapFactory &npf);
    virtual ~NotationHLayout();

    /**
     * Precomputes layout data for a single staff.  The resulting data
     * is stored in the BarDataMap, keyed from the staff reference;
     * the entire map is then used by reconcileBars() and layout().
     * The map should be cleared (by calling reset()) before a full
     * set of staffs is preparsed.
     */
    virtual void scanStaff(StaffType &staff);

    /**
     * Resets internal data stores, notably the BarDataMap that is
     * used to retain the data computed by scanStaff().
     */
    virtual void reset();

    /**
     * Resets internal data stores, notably the given staff's entry
     * in the BarDataMap used to retain the data computed by scanStaff().
     */
    virtual void resetStaff(StaffType &staff);

    /**
     * Lays out all staffs that have been scanned
     */
    virtual void finishLayout();

    /**
     * Set page mode
     */
    virtual void setPageMode(bool pageMode) { m_pageMode = pageMode; }

    /**
     * Get the page mode setting
     */
    virtual bool getPageMode() { return m_pageMode; }

    /**
     * Set a page width
     */
    virtual void setPageWidth(double pageWidth) { m_pageWidth = pageWidth; }

    /**
     * Get the page width
     */
    virtual double getPageWidth() { return m_pageWidth; }

    /**
     * Gets the current spacing factor (1.0 == "normal" spacing)
     */
    double getSpacing() const { return m_spacing; }

    /**
     * Sets the current spacing factor (1.0 == "normal" spacing)
     */
    void setSpacing(double spacing) { m_spacing = spacing; }

    /**
     * Gets the range of "standard" spacing factors (you can
     * setSpacing() to anything you want, but it makes sense to
     * have a standard list for GUI use).  The only guaranteed
     * property of the returned list is that 1.0 will be in it.
     */
    static std::vector<double> getAvailableSpacings();

    /**
     * Returns the total length of all elements once layout is done
     * This is the x-coord of the end of the last element on the longest
     * staff, plus the space allocated to that element
     */
    virtual double getTotalWidth() { return m_totalWidth; }

    /**
     * Returns the total number of bar lines on the given staff
     */
    virtual unsigned int getBarLineCount(StaffType &staff);

    /**
     * Returns the x-coordinate of the given bar number (zero-based)
     * on the given staff
     */
    virtual double getBarLineX(StaffType &staff, unsigned int barNo);

    /**
     * Returns the number that should be displayed next to the
     * specified bar line, if we're showing numbers
     */
    virtual int getBarLineDisplayNumber(StaffType &staff, unsigned int barNo);

    /**
     * Returns true if the specified bar line is in the right place,
     * i.e. if the bar preceding it has the correct length
     */
    virtual bool isBarLineCorrect(StaffType &staff, unsigned int barNo);

    /**
     * Returns a pointer to a time signature event if there is one in
     * this bar, and if so also sets timeSigX to its x-coord
     */
    virtual Rosegarden::Event *getTimeSignatureInBar
    (StaffType &staff, unsigned int barNo, int &timeSigX);

protected:
    /**
     * Inner class for bar data, used by scanStaff()
     */
    struct BarData
    {
        int barNo;            // of corresponding BarPosition in Segment
        NotationElementList::iterator start; // i.e. event following barline
        double x;             // coordinate for display of barline
        double idealWidth;    // theoretical width of bar following barline
        int fixedWidth;       // width of non-note items in bar
	int baseWidth;        // minimum width of note items in bar
	bool correct;         // bar preceding barline has correct duration
        bool needsLayout;
	Rosegarden::Event *timeSignature; // or zero if no new one in this bar
	int timeSigX;
        
        BarData(int ibarno, NotationElementList::iterator istart,
                double ix, double iwidth, int fwidth, int bwidth,
		bool icorrect, Rosegarden::Event *timesig) :
            barNo(ibarno), start(istart), x(ix), idealWidth(iwidth),
            fixedWidth(fwidth), baseWidth(bwidth), 
	    correct(icorrect), needsLayout(true),
	    timeSignature(timesig), timeSigX(0) { }
    };

    typedef FastVector<BarData> BarDataList;
    typedef std::map<StaffType *, BarDataList> BarDataMap;

    /**
     * Returns the bar positions for a given staff, provided that
     * staff has been preparsed since the last reset
     */
    BarDataList& getBarData(StaffType &staff);
    const BarDataList& getBarData(StaffType &staff) const;

    /// Find the staff in which bar "barNo" is widest
    StaffType *getStaffWithWidestBar(int barNo);

    /// Prepends empty fake bars onto staffs that start later than the first
    void fillFakeBars();

    /// Tries to harmonize the bar positions for all the staves (linear mode)
    void reconcileBarsLinear();

    /// Tries to harmonize the bar positions for all the staves (page mode)
    void reconcileBarsPage();

    void layout(BarDataMap::iterator);

    void addNewBar
    (StaffType &staff, int barNo, NotationElementList::iterator start,
     double width, int fwidth, int bwidth, bool correct,
     Rosegarden::Event *timesig);

    double getIdealBarWidth
    (StaffType &staff, int fixedWidth, int baseWidth,
     NotationElementList::iterator shortest,
     int shortCount, int totalCount,
     const Rosegarden::TimeSignature &timeSignature) const;

    typedef std::map<int, NotationElementList::iterator> TieMap;

    // This modifies the NotationElementList::iterator passed to it,
    // moving it on to the last note in the chord, and modifies the
    // TieMap as well
    long positionChord
    (StaffType &staff, 
     NotationElementList::iterator &, const BarDataList::iterator &,
     const Rosegarden::TimeSignature &, const Rosegarden::Clef &clef,
     const Rosegarden::Key &key, TieMap &);

    long positionRest
    (StaffType &staff, 
     const NotationElementList::iterator &, const BarDataList::iterator &,
     const Rosegarden::TimeSignature &);

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

    int getMinWidth(NotationElement &) const;
    int getComfortableGap(Rosegarden::Note::Type type) const;
    int getBarMargin() const;
    int getPreBarMargin() const;
    int getPostBarMargin() const;
    int getFixedItemSpacing() const {
	return (int)((m_npf.getNoteBodyWidth() / 5) * m_spacing);
    }

    //--------------- Data members ---------------------------------

    BarDataMap m_barData;

    double m_totalWidth;
    bool m_pageMode;
    double m_pageWidth;
    double m_spacing;
    NotePixmapFactory &m_npf;

    static std::vector<double> m_availableSpacings;
};

#endif
