
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
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

#ifndef _RG_MATRIXHLAYOUT_H_
#define _RG_MATRIXHLAYOUT_H_

#include "base/FastVector.h"
#include "base/LayoutEngine.h"
#include <utility>
#include "base/Event.h"

#include "gui/general/HZoomable.h"



namespace Rosegarden
{

class TimeSignature;
class Staff;
class Composition;


class MatrixHLayout : public HorizontalLayoutEngine
{
public:
    MatrixHLayout(Composition *c);
    virtual ~MatrixHLayout();

    /**
     * Resets internal data stores for all staffs
     */
    virtual void reset();

    /**
     * Resets internal data stores for a specific staff
     */
    virtual void resetStaff(Staff &staff,
                            timeT = 0,
                            timeT = 0);

    /**
     * Returns the total length of all elements once layout is done.
     * This is the x-coord of the end of the last element on the
     * longest staff
     */
    virtual double getTotalWidth() const;

    /**
     * Returns the number of the first visible bar line
     */
    virtual int getFirstVisibleBar() const;

    /**
     * Returns the number of the first visible bar line
     */
    virtual int getLastVisibleBar() const;

    /**
     * Returns the x-coordinate of the given bar number
     */
    virtual double getBarPosition(int barNo) const;

    /**
     * Precomputes layout data for a single staff, updating any
     * internal data stores associated with that staff and updating
     * any layout-related properties in the events on the staff's
     * segment.
     */
    virtual void scanStaff(Staff&,
                           timeT = 0,
                           timeT = 0);

    /**
     * Computes any layout data that may depend on the results of
     * scanning more than one staff.  This may mean doing most of
     * the layout (likely for horizontal layout) or nothing at all
     * (likely for vertical layout).
     */
    virtual void finishLayout(timeT = 0,
                              timeT = 0);

    /**
     * Returns true if there is a new time signature in the given bar,
     * setting timeSignature appropriately and setting timeSigX to its
     * x-coord
     */
    virtual bool getTimeSignaturePosition(Staff &staff,
                                          int barNo,
                                          TimeSignature &timeSig,
                                          double &timeSigX);

protected:

    //--------------- Data members ---------------------------------

    // pair of has-time-sig and time-sig
    typedef std::pair<bool, TimeSignature> TimeSigData;
    // pair of layout-x and time-signature if there is one
    typedef std::pair<double, TimeSigData> BarData;
    typedef FastVector<BarData> BarDataList;
    BarDataList m_barData;
    double m_totalWidth;
    int m_firstBar;
};

/**
 * "zoomable" version of the above, used in the MatrixView
 * to properly scale Tempo and Chord rulers. 
 * 
 */
class ZoomableMatrixHLayoutRulerScale : public RulerScale, public HZoomable {
public:
    ZoomableMatrixHLayoutRulerScale(MatrixHLayout& layout) : RulerScale(layout.getComposition()), m_referenceHLayout(layout) {};
    
    virtual double getBarPosition(int n)   const { return m_referenceHLayout.getBarPosition(n) * getHScaleFactor(); }
    virtual double getXForTime(timeT time) const { return m_referenceHLayout.getXForTime(time) * getHScaleFactor(); }
    virtual timeT getTimeForX(double x)    const { return m_referenceHLayout.getTimeForX(x / getHScaleFactor()); }
    virtual double getBarWidth(int n)      const { return m_referenceHLayout.getBarWidth(n) * getHScaleFactor(); }
    virtual int getLastVisibleBar()        const { return m_referenceHLayout.getLastVisibleBar(); }

protected:
    MatrixHLayout& m_referenceHLayout;    
};

}

#endif
