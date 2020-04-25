/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2020 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_REAL_TIME_H
#define RG_REAL_TIME_H

#include <string>

#include <rosegardenprivate_export.h>

struct timeval;

namespace Rosegarden 
{

constexpr int nanoSecondsPerSecond = 1000000000;

/// 64-bit time with nanosecond precision.
/**
 * A RealTime consists of two ints that must be at least 32 bits each.
 * A signed 32-bit int can store values exceeding +/- 2 billion.  This
 * means we can safely use our lower int for nanoseconds, as there are
 * 1 billion nanoseconds in a second and we need to handle double that
 * because of the implementations of addition etc that we use.
 *
 * The maximum valid RealTime on a 32-bit system is somewhere around
 * 68 years: 999999999 nanoseconds longer than the classic Unix epoch.
 */
struct ROSEGARDENPRIVATE_EXPORT RealTime
{
    RealTime() : sec(0), nsec(0)  { }
    RealTime(int s, int n);

    int sec;
    int nsec;

    static const RealTime zeroTime;

    // Conversions

    int usec() const  { return nsec / 1000; }
    int msec() const  { return nsec / 1000000; }

    static RealTime fromSeconds(double sec);
    double toSeconds() const
        { return sec + static_cast<double>(nsec) / nanoSecondsPerSecond; }

    static RealTime fromMilliseconds(int msec);
    // ??? Profiler is the only user.  Maybe move it there?
    static RealTime fromTimeval(const struct timeval &);

    // Convenience functions for handling sample frames
    static long realTime2Frame(const RealTime &r, unsigned int sampleRate);
    static RealTime frame2RealTime(long frame, unsigned int sampleRate);

    /// Return "HH:MM:SS.mmm" string to the nearest millisecond.
    std::string toText(bool fixedDp = false) const;

    /// Return a human-readable debug-type string to full precision.
    /**
     * Probably not a format to show to a user directly.  If align
     * is true, prepend " " to the start of positive values so that
     * they line up with negative ones (which start with "-").
     */
    std::string toString(bool align = false) const;

    // Math

    RealTime operator+(const RealTime &r) const
            { return RealTime(sec + r.sec, nsec + r.nsec); }
    RealTime operator-(const RealTime &r) const
            { return RealTime(sec - r.sec, nsec - r.nsec); }
    RealTime operator-() const
            { return RealTime(-sec, -nsec); }
    RealTime operator*(double m) const;
    RealTime operator/(int d) const;

    /// Find the fractional difference between times
    double operator/(const RealTime &r) const;

    // Comparison

    bool operator<(const RealTime &r) const
    {
        if (sec == r.sec)
            return (nsec < r.nsec);
        else
            return (sec < r.sec);
    }

    bool operator>(const RealTime &r) const
    {
        if (sec == r.sec)
            return (nsec > r.nsec);
        else
            return (sec > r.sec);
    }

    bool operator==(const RealTime &r) const
            { return (sec == r.sec  &&  nsec == r.nsec); }
 
    bool operator!=(const RealTime &r) const
            { return !(r == *this); }
 
    bool operator>=(const RealTime &r) const
    {
        if (sec == r.sec)
            return (nsec >= r.nsec);
        else
            return (sec >= r.sec);
    }

    bool operator<=(const RealTime &r) const
    {
        if (sec == r.sec)
            return (nsec <= r.nsec);
        else
            return (sec <= r.sec);
    }

};

// I/O
std::ostream &operator<<(std::ostream &out, const RealTime &rt);

ROSEGARDENPRIVATE_EXPORT QDebug &operator<<(QDebug &, const Rosegarden::RealTime &);

}

#endif
