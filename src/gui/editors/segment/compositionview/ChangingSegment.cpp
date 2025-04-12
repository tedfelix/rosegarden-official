/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "ChangingSegment.h"

#include "SegmentRect.h"
#include "base/SnapGrid.h"

#include <QRect>

#include <math.h>


namespace Rosegarden
{


ChangingSegment::ChangingSegment(Segment &s, const SegmentRect &rect)
        : m_segment(s),
        m_rect(rect),
        m_z(0)
{}

QRect ChangingSegment::rect() const
{
    QRect res = m_rect.rect;

    // For repeating segments, use the base width
    if (m_rect.isRepeating()) {
        res.setWidth(m_rect.baseWidth);
    }

    return res;
}

timeT ChangingSegment::getRepeatTimeAt(const SnapGrid &grid, const QPoint &pos) const
{
    timeT startTime = m_segment.getStartTime();
    timeT repeatInterval = m_segment.getEndMarkerTime() - startTime;

    int repeatWidth = int(nearbyint(grid.getRulerScale()->getXForTime(repeatInterval)));

    int count = (pos.x() - rect().x()) / repeatWidth;

    // Let the caller know that the position was not within a repeat.
    if (count == 0)
        return 0;

    return startTime + count * repeatInterval;
}

void ChangingSegment::setStartTime(timeT time, const SnapGrid &grid)
{
    int x = int(nearbyint(grid.getRulerScale()->getXForTime(time)));

    int curX = rect().x();
    m_rect.rect.setX(x);
    if (m_rect.isRepeating()) {
        int deltaX = curX - x;
        int curW = m_rect.baseWidth;
        m_rect.baseWidth = curW + deltaX;
    }
}

timeT ChangingSegment::getStartTime(const SnapGrid &grid)
{
    //return std::max(grid.snapX(item->rect().x()), 0L); - wrong, we can have negative start times,
        // and if we do this we 'crop' segments when they are moved before the start of the composition

    return grid.snapX(m_rect.rect.x());
}

void ChangingSegment::setEndTime(timeT time, const SnapGrid &grid)
{
    int x = int(nearbyint(grid.getRulerScale()->getXForTime(time)));
    QRect r = rect();
    QPoint topRight = r.topRight();
    topRight.setX(x);
    r.setTopRight(topRight);
    m_rect.rect.setWidth(r.width());

    if (m_rect.isRepeating()) {
        m_rect.baseWidth = r.width();
    }
}

timeT ChangingSegment::getEndTime(const SnapGrid &grid) const
{
    QRect itemRect = rect();

    return std::max(grid.snapX(itemRect.x() + itemRect.width()), 0L);
}

int ChangingSegment::getTrackPos(const SnapGrid &grid) const
{
    return grid.getYBin(rect().y());
}


}
