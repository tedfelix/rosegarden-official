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
    case Y: y = static_cast<int>(m*x) + c; break;
    case M: m = static_cast<double>(y - c) / static_cast<double>(x); break;
    case X: x = static_cast<int>(static_cast<float>(y - c) / m); break;
    case C: c = y - static_cast<int>(m*x); break;
    }
}

void Equation::solveForYByEndPoints(Point a, Point b, double x, double &y)
{
     double m, c, y1, x1;

     m = static_cast<double>(b.y - a.y) / static_cast<double>(b.x - a.x);

     x1 = a.x; y1 = a.y;
     solve(C, y1, m, x1, c);
     solve(Y, y, m, x, c);
}

void Equation::solveForYByEndPoints(Point a, Point b, int x, int &y)
{
     double m;
     int c;

     m = static_cast<double>(b.y - a.y) / static_cast<double>(b.x - a.x);

     solve(C, a.y, m, a.x, c);
     solve(Y, y, m, x, c);
}

}
