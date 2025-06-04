/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "Spline.h"

#include <QPoint>


namespace Rosegarden
{

Spline::PointList *
Spline::calculate(
        const QPoint &start, const QPoint &finish,
        const PointList &controlPoints,
        QPoint &topLeft, QPoint &bottomRight)
{
    if (controlPoints.size() < 2)
        return nullptr;

    PointList *pointAccumulator = new PointList();

    // For very small requests, just return the start and finish points.
    if (abs(start.x() - finish.x()) <= 1  &&
        abs(start.y() - finish.y()) <= 1) {
        topLeft = start;
        bottomRight = finish;
        if (topLeft.x() > bottomRight.x())
            std::swap(topLeft.rx(), bottomRight.rx());
        if (topLeft.y() > bottomRight.y())
            std::swap(topLeft.ry(), bottomRight.ry());

        pointAccumulator->push_back(start);
        pointAccumulator->push_back(finish);
        return pointAccumulator;
    }

    QPoint p(start);

    topLeft = bottomRight = QPoint(0, 0);

    size_t i;

    for (i = 1; i < controlPoints.size(); ++i) {

        QPoint c(controlPoints[i - 1]);

        int x = (c.x() + controlPoints[i].x()) / 2;
        int y = (c.y() + controlPoints[i].y()) / 2;
        QPoint n(x, y);

        calculateSegment(pointAccumulator, p, n, c, topLeft, bottomRight);

        p = n;
    }

    calculateSegment(
            pointAccumulator,
            p, finish,
            controlPoints[i - 1],
            topLeft, bottomRight);

    return pointAccumulator;
}

void
Spline::calculateSegment(PointList *pointAccumulator,
                         const QPoint &start, const QPoint &finish,
                         const QPoint &c,
                         QPoint &topLeft, QPoint &bottomRight)
{
    int x, y, n;

    x = c.x() - start.x();
    y = c.y() - start.y();

    if (x < 0)
        x = -x;
    if (y < 0)
        y = -y;
    if (x > y)
        n = x;
    else
        n = y;

    x = finish.x() - c.x();
    y = finish.y() - c.y();

    if (x < 0)
        x = -x;
    if (y < 0)
        y = -y;
    if (x > y)
        n += x;
    else
        n += y;

    calculateSegmentSub(
            pointAccumulator,
            start, finish, c, n, topLeft, bottomRight);
}

void
Spline::calculateSegmentSub(PointList *pointAccumulator,
                            const QPoint &s, const QPoint &f, const QPoint &c,
                            int n, QPoint &topLeft, QPoint &bottomRight)
{
    double ax = (double)(f.x() + s.x() - 2 * c.x()) / (double)n;
    double ay = (double)(f.y() + s.y() - 2 * c.y()) / (double)n;

    double bx = 2.0 * (double)(c.x() - s.x());
    double by = 2.0 * (double)(c.y() - s.y());

    for (int m = 0; m <= n; ++m) {

        int x = s.x() + (int)((m * ((double)m * ax + bx)) / n);
        int y = s.y() + (int)((m * ((double)m * ay + by)) / n);

        if (x < topLeft.x())
            topLeft.setX(x);
        if (y < topLeft.y())
            topLeft.setY(y);

        if (x > bottomRight.x())
            bottomRight.setX(x);
        if (y > bottomRight.y())
            bottomRight.setY(y);

        pointAccumulator->push_back(QPoint(x, y));
    }
}

}
