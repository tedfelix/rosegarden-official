
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

#ifndef _REAL_TIME_H_
#define _REAL_TIME_H_

#include <iostream>


namespace Rosegarden 
{

struct RealTime
{
    long sec;
    long usec;

    RealTime(): sec(0), usec(0) {;}
    RealTime(long s, long u);

    RealTime(const RealTime &r) :
	sec(r.sec), usec(r.usec) { }

    RealTime &operator=(const RealTime &r) {
	sec = r.sec; usec = r.usec; return *this;
    }

    RealTime operator+(const RealTime &r) const {
	return RealTime(sec + r.sec, usec + r.usec);
    }
    RealTime operator-(const RealTime &r) const {
	return RealTime(sec - r.sec, usec - r.usec);
    }

    bool operator <(const RealTime &r) const {
	if (sec == r.sec) return usec < r.usec;
	else return sec < r.sec;
    }

    bool operator >(const RealTime &r) const {
	if (sec == r.sec) return usec > r.usec;
	else return sec > r.sec;
    }

    bool operator==(const RealTime &r) const {
        return (sec == r.sec && usec == r.usec);
    }
 
    bool operator!=(const RealTime &r) const {
        return !(r == *this);
    }
 
    bool operator>=(const RealTime &r) const {
        if (sec == r.sec) return usec >= r.usec;
        else return sec >= r.sec;
    }

    bool operator<=(const RealTime &r) const {
        if (sec == r.sec) return usec <= r.usec;
        else return sec <= r.sec;
    }

    // Find the fractional difference between times
    //
    double operator/(const RealTime &r) const {
        double lTotal = double(sec) * 1000000.0 + double(usec);
        double rTotal = double(r.sec) * 1000000.0 + double(r.usec);

        if (rTotal == 0) return 0.0;
        else return lTotal/rTotal;
    }

};

std::ostream &operator<<(std::ostream &out, const RealTime &rt);
    
}

#endif
