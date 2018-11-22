/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
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

struct ROSEGARDENPRIVATE_EXPORT RealTime
{
    int sec;
    int nsec;

    int usec() const { return nsec / 1000; }
    int msec() const { return nsec / 1000000; }

    RealTime(): sec(0), nsec(0) {}
    RealTime(int s, int n);

    RealTime(const RealTime &r) :
        sec(r.sec), nsec(r.nsec) { }

    static RealTime fromSeconds(double sec);
    static RealTime fromMilliseconds(int msec);
    static RealTime fromTimeval(const struct timeval &);

    RealTime &operator=(const RealTime &r) {
        sec = r.sec; nsec = r.nsec; return *this;
    }

    RealTime operator+(const RealTime &r) const {
        return RealTime(sec + r.sec, nsec + r.nsec);
    }
    RealTime operator-(const RealTime &r) const {
        return RealTime(sec - r.sec, nsec - r.nsec);
    }
    RealTime operator-() const {
        return RealTime(-sec, -nsec);
    }

    bool operator <(const RealTime &r) const {
        if (sec == r.sec) return nsec < r.nsec;
        else return sec < r.sec;
    }

    bool operator >(const RealTime &r) const {
        if (sec == r.sec) return nsec > r.nsec;
        else return sec > r.sec;
    }

    bool operator==(const RealTime &r) const {
        return (sec == r.sec && nsec == r.nsec);
    }
 
    bool operator!=(const RealTime &r) const {
        return !(r == *this);
    }
 
    bool operator>=(const RealTime &r) const {
        if (sec == r.sec) return nsec >= r.nsec;
        else return sec >= r.sec;
    }

    bool operator<=(const RealTime &r) const {
        if (sec == r.sec) return nsec <= r.nsec;
        else return sec <= r.sec;
    }

    RealTime operator*(double m) const;
    RealTime operator/(int d) const;

    // Find the fractional difference between times
    //
    double operator/(const RealTime &r) const;

    // Return a human-readable debug-type string to full precision
    // (probably not a format to show to a user directly).  If align
    // is true, prepend " " to the start of positive values so that
    // they line up with negative ones (which start with "-").
    // 
    std::string toString(bool align = false) const;

    // Return a user-readable string to the nearest millisecond
    // in a form like HH:MM:SS.mmm
    //
    std::string toText(bool fixedDp = false) const;

    // Convenience functions for handling sample frames
    //
    static long realTime2Frame(const RealTime &r, unsigned int sampleRate);
    static RealTime frame2RealTime(long frame, unsigned int sampleRate);
    // Convert to frequency per minute.
    double toPerMinute();

    static const RealTime zeroTime;
    static const RealTime beforeMaxTime;
};

std::ostream &operator<<(std::ostream &out, const RealTime &rt);
    
}

#endif
