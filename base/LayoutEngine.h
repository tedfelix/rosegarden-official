// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
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

#ifndef _LAYOUT_ENGINE_H_
#define _LAYOUT_ENGINE_H_

#include "RulerScale.h"

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
     * Resets internal data stores for a specific staff.
     * 
     * If startTime == endTime, act on the whole staff; otherwise only
     * the given section.
     */
    virtual void resetStaff(StaffType &staff,
			    timeT startTime = 0,
			    timeT endTime = 0) = 0;

    /**
     * Precomputes layout data for a single staff, updating any
     * internal data stores associated with that staff and updating
     * any layout-related properties in the events on the staff's
     * segment.
     * 
     * If startTime == endTime, act on the whole staff; otherwise only
     * the given section.
     */
    virtual void scanStaff(StaffType &staff,
			   timeT startTime = 0,
			   timeT endTime = 0) = 0;

    /**
     * Computes any layout data that may depend on the results of
     * scanning more than one staff.  This may mean doing most of
     * the layout (likely for horizontal layout) or nothing at all
     * (likely for vertical layout).
     * 
     * If startTime == endTime, act on the whole staff; otherwise only
     * the given section.
     */
    virtual void finishLayout(timeT startTime = 0,
			      timeT endTime = 0) = 0;

    unsigned int getStatus() const { return m_status; }

protected:
    unsigned int m_status;
};


template <class T>
class HorizontalLayoutEngine : public LayoutEngine<T>,
			       public RulerScale
{
public:
    HorizontalLayoutEngine(Composition *c);
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
     * Returns the number of the first visible bar line on the given
     * staff
     */
    virtual int getFirstVisibleBarOnStaff(StaffType &) {
	return  getFirstVisibleBar();
    }

    /**
     * Returns the number of the last visible bar line on the given
     * staff
     */
    virtual int getLastVisibleBarOnStaff(StaffType &) {
	return  getLastVisibleBar();
    }

    /**
     * Returns true if the specified bar has the correct length
     */
    virtual bool isBarCorrectOnStaff(StaffType &, int/* barNo */) {
        return true;
    }

    /**
     * Returns a pointer to a time signature event if there is one in
     * this bar, and if so also sets timeSigX to its x-coord
     */
    virtual Event *getTimeSignaturePosition
    (StaffType &, int /* barNo */, double &/* timeSigX */) {
	return 0;
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
HorizontalLayoutEngine<T>::HorizontalLayoutEngine(Composition *c) :
    LayoutEngine<T>(),
    RulerScale(c)
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
