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

#include "RealTime.h"

namespace Rosegarden {

RealTime::RealTime(int s, int u) :
    sec(s), usec(u)
{
    if (sec == 0) {
	while (usec <= -1000000) { usec += 1000000; --sec; }
	while (usec >=  1000000) { usec -= 1000000; ++sec; }
    } else if (sec < 0) {
	while (usec <= -1000000) { usec += 1000000; --sec; }
	while (usec > 0)         { usec -= 1000000; ++sec; }
    } else { 
	while (usec >= 1000000)  { usec -= 1000000; ++sec; }
	while (usec < 0)         { usec += 1000000; --sec; }
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
    int u = (rt.usec < 0 ? -rt.usec : rt.usec);

    out << s << ".";

    int uu(u);
    if (uu == 0) out << "00000";
    else while (uu < 100000) {
	out << "0";
	uu *= 10;
    }
    
    out << u << "R";
    return out;
}

RealTime
RealTime::operator/(int d) const
{
    return RealTime(sec / d, (usec + 1000000 * (sec % d)) / d);
}

double 
RealTime::operator/(const RealTime &r) const
{
    double lTotal = double(sec) * 1000000.0 + double(usec);
    double rTotal = double(r.sec) * 1000000.0 + double(r.usec);
    
    if (rTotal == 0) return 0.0;
    else return lTotal/rTotal;
}

const RealTime RealTime::zeroTime(0,0);

}
