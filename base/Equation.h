// -*- c-basic-offset: 4 -*-


/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#ifndef _EQUATION_H_
#define _EQUATION_H_

namespace Rosegarden {

/**
 * Equation solving helper class
 */
class Equation
{
public:
    enum Unknown { Y, M, X, C };

    struct Point {
	Point(int xx, int yy) : x(xx), y(yy) { }
	int x;
	int y;
    };

    static void solve(Unknown u, double &y, double &m, double &x, double &c);
    static void solve(Unknown u, int &y, double &m, int &x, int &c);

    static void solveForYByEndPoints(Point a, Point b, double x, double &y);
    static void solveForYByEndPoints(Point a, Point b, int x, int &y);
};

}

#endif
