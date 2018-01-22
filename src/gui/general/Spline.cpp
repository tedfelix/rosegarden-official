/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
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

Spline::calculate(const QPoint &s, const QPoint &f, const PointList &cp,
                  QPoint &topLeft, QPoint &bottomRight)
{
    if (cp.size() < 2)
        return 0;

    int i;
    PointList *acc = new PointList();
    QPoint p(s);

    topLeft = bottomRight = QPoint(0, 0);

    for (i = 1; i < cp.size(); ++i) {

        QPoint c(cp[i - 1]);

        int x = (c.x() + cp[i].x()) / 2;
        int y = (c.y() + cp[i].y()) / 2;
        QPoint n(x, y);

        calculateSegment(acc, p, n, c, topLeft, bottomRight);

        p = n;
    }

    calculateSegment(acc, p, f, cp[i - 1], topLeft, bottomRight);

    return acc;
}

void
Spline::calculateSegment(PointList *acc,
                         const QPoint &s, const QPoint &f, const QPoint &c,
                         QPoint &topLeft, QPoint &bottomRight)
{
    int x, y, n;

    x = c.x() - s.x();
    y = c.y() - s.y();

    if (x < 0)
        x = -x;
    if (y < 0)
        y = -y;
    if (x > y)
        n = x;
    else
        n = y;

    x = f.x() - c.x();
    y = f.y() - c.y();

    if (x < 0)
        x = -x;
    if (y < 0)
        y = -y;
    if (x > y)
        n += x;
    else
        n += y;

    calculateSegmentSub(acc, s, f, c, n, topLeft, bottomRight);
}

void
Spline::calculateSegmentSub(PointList *acc,
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

        acc->push_back(QPoint(x, y));
    }
}

}
