// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#include <vector>

#include "notationelement.h"
#include "notepixmapfactory.h"
#include "progressreporter.h"

#include "Staff.h"
#include "LayoutEngine.h"
#include "FastVector.h"
#include "Quantizer.h"

class NotationProperties;
class NotationGroup;
class NotationChord;
namespace Rosegarden { class Progress; }

/**
 * Horizontal notation layout
 *
 * computes the X coordinates of notation elements
 */

class NotationHLayout : public ProgressReporter,
			public Rosegarden::HorizontalLayoutEngine
{
public:
    NotationHLayout(Rosegarden::Composition *c, NotePixmapFactory *npf,
		     const NotationProperties &properties,
		     QObject* parent, const char* name = 0);

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
    virtual void scanStaff(Rosegarden::Staff &staff,
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
    virtual void resetStaff(Rosegarden::Staff &staff,
			    Rosegarden::timeT startTime = 0,
			    Rosegarden::timeT endTime = 0);

    /**
     * Lays out all staffs that have been scanned
     */
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
     * Gets the current spacing factor (100 == "normal" spacing)
     */
    int getSpacing() const { return m_spacing; }

    /**
     * Sets the current spacing factor (100 == "normal" spacing)
     */
    void setSpacing(int spacing) { m_spacing = spacing; }

    /**
     * Gets the range of "standard" spacing factors (you can
     * setSpacing() to anything you want, but it makes sense to
     * have a standard list for GUI use).  The only guaranteed
     * property of the returned list is that 1.0 will be in it.
     */
    static std::vector<int> getAvailableSpacings();

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
    virtual int getFirstVisibleBarOnStaff(Rosegarden::Staff &staff);

    /**
     * Returns the number of the first visible bar line on any
     * staff
     */
    virtual int getFirstVisibleBar();

    /**
     * Returns the number of the last visible bar line on the given
     * staff
     */
    virtual int getLastVisibleBarOnStaff(Rosegarden::Staff &staff);

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
    virtual bool isBarCorrectOnStaff(Rosegarden::Staff &staff, int barNo);

    /**
     * Returns true if there is a new time signature in the given bar,
     * setting timeSignature appropriately and setting timeSigX to its
     * x-coord
     */
    virtual bool getTimeSignaturePosition
    (Rosegarden::Staff &staff, int barNo,
     Rosegarden::TimeSignature &timeSig, double &timeSigX);

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

    struct Chunk {
	Rosegarden::timeT duration;
	short subordering;
	double fixed;
	double stretchy;
	double x;

	Chunk(Rosegarden::timeT d, short sub, double f, double s) :
	    duration(d), subordering(sub), fixed(f), stretchy(s), x(0) { }
	Chunk(short sub, double f) :
	    duration(0), subordering(sub), fixed(f), stretchy(0), x(0) { }
    };
    typedef std::vector<Chunk> ChunkList;

    /**
     * Inner class for bar data, used by scanStaff()
     */
    struct BarData
    {
	struct BasicData
	{   // slots that can be filled at construction time

	    NotationElementList::iterator start; // i.e. event following barline
	    bool correct; // bar preceding barline has correct duration
	    Rosegarden::TimeSignature timeSignature;
	    bool newTimeSig;

	} basicData;

	struct SizeData
	{   // slots that can be filled when the following bar has been scanned

	    ChunkList chunks;
	    double idealWidth;    // theoretical width of bar following barline
	    double reconciledWidth;
	    double fixedWidth;       // width of non-chunk items in bar
//	    int baseWidth;        // minimum width of note items in bar
	    Rosegarden::timeT actualDuration; // may exceed nominal duration

	} sizeData;

	struct LayoutData
	{   // slots either assumed, or only known at layout time
	    bool needsLayout;
	    double x;             // coordinate for display of barline
	    int timeSigX;

	} layoutData;
        
        BarData(NotationElementList::iterator i,
		bool correct, Rosegarden::TimeSignature timeSig, bool newTimeSig) {
            basicData.start = i;
	    basicData.correct = correct;
	    basicData.timeSignature = timeSig;
	    basicData.newTimeSig = newTimeSig;
	    sizeData.idealWidth = 0;
	    sizeData.reconciledWidth = 0;
	    sizeData.fixedWidth = 0;
//	    sizeData.baseWidth = 0;
	    sizeData.actualDuration = 0;
	    layoutData.needsLayout = true;
	    layoutData.x = -1;
	    layoutData.timeSigX = -1;
	}
    };

    typedef std::map<int, BarData> BarDataList;
    typedef BarDataList::value_type BarDataPair;
    typedef std::map<Rosegarden::Staff *, BarDataList> BarDataMap;
    typedef std::map<int, double> BarPositionList;

    typedef std::map<Rosegarden::Staff *, int> StaffIntMap;
    typedef std::map<long, NotationGroup *> NotationGroupMap;

    void clearBarList(Rosegarden::Staff &);


    /**
     * Set the basic data for the given barNo.  If barNo is
     * beyond the end of the existing bar data list, create new
     * records and/or fill with empty ones as appropriate.
     */
    void setBarBasicData(Rosegarden::Staff &staff, int barNo,
			 NotationElementList::iterator start, bool correct,
			 Rosegarden::TimeSignature timeSig, bool newTimeSig);

    /**
     * Set the size data for the given barNo.  If barNo is
     * beyond the end of the existing bar data list, create new
     * records and/or fill with empty ones as appropriate.
     */
    void setBarSizeData(Rosegarden::Staff &staff, int barNo,
			const ChunkList &chunks, double fixedWidth,
			Rosegarden::timeT actualDuration);

    /**
     * Returns the bar positions for a given staff, provided that
     * staff has been preparsed since the last reset
     */
    BarDataList& getBarData(Rosegarden::Staff &staff);
    const BarDataList& getBarData(Rosegarden::Staff &staff) const;

    /// Find the staff in which bar "barNo" is widest
    Rosegarden::Staff *getStaffWithWidestBar(int barNo);

    /// For a single bar, makes sure synchronisation points align in all staves
    void preSquishBar(int barNo);

    /// Tries to harmonize the bar positions for all the staves (linear mode)
    void reconcileBarsLinear();

    /// Tries to harmonize the bar positions for all the staves (page mode)
    void reconcileBarsPage();

    void layout(BarDataMap::iterator,
		Rosegarden::timeT startTime,
		Rosegarden::timeT endTime);
    
    /// Find earliest element with quantized time of t or greater
    NotationElementList::iterator getStartOfQuantizedSlice 
    (const NotationElementList *, Rosegarden::timeT t) const;

    void scanChord
    (NotationElementList *notes, NotationElementList::iterator &i,
     const Rosegarden::Clef &, const Rosegarden::Key &, AccidentalTable &,
     double &lyricWidth, ChunkList &chunks,
     NotationElementList::iterator &to);

    typedef std::map<int, NotationElementList::iterator> TieMap;

    // This modifies the NotationElementList::iterator passed to it,
    // moving it on to the last note in the chord; updates the TieMap;
    // and may modify the to-iterator if it turns out to point at a
    // note within the chord
    void positionChord
    (Rosegarden::Staff &staff, 
     NotationElementList::iterator &, const BarDataList::iterator &,
     const Rosegarden::TimeSignature &, const Rosegarden::Clef &clef,
     const Rosegarden::Key &key, TieMap &, NotationElementList::iterator &to);

    /// Difference between absolute time of next event and of this
    Rosegarden::timeT getSpacingDuration
    (Rosegarden::Staff &staff, const NotationElementList::iterator &);

    /// Difference between absolute time of chord and of first event not in it
    Rosegarden::timeT getSpacingDuration
    (Rosegarden::Staff &staff, const NotationChord &);

    double getLayoutWidth(Rosegarden::ViewElement &) const;

    int getBarMargin() const;
    int getPreBarMargin() const;
    int getPostBarMargin() const;
    int getFixedItemSpacing() const;

    //--------------- Data members ---------------------------------

    BarDataMap m_barData;
    StaffIntMap m_staffNameWidths;
    BarPositionList m_barPositions;
    NotationGroupMap m_groupsExtant;

    double m_totalWidth;
    bool m_pageMode;
    double m_pageWidth;
    int m_spacing;
    NotePixmapFactory *m_npf;

    static std::vector<int> m_availableSpacings;

    const Rosegarden::Quantizer *m_notationQuantizer;
    const NotationProperties &m_properties;

    int m_timePerProgressIncrement;
    int m_staffCount; // purely for progress reporting
};

#endif
