
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

#ifndef RG_SPLINE_H
#define RG_SPLINE_H

#include "base/FastVector.h"


class QPoint;
class PointList;


namespace Rosegarden
{



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



}

#endif
