/*
    Rosegarden-4 v0.2
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

#include <iostream>

#include "RealTime.h"

namespace Rosegarden {

RealTime::RealTime(long s, long u) :
    sec(s), usec(u)
{
    if (sec == 0) {
	while (usec <= -1000000) { usec += 1000000; --sec; }
	while (usec >=  1000000) { usec -= 1000000; ++sec; }
    } else if (sec < 0) {
	while (usec <= -1000000) { usec += 1000000; --sec; }
	while (usec > 0) { usec -= 1000000; ++sec; }
    } else { 
	while (usec >= 1000000) { usec -= 1000000; ++sec; }
	while (usec < 0) { usec += 1000000; --sec; }
    }
}


std::ostream &operator<<(std::ostream &out, const RealTime &rt)
{
    if (rt < RealTime(0, 0)) {
	out << "-";
    } else {
	out << " ";
    }

    long s = (rt.sec < 0 ? -rt.sec : rt.sec);
    long u = (rt.usec < 0 ? -rt.usec : rt.usec);

    out << s << ".";

    long uu(u);
    if (uu == 0) out << "00000";
    else while (uu < 100000) {
	out << "0";
	uu *= 10;
    }
    
    out << u << "R";
    return out;
}

}
