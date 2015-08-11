/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[CompositionItemHelper]"

#include <cmath>

#include "CompositionItemHelper.h"

#include "base/Segment.h"
#include "base/SnapGrid.h"
#include "misc/Debug.h"
#include "CompositionItem.h"
#include <QColor>
#include <QPoint>
#include <QRect>

namespace Rosegarden
{
    
timeT CompositionItemHelper::getStartTime(CompositionItemPtr item, const Rosegarden::SnapGrid& grid)
{
    timeT t = 0;

    if (item) {
    // t = std::max(grid.snapX(item->rect().x()), 0L); - wrong, we can have negative start times,
        // and if we do this we 'crop' segments when they are moved before the start of the composition
    t = grid.snapX(item->rect().x());

//         RG_DEBUG << "CompositionItemHelper::getStartTime(): item is repeating : " << item->isRepeating()
//                  << " - startTime = " << t
//                  << endl;
    }

    return t;
}

timeT CompositionItemHelper::getEndTime(CompositionItemPtr item, const Rosegarden::SnapGrid& grid)
{
    timeT t = 0;

    if (item) {
        QRect itemRect = item->rect();
        
        t = std::max(grid.snapX(itemRect.x() + itemRect.width()), 0L);

//         RG_DEBUG << "CompositionItemHelper::getEndTime() : rect width = "
//                  << itemRect.width()
//                  << " - item is repeating : " << item->isRepeating()
//                  << " - endTime = " << t
//                  << endl;

    }

    return t;
}

void CompositionItemHelper::setStartTime(CompositionItemPtr item, timeT time,
                                         const Rosegarden::SnapGrid& grid)
{
    if (item) {
        int x = int(nearbyint(grid.getRulerScale()->getXForTime(time)));

        RG_DEBUG << "CompositionItemHelper::setStartTime() time = " << time
                 << " -> x = " << x << endl;
        
        int curX = item->rect().x();
        item->setX(x);
        if (item->isRepeating()) {
            int deltaX = curX - x;
            SegmentRect& sr = item->getCompRect();
            int curW = sr.getBaseWidth();
            sr.setBaseWidth(curW + deltaX);
        }
        
    }
    
}

void CompositionItemHelper::setEndTime(CompositionItemPtr item, timeT time,
                                       const Rosegarden::SnapGrid& grid)
{
    if (item) {
        int x = int(nearbyint(grid.getRulerScale()->getXForTime(time)));
        QRect r = item->rect();
        QPoint topRight = r.topRight();
        topRight.setX(x);
        r.setTopRight(topRight);
        item->setWidth(r.width());

        if (item->isRepeating()) {
            SegmentRect& sr = item->getCompRect();
            sr.setBaseWidth(r.width());
        }
    }
}

int CompositionItemHelper::getTrackPos(CompositionItemPtr item, const Rosegarden::SnapGrid& grid)
{
    return grid.getYBin(item->rect().y());
}

Rosegarden::Segment* CompositionItemHelper::getSegment(CompositionItemPtr item)
{
    return item->getSegment();
}

CompositionItemPtr CompositionItemHelper::makeCompositionItem(Rosegarden::Segment* segment)
{
    return CompositionItemPtr(new CompositionItem(*segment, QRect()));
}

CompositionItemPtr CompositionItemHelper::findSiblingCompositionItem(
        const CompositionModelImpl::ChangingSegmentSet &items,
        CompositionItemPtr referenceItem)
{
    CompositionModelImpl::ChangingSegmentSet::const_iterator it;
    Rosegarden::Segment *referenceSegment = referenceItem->getSegment();

    // For each item in the incoming container
    for (it = items.begin(); it != items.end(); ++it) {
        CompositionItemPtr item = *it;
        Rosegarden::Segment *segment = item->getSegment();

        // If it has the same Segment as the reference item, return it.
        if (segment == referenceSegment) {
            return item;
        }
    }

    // Not found, just return the incoming item.
    return referenceItem;
}

}
