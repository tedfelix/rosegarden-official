
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

#ifndef LAYOUTENGINE_H
#define LAYOUTENGINE_H

class Staff;


/**
 * Base classes for layout engines.  The original intention is that
 * different sorts of renderers (piano-roll, score etc) could be
 * implemented by simply plugging different layout classes and pixmap
 * factories into a single view class.  However, because we've just
 * evolved all this code with a single score view, there's some
 * refactoring to be done (mostly in NotationView) before this is
 * really possible.
 */

class LayoutEngine
{
public: 
    LayoutEngine();
    virtual ~LayoutEngine();

    /**
     * Resets internal data stores for all staffs
     */
    virtual void reset() = 0;

    /**
     * Resets internal data stores for a specific staff
     */
    virtual void resetStaff(Staff &staff) = 0;

    /**
     * Precomputes layout data for a single staff, updating any
     * internal data stores associated with that staff and updating
     * any layout-related properties in the events on the staff's
     * track.
     */
    virtual void scanStaff(Staff &staff) = 0;

    /**
     * Computes any layout data that may depend on the results of
     * scanning more than one staff.  This may mean doing most of
     * the layout (likely for horizontal layout) or nothing at all
     * (likely for vertical layout).
     */
    virtual void finishLayout() = 0;

    unsigned int getStatus() const { return m_status; }

protected:
    unsigned int m_status;
};



class HorizontalLayoutEngine : public LayoutEngine
{
public:
    HorizontalLayoutEngine();
    virtual ~HorizontalLayoutEngine();

    /**
     * Returns the total length of all elements once layout is done.
     * This is the x-coord of the end of the last element on the
     * longest staff
     */
    virtual double getTotalWidth() = 0;

    /**
     * Returns the total number of bar lines on the given staff
     */
    virtual unsigned int getBarLineCount(Staff &staff) = 0;

    /**
     * Returns the x-coordinate of the given bar number (zero-based)
     * on the given staff
     */
    virtual double getBarLineX(Staff &staff, unsigned int barNo) = 0;

    /**
     * Returns the number that should be displayed next to the
     * specified bar line, if we're showing numbers
     */
    virtual int getBarLineDisplayNumber(Staff &staff, unsigned int barNo) {
        return (int)barNo;
    }

    /**
     * Returns true if the specified bar line should be drawn
     */
    virtual bool isBarLineVisible(Staff &staff, unsigned int barNo) {
        return getBarLineDisplayNumber(staff, barNo) >= 0;
    }

    /**
     * Returns true if the specified bar line is in the right place,
     * i.e. if the bar preceding it has the correct length
     */
    virtual bool isBarLineCorrect(Staff &staff, unsigned int barNo) {
        return true;
    }
};



class VerticalLayoutEngine : public LayoutEngine
{
public:
    VerticalLayoutEngine();
    virtual ~VerticalLayoutEngine();

    // I don't think we need to add anything here for now
};



#endif
