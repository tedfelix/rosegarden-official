
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

#ifndef _RULER_SCALE_H_
#define _RULER_SCALE_H_

#include "Event.h"

namespace Rosegarden {

class Composition;

/**
 * RulerScale is a base for classes that may be queried in order to
 * discover the correct x-coordinates for bar lines and bar
 * subdivisions.
 *
 * RulerScale does not contain any methods that relate bar numbers
 * to times, time signature or duration -- those are in Composition.
 *
 * The methods in RulerScale should return extrapolated (but valid)
 * results even when passed a bar number outside the theoretically
 * visible or existant bar range.
 *
 * Apart from getBarPosition, every method in this class has a
 * default implementation, which should work correctly provided
 * the subclass maintains spacing proportional to time within a
 * bar, but which may not be an efficient implementation for any
 * given subclass.
 * 
 * (Potential to-do: At the moment all our RulerScales are used in
 * contexts where spacing proportional to time within a bar is the
 * only interpretation that makes sense, so this is okay.  In
 * theory though we should probably subclass out these "default"
 * implementations into an intermediate abstract class.)
 */

class RulerScale
{
public:
    virtual ~RulerScale();
    Composition *getComposition() const { return m_composition; }

    /**
     * Return the number of the first visible bar.
     */
    virtual int getFirstVisibleBar();

    /**
     * Return the number of the last visible bar.  (The last
     * visible bar_line_ will be at the end of this bar.)
     */
    virtual int getLastVisibleBar();

    /**
     * Return the x-coordinate at which bar number n starts.
     */
    virtual double getBarPosition(int n) = 0;

    /**
     * Return the width of bar number n.
     */
    virtual double getBarWidth(int n);

    /**
     * Return the width of each beat subdivision in bar n.
     */
    virtual double getBeatWidth(int n);

    /**
     * Return the number of the bar containing the given x-coord.
     */
    virtual int getBarForX(double x);

    /**
     * Return the nearest time value to the given x-coord.
     */
    virtual timeT getTimeForX(double x);

    /**
     * Return the x-coord corresponding to the given time value.
     */
    virtual double getXForTime(timeT time);

    /**
     * Return the duration corresponding to the given delta-x
     * starting at the given x-coord.
     */
    virtual timeT getDurationForWidth(double x, double width);

    /**
     * Return the width corresponding to the given duration
     * starting at the given time.
     */
    virtual double getWidthForDuration(timeT startTime, timeT duration);

    /**
     * Return the width of the entire scale.
     */
    virtual double getTotalWidth();

protected:
    RulerScale(Composition *c);
    Composition *m_composition;
};


/**
 * SimpleRulerScale is an implementation of RulerScale that maintains
 * a strict proportional correspondence between x-coordinate and time.
 */

class SimpleRulerScale : public RulerScale
{
public:
    /**
     * Construct a SimpleRulerScale for the given Composition, with a
     * given origin and x-coord/time ratio.  (For example, a ratio of
     * 10 means that one pixel equals 10 time units.)
     */
    SimpleRulerScale(Composition *composition,
		     double origin, double unitsPerPixel);
    virtual ~SimpleRulerScale();

    double getOrigin() { return m_origin; }
    void   setOrigin(double origin) { m_origin = origin; }

    double getUnitsPerPixel() { return m_ratio; }
    void   setUnitsPerPixel(double ratio) { m_ratio = ratio; }

    virtual double getBarPosition(int n);
    virtual double getBarWidth(int n);
    virtual double getBeatWidth(int n);
    virtual int getBarForX(double x);
    virtual timeT getTimeForX(double x);
    virtual double getXForTime(timeT time);

protected:
    double m_origin;
    double m_ratio;
};


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

    enum SnapDirection { SnapEither, SnapLeft, SnapRight };

    /**
     * Set the snap size of the grid to the given time.
     * The snap time must be positive, or else one of the
     * special constants NoSnap, SnapToBar or SnapToBeat.
     * The default is SnapToBeat.
     */
    void setSnapTime(timeT snap);

    /**
     * Return the snap size of the grid, at the given
     * x-coordinate.  (The x-coordinate is required in
     * case the built-in snap size is SnapToBar or
     * SnapToBeat, in which case we need to know the
     * current time signature.)
     * Returns zero for NoSnap.
     */
    timeT getSnapTime(double x) const;

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
