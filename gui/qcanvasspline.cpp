// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
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

/**********************************************************************
** Copyright (C) 1999-2001 Trolltech AS.  All rights reserved.
**
** This file is part of the canvas module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition licenses may use this
** file in accordance with the Qt Commercial License Agreement provided
** with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "qcanvasspline.h"

Q3PointArray::Q3PointArray() : QPointArray() 
{
}

Q3PointArray::Q3PointArray( const QRect &r, bool closed=FALSE )
    : QPointArray(r, closed)
{
}

Q3PointArray::Q3PointArray( int nPoints, const QCOORD *points )
    : QPointArray(nPoints, points)
{
}

// Work functions for QPointArray::cubicBezier()
static
void split(const double *p, double *l, double *r)
{
    double tmpx;
    double tmpy;

    l[0] =  p[0];
    l[1] =  p[1];
    r[6] =  p[6];
    r[7] =  p[7];

    l[2] = (p[0]+ p[2])/2;
    l[3] = (p[1]+ p[3])/2;
    tmpx = (p[2]+ p[4])/2;
    tmpy = (p[3]+ p[5])/2;
    r[4] = (p[4]+ p[6])/2;
    r[5] = (p[5]+ p[7])/2;

    l[4] = (l[2]+ tmpx)/2;
    l[5] = (l[3]+ tmpy)/2;
    r[2] = (tmpx + r[4])/2;
    r[3] = (tmpy + r[5])/2;

    l[6] = (l[4]+ r[2])/2;
    l[7] = (l[5]+ r[3])/2;
    r[0] = l[6];
    r[1] = l[7];
}

// Based on:
//
//   A Fast 2D Point-On-Line Test
//   by Alan Paeth
//   from "Graphics Gems", Academic Press, 1990
static
int pnt_on_line( const int* p, const int* q, const int* t )
{
/*
 * given a line through P:(px,py) Q:(qx,qy) and T:(tx,ty)
 * return 0 if T is not on the line through      <--P--Q-->
 *        1 if T is on the open ray ending at P: <--P
 *        2 if T is on the closed interior along:   P--Q
 *        3 if T is on the open ray beginning at Q:    Q-->
 *
 * Example: consider the line P = (3,2), Q = (17,7). A plot
 * of the test points T(x,y) (with 0 mapped onto '.') yields:
 *
 *     8| . . . . . . . . . . . . . . . . . 3 3
 *  Y  7| . . . . . . . . . . . . . . 2 2 Q 3 3    Q = 2
 *     6| . . . . . . . . . . . 2 2 2 2 2 . . .
 *  a  5| . . . . . . . . 2 2 2 2 2 2 . . . . .
 *  x  4| . . . . . 2 2 2 2 2 2 . . . . . . . .
 *  i  3| . . . 2 2 2 2 2 . . . . . . . . . . .
 *  s  2| 1 1 P 2 2 . . . . . . . . . . . . . .    P = 2
 *     1| 1 1 . . . . . . . . . . . . . . . . .
 *      +--------------------------------------
 *        1 2 3 4 5 X-axis 10        15      19
 *
 * Point-Line distance is normalized with the Infinity Norm
 * avoiding square-root code and tightening the test vs the
 * Manhattan Norm. All math is done on the field of integers.
 * The latter replaces the initial ">= MAX(...)" test with
 * "> (ABS(qx-px) + ABS(qy-py))" loosening both inequality
 * and norm, yielding a broader target line for selection.
 * The tightest test is employed here for best discrimination
 * in merging collinear (to grid coordinates) vertex chains
 * into a larger, spanning vectors within the Lemming editor.
 */

	// if all points are coincident, return condition 2 (on line)
	if(q[0]==p[0] && q[1]==p[1] && q[0]==t[0] && q[1]==t[1]) {
		return 2;
	}

    if ( QABS((q[1]-p[1])*(t[0]-p[0])-(t[1]-p[1])*(q[0]-p[0])) >=
	(QMAX(QABS(q[0]-p[0]), QABS(q[1]-p[1])))) return 0;

    if (((q[0]<p[0])&&(p[0]<t[0])) || ((q[1]<p[1])&&(p[1]<t[1])))
	return 1 ;
    if (((t[0]<p[0])&&(p[0]<q[0])) || ((t[1]<p[1])&&(p[1]<q[1])))
	return 1 ;
    if (((p[0]<q[0])&&(q[0]<t[0])) || ((p[1]<q[1])&&(q[1]<t[1])))
	return 3 ;
    if (((t[0]<q[0])&&(q[0]<p[0])) || ((t[1]<q[1])&&(q[1]<p[1])))
	return 3 ;

    return 2 ;
}
static
void polygonizeQBezier( double* acc, int& accsize, const double ctrl[],
			int maxsize )
{
    if ( accsize > maxsize / 2 )
    {
	// This never happens in practice.

	if ( accsize >= maxsize-4 )
	    return;
	// Running out of space - approximate by a line.
	acc[accsize++] = ctrl[0];
	acc[accsize++] = ctrl[1];
	acc[accsize++] = ctrl[6];
	acc[accsize++] = ctrl[7];
	return;
    }

    //intersects:
    double l[8];
    double r[8];
    split( ctrl, l, r);

    // convert to integers for line condition check
    int c0[2]; c0[0] = int(ctrl[0]); c0[1] = int(ctrl[1]);
    int c1[2]; c1[0] = int(ctrl[2]); c1[1] = int(ctrl[3]);
    int c2[2]; c2[0] = int(ctrl[4]); c2[1] = int(ctrl[5]);
    int c3[2]; c3[0] = int(ctrl[6]); c3[1] = int(ctrl[7]);

    // #### Duplication needed?
    if ( QABS(c1[0]-c0[0]) <= 1 && QABS(c1[1]-c0[1]) <= 1
      && QABS(c2[0]-c0[0]) <= 1 && QABS(c2[1]-c0[1]) <= 1
      && QABS(c3[0]-c1[0]) <= 1 && QABS(c3[1]-c0[1]) <= 1 )
    {
	// Approximate by one line.
	// Dont need to write last pt as it is the same as first pt
	// on the next segment
	acc[accsize++] = l[0];
	acc[accsize++] = l[1];
	return;
    }

    if ( ( pnt_on_line( c0, c3, c1 ) == 2 && pnt_on_line( c0, c3, c2 ) == 2 )
      || ( QABS(c1[0]-c0[0]) <= 1 && QABS(c1[1]-c0[1]) <= 1
	&& QABS(c2[0]-c0[0]) <= 1 && QABS(c2[1]-c0[1]) <= 1
	&& QABS(c3[0]-c1[0]) <= 1 && QABS(c3[1]-c0[1]) <= 1 ) )
    {
	// Approximate by one line.
	// Dont need to write last pt as it is the same as first pt
	// on the next segment
	acc[accsize++] = l[0];
	acc[accsize++] = l[1];
	return;
    }

    // Too big and too curved - recusively subdivide.
    polygonizeQBezier( acc, accsize, l, maxsize );
    polygonizeQBezier( acc, accsize, r, maxsize );
}


Q3PointArray Q3PointArray::cubicBezier() const
{
#ifdef USE_SIMPLE_QBEZIER_CODE
    if ( size() != 4 ) {
#if defined(QT_CHECK_RANGE)
	qWarning( "QPointArray::bezier: The array must have 4 control points" );
#endif
	QPointArray p;
	return p;
    }

    int v;
    float xvec[4];
    float yvec[4];
    for ( v=0; v<4; v++ ) {			// store all x,y in xvec,yvec
	int x, y;
	point( v, &x, &y );
	xvec[v] = (float)x;
	yvec[v] = (float)y;
    }

    QRect r = boundingRect();
    int m = QMAX(r.width(),r.height())/2;
    m = QMIN(m,30);				// m = number of result points
    if ( m < 2 )				// at least two points
	m = 2;
    QPointArray p( m );				// p = Bezier point array
    register QPointData *pd = p.data();

    float x0 = xvec[0],	 y0 = yvec[0];
    float dt = 1.0F/m;
    float cx = 3.0F * (xvec[1] - x0);
    float bx = 3.0F * (xvec[2] - xvec[1]) - cx;
    float ax = xvec[3] - (x0 + cx + bx);
    float cy = 3.0F * (yvec[1] - y0);
    float by = 3.0F * (yvec[2] - yvec[1]) - cy;
    float ay = yvec[3] - (y0 + cy + by);
    float t = dt;

    pd->rx() = (QCOORD)xvec[0];
    pd->ry() = (QCOORD)yvec[0];
    pd++;
    m -= 2;

    while ( m-- ) {
	pd->rx() = (QCOORD)qRound( ((ax * t + bx) * t + cx) * t + x0 );
	pd->ry() = (QCOORD)qRound( ((ay * t + by) * t + cy) * t + y0 );
	pd++;
	t += dt;
    }

    pd->rx() = (QCOORD)xvec[3];
    pd->ry() = (QCOORD)yvec[3];

    return p;
#else

    if ( size() != 4 ) {
#if defined(QT_CHECK_RANGE)
	qWarning( "QPointArray::bezier: The array must have 4 control points" );
#endif
	Q3PointArray pa;
	return pa;
    } else {
	QRect r = boundingRect();
	int m = 4+2*QMAX(r.width(),r.height());
	double *p = new double[m];
	double ctrl[8];
	int i;
	for (i=0; i<4; i++) {
	    ctrl[i*2] = at(i).x();
	    ctrl[i*2+1] = at(i).y();
	}
	int len=0;
	polygonizeQBezier( p, len, ctrl, m );
	Q3PointArray pa((len/2)+1); // one extra point for last point on line
	int j=0;
	for (i=0; j<len; i++) {
	    int x = qRound(p[j++]);
	    int y = qRound(p[j++]);
	    pa[i] = QPoint(x,y);
	}
	// add last pt on the line, which will be at the last control pt
	pa[(int)pa.size()-1] = at(3);
	delete[] p;

	return pa;
    }

#endif
}


/*!
  Create a spline with no control points on the canvas \a canvas.

  \sa setControlPoints()
*/
QCanvasSpline::QCanvasSpline(QCanvas* canvas) :
    QCanvasPolygon(canvas),
//     QCanvasLine(canvas),
    cl(TRUE)
{
}

/*!
  Destroy the spline.
*/
QCanvasSpline::~QCanvasSpline()
{
}

void QCanvasSpline::draw ( QPainter & p )
{
    p.setPen(pen());
    p.setBrush(brush());
    drawShape(p);
}

void QCanvasSpline::drawShape ( QPainter & p )
{
    p.drawPolyline(poly);
}


// ### shouldn't we handle errors more gracefully than with an assert? Lars
/*!
  Set the spline control points to \a ctrl.

  If \a close is TRUE, then the first point in \a ctrl will be re-used
  as the last point, and the number of control points must be a
  multiple of 3.  If \a close is FALSE, one additional control point
  is required, and the number of control points must be one of (4, 7,
  11, ...).

  If the number of control points doesn't meet the above conditions,
  the number of points will be truncated to the largest number of points
  that do meet the requirement.
*/
void QCanvasSpline::setControlPoints(Q3PointArray ctrl, bool close)
{
    if ( (int)ctrl.count() % 3 != (close ? 0 : 1) ) {
	qWarning( "QCanvasSpline::setControlPoints(): Number of points doesn't fit." );
	int numCurves = (ctrl.count() - (close ? 0 : 1 ))/ 3;
	ctrl.resize( numCurves*3 + ( close ? 0 : 1 ) );
    }

    cl = close;
    bez = ctrl;
    recalcPoly();
}

/*!
  Returns the current set of control points.

  \sa setControlPoints(), closed()
*/
Q3PointArray QCanvasSpline::controlPoints() const
{
    return bez;
}

/*!
  Returns whether the control points are considered a closed set.
*/
bool QCanvasSpline::closed() const
{
    return cl;
}

void QCanvasSpline::recalcPoly()
{
    QList<Q3PointArray> segs;
    segs.setAutoDelete(TRUE);
    int n=0;
    for (int i=0; i<(int)bez.count()-1; i+=3) {
	Q3PointArray ctrl(4);
	ctrl[0] = bez[i+0];
	ctrl[1] = bez[i+1];
	ctrl[2] = bez[i+2];
	if ( cl )
	    ctrl[3] = bez[(i+3)%(int)bez.count()];
	else
	    ctrl[3] = bez[i+3];
	Q3PointArray *seg = new Q3PointArray(ctrl.cubicBezier());
	n += seg->count()-1;
	segs.append(seg);
    }
    QPointArray p(n+1);
    n=0;
    for (QPointArray* seg = segs.first(); seg; seg = segs.next()) {
	for (int i=0; i<(int)seg->count()-1; i++)
	    p[n++] = seg->point(i);
	if ( n == (int)p.count()-1 )
	    p[n] = seg->point(seg->count()-1);
    }

    QCanvasPolygon::setPoints(p);
}

int QCanvasSpline::rtti() const { return RTTI; }
int QCanvasSpline::RTTI = 8; //Rtti_Spline;

