
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

#include "Equation.h"

namespace Rosegarden {

void Equation::solve(Unknown u, double &y, double &m, double &x, double &c)
{
    switch(u) {
    case Y: y = m*x + c; break;
    case M: m = (y - c) / x; break;
    case X: x = (y - c) / m; break;
    case C: c = y - m*x; break;
    }
}

void Equation::solve(Unknown u, int &y, double &m, int &x, int &c)
{
    switch(u) {
    case Y: y = (int)(m*x) + c; break;
    case M: m = (double)(y - c) / (double)x; break;
    case X: x = (int)((float)(y - c) / m); break;
    case C: c = y - (int)(m*x); break;
    }
}

void Equation::solveForYByEndPoints(Point a, Point b, double x, double &y)
{
     double m, c, y1, x1;

     m = (double)(b.y - a.y) / (double)(b.x - a.x);

     x1 = a.x; y1 = a.y;
     solve(C, y1, m, x1, c);
     solve(Y, y, m, x, c);
}

void Equation::solveForYByEndPoints(Point a, Point b, int x, int &y)
{
     double m;
     int c;

     m = (double)(b.y - a.y) / (double)(b.x - a.x);

     solve(C, a.y, m, a.x, c);
     solve(Y, y, m, x, c);
}

}
