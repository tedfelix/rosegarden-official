// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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
#include "Quantizer.h"

#include <vector>

class NotationProperties;
class NotationGroup;
class Chord;
class RosegardenProgressDialog;

/**
 * Horizontal notation layout
 *
 * computes the X coordinates of notation elements
 */

class NotationHLayout : public Rosegarden::HorizontalLayoutEngine<NotationElement>
{
public:
    NotationHLayout(Rosegarden::Composition *c, NotePixmapFactory *npf,
		    Rosegarden::Quantizer *legatoQuantizer,
		    const NotationProperties &properties);
    virtual ~NotationHLayout();

    void setNotePixmapFactory(NotePixmapFactory *npf) {
	m_npf = npf;
    }

    /**
     * Precomputes layout data for a single staff.  The resulting data
     * is stored in the BarDataMap, keyed from the staff reference;
     * the entire map is then used by reconcileBars() and layout().
     * The map should be cleared (by calling reset()) before a full
     * set of staffs is preparsed.
     */
    virtual void scanStaff(StaffType &staff,
			   Rosegarden::timeT startTime = 0,
			   Rosegarden::timeT endTime = 0);

    /**
     * Resets internal data stores, notably the BarDataMap that is
     * used to retain the data computed by scanStaff().
     */
    virtual void reset();

    /**
     * Resets internal data stores, notably the given staff's entry
     * in the BarDataMap used to retain the data computed by scanStaff().
     */
    virtual void resetStaff(StaffType &staff,
			    Rosegarden::timeT startTime = 0,
			    Rosegarden::timeT endTime = 0);

    /**
     * Lays out all staffs that have been scanned
     */
    //!!! shouldn't need these args, should we?
    virtual void finishLayout(Rosegarden::timeT startTime = 0,
			      Rosegarden::timeT endTime = 0);

    /**
     * Set page mode
     */
    virtual void setPageMode(bool pageMode) { m_pageMode = pageMode; }

    /**
     * Get the page mode setting
     */
    virtual bool isPageMode() { return m_pageMode; }

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
     * Returns the number of the first visible bar line on the given
     * staff
     */
    virtual int getFirstVisibleBarOnStaff(StaffType &staff);

    /**
     * Returns the number of the first visible bar line on any
     * staff
     */
    virtual int getFirstVisibleBar();

    /**
     * Returns the number of the last visible bar line on the given
     * staff
     */
    virtual int getLastVisibleBarOnStaff(StaffType &staff);

    /**
     * Returns the number of the first visible bar line on any
     * staff
     */
    virtual int getLastVisibleBar();

    /**
     * Returns the x-coordinate of the given bar number
     */
    virtual double getBarPosition(int barNo);

    /**
     * Returns true if the specified bar has the correct length
     */
    virtual bool isBarCorrectOnStaff(StaffType &staff, int barNo);

    /**
     * Returns a pointer to a time signature event if there is one
     * visible in this bar, and if so also sets timeSigX to its x-coord
     */
    virtual Rosegarden::Event *getTimeSignaturePosition
    (StaffType &staff, int barNo, double &timeSigX);

    void setProgressDialog(RosegardenProgressDialog *dialog) {
	m_progressDlg = dialog;
    }

    /// purely optional, used only for progress reporting
    void setStaffCount(int staffCount) {
	m_staffCount = staffCount;
    }

protected:
    class AccidentalTable
    {
    public:
        AccidentalTable(Rosegarden::Key, Rosegarden::Clef);
	AccidentalTable(const AccidentalTable &);
	AccidentalTable &operator=(const AccidentalTable &);
        Rosegarden::Accidental getDisplayAccidental(Rosegarden::Accidental,
                                                    int height) const;
        void update(Rosegarden::Accidental, int height);
	void copyFrom(const AccidentalTable &); // must have same clef & key

    private:
        Rosegarden::Key m_key;
        Rosegarden::Clef m_clef;
	Rosegarden::Accidental m_accidentals[7];
    };

    /**
     * Inner class for bar data, used by scanStaff()
     */
    struct BarData
    {
	struct BasicData
	{   // slots that can be filled at construction time

	    NotationElementList::iterator start; // i.e. event following barline
	    bool correct; // bar preceding barline has correct duration
	    Rosegarden::Event *timeSignature; // null if no new one in this bar

	} basicData;

	struct SizeData
	{   // slots that can be filled when the following bar has been scanned

	    double idealWidth;    // theoretical width of bar following barline
	    int fixedWidth;       // width of non-note items in bar
	    int baseWidth;        // minimum width of note items in bar
	    Rosegarden::timeT actualDuration; // may exceed nominal duration

	} sizeData;

	struct LayoutData
	{   // slots either assumed, or only known at layout time
	    bool needsLayout;
	    double x;             // coordinate for display of barline
	    int timeSigX;

	} layoutData;
        
        BarData(NotationElementList::iterator i,
		bool correct, Rosegarden::Event *timeSig) {
            basicData.start = i;
	    basicData.correct = correct;
	    basicData.timeSignature = timeSig;
	    sizeData.idealWidth = 0;
	    sizeData.fixedWidth = 0;
	    sizeData.baseWidth = 0;
	    sizeData.actualDuration = 0;
	    layoutData.needsLayout = true;
	    layoutData.x = -1;
	    layoutData.timeSigX = -1;
	}

	~BarData() { delete basicData.timeSignature; }
    };

    typedef std::map<int, BarData> BarDataList;
    typedef BarDataList::value_type BarDataPair;
    typedef std::map<StaffType *, BarDataList> BarDataMap;
    typedef std::map<int, double> BarPositionList;
    typedef std::map<StaffType *, int> StaffIntMap;
    typedef std::map<long, NotationGroup *> NotationGroupMap;

    void clearBarList(StaffType &);

    /**
     * Set the basic data for the given barNo.  If barNo is
     * beyond the end of the existing bar data list, create new
     * records and/or fill with empty ones as appropriate.
     */
    void setBarBasicData(StaffType &staff, int barNo,
			 NotationElementList::iterator start,
			 bool correct, Rosegarden::Event *timeSig);

    /**
     * Set the size data for the given barNo.  If barNo is
     * beyond the end of the existing bar data list, create new
     * records and/or fill with empty ones as appropriate.
     */
    void setBarSizeData(StaffType &staff, int barNo,
			double width, int fixedWidth, int baseWidth,
			Rosegarden::timeT actualDuration);

    /**
     * Returns the bar positions for a given staff, provided that
     * staff has been preparsed since the last reset
     */
    BarDataList& getBarData(StaffType &staff);
    const BarDataList& getBarData(StaffType &staff) const;

    /// Find the staff in which bar "barNo" is widest
    StaffType *getStaffWithWidestBar(int barNo);

    /// Tries to harmonize the bar positions for all the staves (linear mode)
    void reconcileBarsLinear();

    /// Tries to harmonize the bar positions for all the staves (page mode)
    void reconcileBarsPage();

    void layout(BarDataMap::iterator,
		Rosegarden::timeT startTime,
		Rosegarden::timeT endTime);
    
    double getIdealBarWidth
    (StaffType &staff, int fixedWidth, int baseWidth,
     NotationElementList::iterator shortest,
     int shortCount, int totalCount,
     const Rosegarden::TimeSignature &timeSignature) const;

    void legatoQuantize(Rosegarden::Segment &segment);

    /// Find earliest element with quantized time of t or greater
    NotationElementList::iterator getStartOfQuantizedSlice 
    (const NotationElementList *, Rosegarden::timeT t) const;

    void scanChord
    (NotationElementList *notes, NotationElementList::iterator &i,
     const Rosegarden::Clef &, const Rosegarden::Key &, AccidentalTable &,
     int &fixedWidth, int &baseWidth,
     NotationElementList::iterator &shortest, int &shortCount,
     NotationElementList::iterator &to);

    void scanRest
    (NotationElementList *notes, NotationElementList::iterator &i,
     int &fixedWidth, int &baseWidth,
     NotationElementList::iterator &shortest, int &shortCount);

    typedef std::map<int, NotationElementList::iterator> TieMap;

    // This modifies the NotationElementList::iterator passed to it,
    // moving it on to the last note in the chord; updates the TieMap;
    // and may modify the to-iterator if it turns out to point at a
    // note within the chord
    long positionChord
    (StaffType &staff, 
     NotationElementList::iterator &, const BarDataList::iterator &,
     const Rosegarden::TimeSignature &, const Rosegarden::Clef &clef,
     const Rosegarden::Key &key, TieMap &, NotationElementList::iterator &to);

    long positionRest
    (StaffType &staff, 
     const NotationElementList::iterator &, const BarDataList::iterator &,
     const Rosegarden::TimeSignature &);

    /// Difference between absolute time of next event and of this
    Rosegarden::timeT getSpacingDuration
    (StaffType &staff, const NotationElementList::iterator &);

    /// Difference between absolute time of chord and of first event not in it
    Rosegarden::timeT getSpacingDuration
    (StaffType &staff, const Chord &);

    int getMinWidth(NotationElement &) const;
    int getComfortableGap(Rosegarden::Note::Type type) const;
    int getBarMargin() const;
    int getPreBarMargin() const;
    int getPostBarMargin() const;
    int getFixedItemSpacing() const {
	return (int)((m_npf->getNoteBodyWidth() / 5) * m_spacing);
    }

    //--------------- Data members ---------------------------------

    BarDataMap m_barData;
    StaffIntMap m_staffNameWidths;
    BarPositionList m_barPositions;
    NotationGroupMap m_groupsExtant;

    double m_totalWidth;
    bool m_pageMode;
    double m_pageWidth;
    double m_spacing;
    NotePixmapFactory *m_npf;

    static std::vector<double> m_availableSpacings;

    Rosegarden::Quantizer *m_legatoQuantizer;
    const NotationProperties &m_properties;
    RosegardenProgressDialog *m_progressDlg;
    int m_staffCount; // purely for progress reporting
};

#endif
