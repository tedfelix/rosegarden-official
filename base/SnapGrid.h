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

#ifndef _SNAP_GRID_H_
#define _SNAP_GRID_H_

#include "RulerScale.h"

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
     * x-coordinate mappings and the given vstep for y-coords.
     * If vstep is zero, y-coords are not snapped at all.
     */
    SnapGrid(RulerScale *rulerScale, int vstep = 0);

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
     * Return the snap size of the grid, at the given time.  (The time
     * is required in case the built-in snap size is SnapToBar,
     * SnapToBeat or SnapToUnit, in which case we need to know the
     * current time signature.)  Returns zero for NoSnap.
     */
    timeT getSnapTime(timeT t) const;

    /**
     * Snap a given x-coordinate to the nearest time on
     * the grid.  Of course this also does x-to-time
     * conversion, so it's useful even in NoSnap mode.
     * If the snap time is greater than the bar duration
     * at this point, the bar duration will be used instead.
     *
     * If d is SnapLeft or SnapRight, a time to the left or
     * right respectively of the given coordinate will be
     * returned; otherwise the nearest time on either side
     * will be returned.
     */
    timeT snapX(double x, SnapDirection d = SnapEither) const;

    /**
     * Snap a given time to the nearest time on the grid.
     * Unlike snapX, this is not useful in NoSnap mode.
     * If the snap time is greater than the bar duration
     * at this point, the bar duration will be used instead.
     *
     * If d is SnapLeft or SnapRight, a time to the left or
     * right respectively of the given coordinate will be
     * returned; otherwise the nearest time on either side
     * will be returned.
     */
    timeT snapTime(timeT t, SnapDirection d = SnapEither) const;

    /**
     * Snap a given y-coordinate to the nearest lower
     * multiple of the vstep.
     */
    int snapY(int y) const {
	if (m_vstep == 0) return y;
	else return y / m_vstep * m_vstep;
    }

    /**
     * Return the vstep bin number for the given y-coordinate.
     */
    int getYBin(int y) const {
	if (m_vstep == 0) return y;
	else return y / m_vstep;
    }

    /**
     * Return the y-coordinate of the grid line at the start
     * of the given vstep bin.
     */
    int getYBinCoordinate(int bin) const {
	if (m_vstep == 0) return bin;
	else return bin * m_vstep;
    }

    int getYSnap() const {
	return m_vstep;
    }

    RulerScale *getRulerScale() {
	return m_rulerScale;
    }

protected:
    RulerScale *m_rulerScale; // I don't own this
    timeT m_snapTime;
    int m_vstep;
};

}

#endif
