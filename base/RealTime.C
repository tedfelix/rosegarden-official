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

#include <iostream>

#if (__GNUC__ < 3)
#include <strstream>
#define stringstream strstream
#else
#include <sstream>
#endif

#include "RealTime.h"

namespace Rosegarden {

// A RealTime consists of two ints that must be at least 32 bits each.
// A signed 32-bit int can store values exceeding +/- 2 billion.  This
// means we can safely use our lower int for nanoseconds, as there are
// 1 billion nanoseconds in a second and we need to handle double that
// because of the implementations of addition etc that we use.
//
// The maximum valid RealTime on a 32-bit system is somewhere around
// 68 years: 999999999 nanoseconds longer than the classic Unix epoch.

#define ONE_BILLION 1000000000

RealTime::RealTime(int s, int n) :
    sec(s), nsec(n)
{
    if (sec == 0) {
	while (nsec <= -ONE_BILLION) { nsec += ONE_BILLION; --sec; }
	while (nsec >=  ONE_BILLION) { nsec -= ONE_BILLION; ++sec; }
    } else if (sec < 0) {
	while (nsec <= -ONE_BILLION) { nsec += ONE_BILLION; --sec; }
	while (nsec > 0)             { nsec -= ONE_BILLION; ++sec; }
    } else { 
	while (nsec >=  ONE_BILLION) { nsec -= ONE_BILLION; ++sec; }
	while (nsec < 0)             { nsec += ONE_BILLION; --sec; }
    }
}


std::ostream &operator<<(std::ostream &out, const RealTime &rt)
{
    if (rt < RealTime::zeroTime) {
	out << "-";
    } else {
	out << " ";
    }

    int s = (rt.sec < 0 ? -rt.sec : rt.sec);
    int n = (rt.nsec < 0 ? -rt.nsec : rt.nsec);

    out << s << ".";

    int nn(n);
    if (nn == 0) out << "00000000";
    else while (nn < (ONE_BILLION / 10)) {
	out << "0";
	nn *= 10;
    }
    
    out << n << "R";
    return out;
}

std::string
RealTime::toString() const
{
    std::stringstream out;
    out << *this;
    
#if (__GNUC__ < 3)
    out << std::ends;
#endif

    std::string s = out.str();

    // remove trailing R
    return s.substr(0, s.length() - 1);
}

RealTime
RealTime::operator/(int d) const
{
    int secdiv = sec / d;
    int secrem = sec % d;

    double nsecdiv = (double(nsec) + ONE_BILLION * double(secrem)) / d;
    
    return RealTime(secdiv, int(nsecdiv + 0.5));
}

double 
RealTime::operator/(const RealTime &r) const
{
    double lTotal = double(sec) * ONE_BILLION + double(nsec);
    double rTotal = double(r.sec) * ONE_BILLION + double(r.nsec);
    
    if (rTotal == 0) return 0.0;
    else return lTotal/rTotal;
}

long
RealTime::realTime2Frame(const RealTime &time, unsigned int sampleRate)
{
    // We like integers.  The last term is always zero unless the
    // sample rate is greater than 1MHz, but hell, you never know...

    long frame =
	time.sec * sampleRate +
	(time.msec() * sampleRate) / 1000 +
	((time.usec() - 1000 * time.msec()) * sampleRate) / 1000000 +
	((time.nsec - 1000 * time.usec()) * sampleRate) / 1000000000;

    return frame;
}

RealTime
RealTime::frame2RealTime(long frame, unsigned int sampleRate)
{
    RealTime rt;
    rt.sec = frame / sampleRate;
    frame -= rt.sec * sampleRate;
    rt.nsec = (int)(((float(frame) * 1000000) / sampleRate) * 1000);
    return rt;
}

const RealTime RealTime::zeroTime(0,0);

}
