
/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
    See the AUTHORS file for more details.

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
    virtual int getFirstVisibleBar() const;

    /**
     * Return the number of the last visible bar.  (The last
     * visible bar_line_ will be at the end of this bar.)
     */
    virtual int getLastVisibleBar() const;

    /**
     * Return the x-coordinate at which bar number n starts.
     */
    virtual double getBarPosition(int n) const = 0;

    /**
     * Return the width of bar number n.
     */
    virtual double getBarWidth(int n) const;

    /**
     * Return the width of each beat subdivision in bar n.
     */
    virtual double getBeatWidth(int n) const;

    /**
     * Return the number of the bar containing the given x-coord.
     */
    virtual int getBarForX(double x) const;

    /**
     * Return the nearest time value to the given x-coord.
     */
    virtual timeT getTimeForX(double x) const;

    /**
     * Return the x-coord corresponding to the given time value.
     */
    virtual double getXForTime(timeT time) const;

    /**
     * Return the duration corresponding to the given delta-x
     * starting at the given x-coord.
     */
    virtual timeT getDurationForWidth(double x, double width) const;

    /**
     * Return the width corresponding to the given duration
     * starting at the given time.
     */
    virtual double getWidthForDuration(timeT startTime, timeT duration) const;

    /**
     * Return the width of the entire scale.
     */
    virtual double getTotalWidth() const;

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

    double getOrigin() const { return m_origin; }
    void   setOrigin(double origin) { m_origin = origin; }

    double getUnitsPerPixel() const { return m_ratio; }
    void   setUnitsPerPixel(double ratio) { m_ratio = ratio; }

    virtual double getBarPosition(int n) const;
    virtual double getBarWidth(int n) const;
    virtual double getBeatWidth(int n) const;
    virtual int getBarForX(double x) const;
    virtual timeT getTimeForX(double x) const;
    virtual double getXForTime(timeT time) const;

protected:
    double m_origin;
    double m_ratio;

private:
    SimpleRulerScale(const SimpleRulerScale &ruler);
    SimpleRulerScale &operator=(const SimpleRulerScale &ruler);
};

}

#endif
