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

#ifndef MATRIXHLAYOUT_H
#define MATRIXHLAYOUT_H

#include "LayoutEngine.h"
#include "matrixelement.h"

#include "FastVector.h"

class MatrixHLayout : public Rosegarden::HorizontalLayoutEngine<MatrixElement>
{
public:
    MatrixHLayout(Rosegarden::Composition *c);
    virtual ~MatrixHLayout();

    /**
     * Resets internal data stores for all staffs
     */
    virtual void reset();

    /**
     * Resets internal data stores for a specific staff
     */
    virtual void resetStaff(StaffType &staff,
			    Rosegarden::timeT = 0,
			    Rosegarden::timeT = 0);

    /**
     * Returns the total length of all elements once layout is done.
     * This is the x-coord of the end of the last element on the
     * longest staff
     */
    virtual double getTotalWidth();

    /**
     * Returns the number of the first visible bar line
     */
    virtual int getFirstVisibleBar();

    /**
     * Returns the number of the first visible bar line
     */
    virtual int getLastVisibleBar();

    /**
     * Returns the x-coordinate of the given bar number
     */
    virtual double getBarPosition(int barNo);

    /**
     * Precomputes layout data for a single staff, updating any
     * internal data stores associated with that staff and updating
     * any layout-related properties in the events on the staff's
     * segment.
     */
    virtual void scanStaff(StaffType&,
			   Rosegarden::timeT = 0,
			   Rosegarden::timeT = 0);

    /**
     * Computes any layout data that may depend on the results of
     * scanning more than one staff.  This may mean doing most of
     * the layout (likely for horizontal layout) or nothing at all
     * (likely for vertical layout).
     */
    virtual void finishLayout(Rosegarden::timeT = 0,
			      Rosegarden::timeT = 0);

    /**
     * Returns a pointer to a time signature event if there is one
     * visible in this bar, and if so also sets timeSigX to its x-coord
     */
    virtual Rosegarden::Event *getTimeSignaturePosition
    (StaffType &staff, int barNo, double &timeSigX);

protected:

    //--------------- Data members ---------------------------------

    // pair of layout-x and time-signature event if there is one
    typedef std::pair<double, Rosegarden::Event *> BarData;
    typedef FastVector<BarData> BarDataList;
    BarDataList m_barData;
    double m_totalWidth;
    int m_firstBar;
};

#endif
