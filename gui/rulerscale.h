
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
namespace Rosegarden { class Composition; }

/**
 * RulerScale is a base for classes that may be queried by LoopRuler
 * and friends in order to discover the correct x-coordinates for bar
 * lines and bar subdivisions.
 *
 * RulerScale does not contain any methods that relate bar numbers
 * to times, time signature or duration -- those are in Composition.
 *
 * The methods in RulerScale should return extrapolated (but valid)
 * results even when passed a bar number outside the theoretically
 * visible or existant bar range.
 */

class RulerScale
{
public:
    virtual ~RulerScale();
    Rosegarden::Composition *getComposition() { return m_composition; }

    /**
     * Return the number of the first visible bar.
     */
    virtual int getFirstBarNumber();

    /**
     * Return the number of the last visible bar.
     */
    virtual int getLastBarNumber();

    /**
     * Return the x-coordinate at which bar number n starts.
     */
    virtual double getBarPosition(int n) = 0;

    /**
     * Return the width of bar number n.
     */
    virtual double getBarWidth(int n) = 0;

    /**
     * Return the width of each beat subdivision in bar n.
     */
    virtual double getBeatWidth(int n) = 0;

    /**
     * Return the number of the bar containing the given x-coord.
     */
    virtual int getBarForX(double x) = 0;

    /**
     * Return the nearest time value to the given x-coord.
     */
    virtual Rosegarden::timeT getTimeForX(double x) = 0;

    /**
     * Return the x-coord corresponding to the given time value.
     */
    virtual double getXForTime(Rosegarden::timeT time) = 0;

    /**
     * Return the duration corresponding to the given delta-x
     * starting at the given x-coord.
     */
    Rosegarden::timeT getDurationForWidth(double x, double width) {
	return getTimeForX(x + width) - getTimeForX(x);
    }

    /**
     * Return the width corresponding to the given duration
     * starting at the given time.
     */
    double getWidthForDuration(Rosegarden::timeT startTime,
			       Rosegarden::timeT duration) {
	return getXForTime(startTime + duration) - getXForTime(startTime);
    }

protected:
    RulerScale(Rosegarden::Composition *c);
    Rosegarden::Composition *m_composition;
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
    SimpleRulerScale(Rosegarden::Composition *composition,
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
    virtual Rosegarden::timeT getTimeForX(double x);
    virtual double getXForTime(Rosegarden::timeT time);

protected:
    double m_origin;
    double m_ratio;
};


#endif
