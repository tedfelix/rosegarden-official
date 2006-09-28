// -*- c-basic-offset: 4 -*-


#ifndef _EQUATION_H_
#define _EQUATION_H_

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

#endif
