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

#ifndef _LAYOUT_ENGINE_H_
#define _LAYOUT_ENGINE_H_

namespace Rosegarden {

template <class T>
class Staff;


/**
 * Base classes for layout engines.  The intention is that
 * different sorts of renderers (piano-roll, score etc) can be
 * implemented by simply plugging different implementations
 * of Staff and LayoutEngine into a single view class.
 */

template <class T>
class LayoutEngine
{
public: 
    typedef Staff<T> StaffType;

    LayoutEngine();
    virtual ~LayoutEngine();

    /**
     * Resets internal data stores for all staffs
     */
    virtual void reset() = 0;

    /**
     * Resets internal data stores for a specific staff
     */
    virtual void resetStaff(StaffType &staff) = 0;

    /**
     * Precomputes layout data for a single staff, updating any
     * internal data stores associated with that staff and updating
     * any layout-related properties in the events on the staff's
     * segment.
     */
    virtual void scanStaff(StaffType &staff) = 0;

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


template <class T>
class HorizontalLayoutEngine : public LayoutEngine<T>
{
public:
    HorizontalLayoutEngine();
    virtual ~HorizontalLayoutEngine();

    typedef Staff<T> StaffType;

    /**
     * Sets a page width for the layout.
     *
     * A layout implementation does not have to use this.  Some might
     * use it (for example) to ensure that bar lines fall precisely at
     * the right-hand margin of each page.  The computed x-coordinates
     * will still require to be wrapped into lines by the staff or
     * view implementation, however.
     *
     * A width of zero indicates no requirement for division into
     * pages.
     */
    virtual void setPageWidth(double) { /* default: ignore it */ }

    /**
     * Returns the total length of all elements once layout is done.
     * This is the x-coord of the end of the last element on the
     * longest staff
     */
    virtual double getTotalWidth() = 0;

    /**
     * Returns the total number of bar lines on the given staff
     */
    virtual unsigned int getBarLineCount(StaffType &staff) = 0;

    /**
     * Returns the x-coordinate of the given bar number (zero-based)
     * on the given staff
     */
    virtual double getBarLineX(StaffType &staff, unsigned int barNo) = 0;

    /**
     * Returns the number that should be displayed next to the
     * specified bar line, if we're showing numbers
     */
    virtual int getBarLineDisplayNumber(StaffType &, unsigned int barNo) {
        return (int)barNo;
    }

    /**
     * Returns true if the specified bar line should be drawn
     */
    virtual bool isBarLineVisible(StaffType &staff, unsigned int barNo) {
        return getBarLineDisplayNumber(staff, barNo) >= 0;
    }

    /**
     * Returns true if the specified bar line is in the right place,
     * i.e. if the bar preceding it has the correct length
     */
    virtual bool isBarLineCorrect(StaffType &, unsigned int) {
        return true;
    }

    /**
     * Returns true if the specified bar line is at the very start
     * of a row, in a page view (may mean different visual treatment)
     */
    virtual bool isBarLineAtRowStart(StaffType &, unsigned int) {
        return false;
    }
};



template <class T>
class VerticalLayoutEngine : public LayoutEngine<T>
{
public:
    VerticalLayoutEngine();
    virtual ~VerticalLayoutEngine();

    // I don't think we need to add anything here for now
};


template <class T>
LayoutEngine<T>::LayoutEngine() :
    m_status(0)
{
    // empty
}

template <class T>
LayoutEngine<T>::~LayoutEngine()
{
    // empty
}


template <class T>
HorizontalLayoutEngine<T>::HorizontalLayoutEngine() :
    LayoutEngine<T>()
{
    // empty
}

template <class T>
HorizontalLayoutEngine<T>::~HorizontalLayoutEngine()
{
    // empty
}


template <class T>
VerticalLayoutEngine<T>::VerticalLayoutEngine() :
    LayoutEngine<T>()
{
    // empty
}

template <class T>
VerticalLayoutEngine<T>::~VerticalLayoutEngine()
{
    // empty
}


}


#endif
