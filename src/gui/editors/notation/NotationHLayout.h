
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_NOTATIONHLAYOUT_H_
#define _RG_NOTATIONHLAYOUT_H_

#include "base/LayoutEngine.h"
#include "base/NotationTypes.h"
#include "NotationElement.h"
#include "gui/general/ProgressReporter.h"
#include <map>
#include <vector>
#include "base/Event.h"


class TieMap;
class QObject;


namespace Rosegarden
{

class ViewElement;
class Staff;
class Quantizer;
class NotePixmapFactory;
class NotationProperties;
class NotationGroup;
class NotationChord;
class Key;
class Composition;
class Clef;
class AccidentalTable;


/**
 * Horizontal notation layout
 *
 * computes the X coordinates of notation elements
 */

class NotationHLayout : public ProgressReporter,
                        public HorizontalLayoutEngine
{
public:
    NotationHLayout(Composition *c,
                    NotePixmapFactory *npf,
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
    virtual void scanStaff(Staff &staff,
                           timeT startTime = 0,
                           timeT endTime = 0);

    /**
     * Resets internal data stores, notably the BarDataMap that is
     * used to retain the data computed by scanStaff().
     */
    virtual void reset();

    /**
     * Resets internal data stores, notably the given staff's entry
     * in the BarDataMap used to retain the data computed by scanStaff().
     */
    virtual void resetStaff(Staff &staff,
                            timeT startTime = 0,
                            timeT endTime = 0);

    /**
     * Lays out all staffs that have been scanned
     */
    virtual void finishLayout(timeT startTime = 0,
                              timeT endTime = 0);

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
     * property of the returned list is that 100 will be in it.
     */
    static std::vector<int> getAvailableSpacings();

    /**
     * Gets the current proportion (100 == spaces proportional to
     * durations, 0 == equal spacings)
     */
    int getProportion() const { return m_proportion; }

    /**
     * Sets the current proportion (100 == spaces proportional to
     * durations, 0 == equal spacings)
     */
    void setProportion(int proportion) { m_proportion = proportion; }

    /**
     * Gets the range of "standard" proportion factors (you can
     * setProportion() to anything you want, but it makes sense to
     * have a standard list for GUI use).  The only guaranteed
     * property of the returned list is that 0, 100, and whatever the
     * default proportion is will be in it.
     */
    static std::vector<int> getAvailableProportions();

    /**
     * Returns the total length of all elements once layout is done
     * This is the x-coord of the end of the last element on the longest
     * staff, plus the space allocated to that element
     */
    virtual double getTotalWidth() const { return m_totalWidth; }

    /**
     * Returns the number of the first visible bar line on the given
     * staff
     */
    virtual int getFirstVisibleBarOnStaff(Staff &staff);

    /**
     * Returns the number of the first visible bar line on any
     * staff
     */
    virtual int getFirstVisibleBar() const;

    /**
     * Returns the number of the last visible bar line on the given
     * staff
     */
    virtual int getLastVisibleBarOnStaff(Staff &staff) const;

    /**
     * Returns the number of the first visible bar line on any
     * staff
     */
    virtual int getLastVisibleBar() const;

    /**
     * Returns the x-coordinate of the given bar number
     */
    virtual double getBarPosition(int barNo) const;

    /**
     * Returns the nearest time value to the given X coord.
     */
    virtual timeT getTimeForX(double x) const;

    /**
     * Returns the X coord corresponding to the given time value.
     * This RulerScale method works by interpolating between bar lines
     * (the inverse of the way getTimeForX works), and should be used
     * for any rulers associated with the layout.
     */
    virtual double getXForTime(timeT time) const;

    /**
     * Returns the X coord corresponding to the given time value.
     * This method works by interpolating between event positions, and
     * should be used for position pointer tracking during playback.
     */
    virtual double getXForTimeByEvent(timeT time) const;

    /**
     * Returns true if the specified bar has the correct length
     */
    virtual bool isBarCorrectOnStaff(Staff &staff, int barNo);

    /**
     * Returns true if there is a new time signature in the given bar,
     * setting timeSignature appropriately and setting timeSigX to its
     * x-coord
     */
    virtual bool getTimeSignaturePosition
    (Staff &staff, int barNo,
     TimeSignature &timeSig, double &timeSigX);

    /// purely optional, used only for progress reporting
    void setStaffCount(int staffCount) {
        m_staffCount = staffCount;
    }

protected:

    struct Chunk {
        timeT duration;
        short subordering;
        float fixed;
        float stretchy;
        float x;

        Chunk(timeT d, short sub, float f, float s) :
            duration(d), subordering(sub), fixed(f), stretchy(s), x(0) { }
        Chunk(short sub, float f) :
            duration(0), subordering(sub), fixed(f), stretchy(0), x(0) { }
    };
    typedef std::vector<Chunk> ChunkList;

    /**
     * Inner class for bar data, used by scanStaff()
     */
    struct BarData
    {
        ChunkList chunks;
        
        struct BasicData
        {   // slots that can be filled at construction time

            NotationElementList::iterator start; // i.e. event following barline
            bool correct; // bar preceding barline has correct duration
            TimeSignature timeSignature;
            bool newTimeSig;

        } basicData;

        struct SizeData
        {   // slots that can be filled when the following bar has been scanned

            float idealWidth;    // theoretical width of bar following barline
            float reconciledWidth;
            float fixedWidth;       // width of non-chunk items in bar
            int clefKeyWidth;
            timeT actualDuration; // may exceed nominal duration

        } sizeData;

        struct LayoutData
        {   // slots either assumed, or only known at layout time
            bool needsLayout;
            double x;             // coordinate for display of barline
            int timeSigX;

        } layoutData;

        BarData(NotationElementList::iterator i,
                bool correct, TimeSignature timeSig, bool newTimeSig) {
            basicData.start = i;
            basicData.correct = correct;
            basicData.timeSignature = timeSig;
            basicData.newTimeSig = newTimeSig;
            sizeData.idealWidth = 0;
            sizeData.reconciledWidth = 0;
            sizeData.fixedWidth = 0;
            sizeData.clefKeyWidth = 0;
            sizeData.actualDuration = 0;
            layoutData.needsLayout = true;
            layoutData.x = -1;
            layoutData.timeSigX = -1;
        }
    };

    typedef std::map<int, BarData> BarDataList;
    typedef BarDataList::value_type BarDataPair;
    typedef std::map<Staff *, BarDataList> BarDataMap;
    typedef std::map<int, double> BarPositionList;

    typedef std::map<Staff *, int> StaffIntMap;
    typedef std::map<long, NotationGroup *> NotationGroupMap;

    void clearBarList(Staff &);


    /**
     * Set the basic data for the given barNo.  If barNo is
     * beyond the end of the existing bar data list, create new
     * records and/or fill with empty ones as appropriate.
     */
    void setBarBasicData(Staff &staff, int barNo,
                         NotationElementList::iterator start, bool correct,
                         TimeSignature timeSig, bool newTimeSig);

    /**
     * Set the size data for the given barNo.  If barNo is
     * beyond the end of the existing bar data list, create new
     * records and/or fill with empty ones as appropriate.
     */
    void setBarSizeData(Staff &staff, int barNo,
                        float fixedWidth, timeT actualDuration);

    /**
     * Returns the bar positions for a given staff, provided that
     * staff has been preparsed since the last reset
     */
    BarDataList& getBarData(Staff &staff);
    const BarDataList& getBarData(Staff &staff) const;

    /// Find the staff in which bar "barNo" is widest
    Staff *getStaffWithWidestBar(int barNo);

    /// Find width of clef+key in the staff in which they're widest in this bar
    int getMaxRepeatedClefAndKeyWidth(int barNo);

    /// For a single bar, makes sure synchronisation points align in all staves
    void preSquishBar(int barNo);

    /// Tries to harmonize the bar positions for all the staves (linear mode)
    void reconcileBarsLinear();

    /// Tries to harmonize the bar positions for all the staves (page mode)
    void reconcileBarsPage();

    void layout(BarDataMap::iterator,
                timeT startTime,
                timeT endTime);
    
    /// Find earliest element with quantized time of t or greater
    NotationElementList::iterator getStartOfQuantizedSlice 
    (NotationElementList *, timeT t) const;

    void scanChord
    (NotationElementList *notes, NotationElementList::iterator &i,
     const Clef &, const ::Rosegarden::Key &,
     AccidentalTable &, float &lyricWidth,
     ChunkList &chunks, NotePixmapFactory *, int ottavaShift,
     NotationElementList::iterator &to);

    typedef std::map<int, NotationElementList::iterator> TieMap;

    // This modifies the NotationElementList::iterator passed to it,
    // moving it on to the last note in the chord; updates the TieMap;
    // and may modify the to-iterator if it turns out to point at a
    // note within the chord
    void positionChord
    (Staff &staff, 
     NotationElementList::iterator &, const Clef &clef,
     const ::Rosegarden::Key &key, TieMap &, NotationElementList::iterator &to);

    void sampleGroupElement
    (Staff &staff, const Clef &clef,
     const ::Rosegarden::Key &key, const NotationElementList::iterator &);

    /// Difference between absolute time of next event and of this
    timeT getSpacingDuration
    (Staff &staff, const NotationElementList::iterator &);

    /// Difference between absolute time of chord and of first event not in it
    timeT getSpacingDuration
    (Staff &staff, const NotationChord &);

    float getLayoutWidth(ViewElement &,
                         NotePixmapFactory *,
                         const ::Rosegarden::Key &) const;

    int getBarMargin() const;
    int getPreBarMargin() const;
    int getPostBarMargin() const;
    int getFixedItemSpacing() const;

    NotePixmapFactory *getNotePixmapFactory(Staff &);
    NotePixmapFactory *getGraceNotePixmapFactory(Staff &);

    //--------------- Data members ---------------------------------

    BarDataMap m_barData;
    StaffIntMap m_staffNameWidths;
    BarPositionList m_barPositions;
    NotationGroupMap m_groupsExtant;

    double m_totalWidth;
    bool m_pageMode;
    double m_pageWidth;
    int m_spacing;
    int m_proportion;
    int m_keySigCancelMode;

    //!!! This should not be here -- different staffs may have
    //different sizes in principle, so we should always be referring
    //to the npf of a particular staff
    NotePixmapFactory *m_npf;

    static std::vector<int> m_availableSpacings;
    static std::vector<int> m_availableProportions;

    const Quantizer *m_notationQuantizer;
    const NotationProperties &m_properties;

    int m_timePerProgressIncrement;
    std::map<Staff *, bool> m_haveOttavaSomewhere;
    int m_staffCount; // purely for progress reporting
};


}

#endif
