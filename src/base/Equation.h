/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */


/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_EQUATION_H
#define RG_EQUATION_H

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
