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

#ifndef SPLINE_H
#define SPLINE_H

#include "FastVector.h"
#include <qpoint.h>

class Spline
{
public:
    typedef FastVector<QPoint> PointList;

    /**
     * Calculate a set of polyline points to approximate
     * a Bezier spline.  Caller takes ownership of returned
     * heap-allocated container.
     */
    static PointList *calculate(const QPoint &start, const QPoint &finish,
				const PointList &controlPoints,
				QPoint &topLeft, QPoint &bottomRight);

private:
    static void calculateSegment
    (PointList *acc,
     const QPoint &start, const QPoint &finish, const QPoint &control,
     QPoint &topLeft, QPoint &bottomRight);

    static void calculateSegmentSub
    (PointList *acc,
     const QPoint &start, const QPoint &finish, const QPoint &control, int n,
     QPoint &topLeft, QPoint &bottomRight);
};


#endif
