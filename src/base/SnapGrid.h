/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SNAP_GRID_H
#define RG_SNAP_GRID_H

#include "RulerScale.h"

#include <map>

namespace Rosegarden {

/**
 * SnapGrid is a class that maps x-coordinate onto time, using a
 * RulerScale to get the mapping but constraining the results to a
 * discrete set of suitable times.
 *
 * (It also snaps y-coordinates, but that bit isn't very interesting.)
 */

class SnapGrid
{
public:
    /**
     * Construct a SnapGrid that uses the given RulerScale for
     * x-coordinate mappings and the given ysnap for y-coords.
     * If ysnap is zero, y-coords are not snapped at all.
     */
    SnapGrid(const RulerScale *rulerScale, int ysnap = 0);

    static const timeT NoSnap;
    static const timeT SnapToBar;
    static const timeT SnapToBeat;
    static const timeT SnapToUnit;

    enum SnapDirection { SnapEither, SnapLeft, SnapRight };

    /**
     * Set the snap size of the grid to the given time.
     * The snap time must be positive, or else one of the
     * special constants NoSnap, SnapToBar, SnapToBeat or
     * SnapToUnit.
     * The default is SnapToBeat.
     */
    void setSnapTime(timeT snap);

    /**
     * Return the snap size of the grid, at the given x-coordinate.
     * (The x-coordinate is required in case the built-in snap size is
     * SnapToBar, SnapToBeat or SnapToUnit, in which case we need to
     * know the current time signature.)  Returns zero for NoSnap.
     */
    timeT getSnapTime(double x) const;

    /**
     * Return the snap setting -- the argument that was passed to
     * setSnapTime.  This differs from getSnapTime, which interprets
     * the NoSnap, SnapToBar, SnapToBeat and SnapToUnit settings to
     * return actual timeT values; instead this function returns those
     * actual constants if set.
     */
    timeT getSnapSetting() const;

    /**
     * Return the snap size of the grid, at the given time.  (The time
     * is required in case the built-in snap size is SnapToBar,
     * SnapToBeat or SnapToUnit, in which case we need to know the
     * current time signature.)  Returns zero for NoSnap.
     */
    timeT getSnapTime(timeT t) const;

    /**
     * Snap a given x-coordinate to the nearest time on the grid.  Of
     * course this also does x-to-time conversion, so it's useful even
     * in NoSnap mode.  If the snap time is greater than the bar
     * duration at this point, the bar duration will be used instead.
     *
     * If d is SnapLeft or SnapRight, a time to the left or right
     * respectively of the given coordinate will be returned;
     * otherwise the nearest time on either side will be returned.
     */
    timeT snapX(double x, SnapDirection d = SnapEither) const;

    /**
     * Snap a given time to the nearest time on the grid.  Unlike
     * snapX, this is not useful in NoSnap mode.  If the snap time is
     * greater than the bar duration at this point, the bar duration
     * will be used instead.
     *
     * If d is SnapLeft or SnapRight, a time to the left or right
     * respectively of the given coordinate will be returned;
     * otherwise the nearest time on either side will be returned.
     */
    timeT snapTime(timeT t, SnapDirection d = SnapEither) const;

    /**
     * Snap a given y-coordinate to the nearest lower bin coordinate.
     */
    int snapY(int y) const {
        if (m_ysnap == 0) return y;
	return getYBinCoordinate(getYBin(y));
    }

    /**
     * Return the bin number for the given y-coordinate.
     */
    int getYBin(int y) const;

    /**
     * Return the y-coordinate of the grid line at the start of the
     * given bin.
     */
    int getYBinCoordinate(int bin) const;

    /**
     * Set the default vertical step.  This is used as the height for
     * bins that have no specific height multiple set, and the base
     * height for bins that have a multiple.  Setting the Y snap here
     * is equivalent to specifying it in the constructor.
     */
    void setYSnap(int ysnap) {
	m_ysnap = ysnap;
    }

    /**
     * Retrieve the default vertical step.
     */
    int getYSnap() const {
        return m_ysnap;
    }

    /**
     * Set the height multiple for a specific bin.  The bin will be
     * multiple * ysnap high.  The default is 1 for all bins.
     */
    void setBinHeightMultiple(int bin, int multiple) {
	m_ymultiple[bin] = multiple;
    }
    
    /**
     * Retrieve the height multiple for a bin.
     */
    int getBinHeightMultiple(int bin) {
	if (m_ymultiple.find(bin) == m_ymultiple.end()) return 1;
	return m_ymultiple[bin];
    }

    const RulerScale *getRulerScale() const {
        return m_rulerScale;
    }

protected:
    const RulerScale *m_rulerScale; // I don't own this
    timeT m_snapTime;
    int m_ysnap;
    std::map<int, int> m_ymultiple;
};

}

#endif
