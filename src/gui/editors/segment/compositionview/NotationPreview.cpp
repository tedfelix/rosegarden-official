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

#define RG_MODULE_STRING "[NotationPreview]"

#if 0  // Abandoning.  No sense dragging down the build...

// Local
#include "NotationPreview.h"

// Rosegarden
#include "CompositionRect.h"
#include "CompositionModelImpl.h"
#include "base/Composition.h"
#include "base/Studio.h"
#include "base/BaseProperties.h"
#include "base/Profiler.h"

// System
#include <math.h>
#include <algorithm>

namespace Rosegarden
{


NotationPreview::NotationPreview(Composition &composition,
                                 CompositionModelImpl &compositionModelImpl,
                                 Studio &studio,
                                 RulerScale *rulerScale,
                                 int vStep) :
    m_composition(composition),
    m_compositionModelImpl(compositionModelImpl),
    m_studio(studio),
    m_grid(rulerScale, vStep)
{
}

void NotationPreview::getSegmentRects(
        const QRect &clipRect, RectRanges *notationPreview)
{
    // The code here must mimic the code in
    // CompositionModelImpl::getSegmentRects().

    // This is interesting as CompositionModelImpl::getSegmentRects() is
    // essentially a renderer for all the segments and previews.  It just
    // renders to rectangles.  It's then up to CompositionView to draw
    // those rectangles on the screen.

    // The code in CompositionModelImpl::getSegmentRects() is interleaved
    // in such a way that all the work gets done at the same time.  This
    // makes sense.  For NotationPreview::getSegmentRects() to make sense,
    // we might need to turn CompositionModelImpl::getSegmentRects() inside
    // out and redesign it.  At this point, it would probably be good to
    // think through an "ideal" design and see if that gives us any ideas.

    // for each segment in the composition

        // if the segment is moving, continue

    // end for

    // for each moving segment
    // end for
}

void NotationPreview::makeNotationPreviewRects(QPoint basePoint,
        const Segment* segment, const QRect& clipRect, RectRanges* npRects)
{
    Profiler profiler("NotationPreview::makeNotationPreviewRects");

    RectList* cachedNPData = getNotationPreviewData(segment);

    if (cachedNPData->empty())
        return ;

    RectList::iterator npEnd = cachedNPData->end();

    // Find the first preview rect that *starts within* the clipRect.
    // Probably not the right thing to do as this means any event that starts
    // prior to the clipRect but stretches through the clipRect will be
    // dropped.  And this explains why long notes disappear from the segment
    // previews.
    // Note that RectList is a std::vector, so this call will take increasing
    // amounts of time as the number of events to the left of the clipRect
    // increases.  This is probably at least a small part of the "CPU usage
    // increasing over time" issue.
    // If cachedNPData is sorted by start time, we could at least do a binary
    // search.
    RectList::iterator npi = std::lower_bound(cachedNPData->begin(), npEnd, clipRect, RectCompare());

    // If no preview rects were within the clipRect, bail.
    if (npi == npEnd)
        return ;

    // ??? Go back one event if we aren't already at the beginning.  Why?
    // Hilariously, this partially "fixes" the "missing event in preview"
    // problem.  However, it only "fixes" the problem for a single event.
    // Is that why this is here?
    // When testing, to get around the fact that the segments are drawn on a
    // segment layer in CompositionView, just disable then re-enable segment
    // previews in the menu and the "missing event in preview" problem is easy
    // to see.
    if (npi != cachedNPData->begin())
        --npi;

    // Compute the interval within the Notation Preview for this segment.

    RectRange interval;
    interval.range.first = npi;

    // Compute the rightmost x coord (xLim)
    int segEndX = int(nearbyint(m_grid.getRulerScale()->getXForTime(segment->getEndMarkerTime())));
    int xLim = std::min(clipRect.right(), segEndX);

    //RG_DEBUG << "NotationPreview::makeNotationPreviewRects : basePoint.x : "
    //         << basePoint.x();

    // Search sequentially for the last preview rect in the segment.
    while (npi != npEnd  &&  npi->x() < xLim)
        ++npi;

    interval.range.second = npi;
    interval.basePoint.setX(0);
    interval.basePoint.setY(basePoint.y());
    interval.color = segment->getPreviewColour();

    // Add the interval to the caller's interval list.
    npRects->push_back(interval);
}

void NotationPreview::makeNotationPreviewRectsMovingSegment(QPoint basePoint,
        const Segment* segment, const QRect& currentSR, RectRanges* npRects)
{
    CompositionRect unmovedSR =
            m_compositionModelImpl.computeSegmentRect(*segment);

    RectList* cachedNPData = getNotationPreviewData(segment);

    if (cachedNPData->empty())
        return ;

    RectList::iterator npBegin = cachedNPData->begin();
    RectList::iterator npEnd = cachedNPData->end();

    RectList::iterator npi;

    if (m_compositionModelImpl.getChangeType() ==
            CompositionModelImpl::ChangeResizeFromStart)
        npi = std::lower_bound(npBegin, npEnd, currentSR, RectCompare());
    else
        npi = std::lower_bound(npBegin, npEnd, unmovedSR, RectCompare());

    if (npi == npEnd)
        return ;

    // ??? Bump iterator back one to try and pick up the previous event
    //     rectangle which might be needed.
    if (npi != npBegin  &&
            m_compositionModelImpl.getChangeType() !=
                    CompositionModelImpl::ChangeResizeFromStart) {
        --npi;
    }

    // Compute the interval within the Notation Preview for this segment.

    RectRange interval;
    interval.range.first = npi;

    // Compute the rightmost x coord (xLim)
    int xLim = m_compositionModelImpl.getChangeType() ==
            CompositionModelImpl::ChangeMove ?
                    unmovedSR.right() : currentSR.right();

    //RG_DEBUG << "NotationPreview::makeNotationPreviewRectsMovingSegment : basePoint.x : "
    //         << basePoint.x();

    // Search sequentially for the last preview rect in the segment.
    while (npi != npEnd  &&  npi->x() < xLim)
        ++npi;

    interval.range.second = npi;
    interval.basePoint.setY(basePoint.y());

    if (m_compositionModelImpl.getChangeType() ==
            CompositionModelImpl::ChangeMove)
        interval.basePoint.setX(basePoint.x() - unmovedSR.x());
    else
        interval.basePoint.setX(0);

    interval.color = segment->getPreviewColour();

    npRects->push_back(interval);
}

NotationPreview::RectList* NotationPreview::getNotationPreviewData(const Segment* s)
{
    RectList* npData = m_notationPreviewDataCache[s];

    if (!npData) {
        npData = makeNotationPreviewDataCache(s);
    }

    return npData;
}

NotationPreview::RectList* NotationPreview::makeNotationPreviewDataCache(const Segment *s)
{
    RectList* npData = new RectList();

    // Create the preview
    createEventRects(s, npData);

    // Store in the cache.
    // Callers guarantee that m_notationPreviewDataCache[s] is not currently
    // pointing to anything.
    m_notationPreviewDataCache[s] = npData;

    return npData;
}

void NotationPreview::createEventRects(const Segment *segment, RectList *npData)
{
    npData->clear();

    int segStartX = static_cast<int>(nearbyint(
            m_grid.getRulerScale()->getXForTime(segment->getStartTime())));

    bool isPercussion = false;
    Track *track = m_composition.getTrackById(segment->getTrack());
    if (track) {
        InstrumentId iid = track->getInstrument();
        Instrument *instrument = m_studio.getInstrumentById(iid);
        if (instrument && instrument->isPercussion()) isPercussion = true;
    }

    // For each event in the segment
    for (Segment::const_iterator i = segment->begin();
         i != segment->end(); ++i) {

        long pitch = 0;
        if (!(*i)->isa(Note::EventType) ||
            !(*i)->get<Int>(BaseProperties::PITCH, pitch)) {
            continue;
        }

        timeT eventStart = (*i)->getAbsoluteTime();
        timeT eventEnd = eventStart + (*i)->getDuration();
        //  if (eventEnd > segment->getEndMarkerTime()) {
        //      eventEnd = segment->getEndMarkerTime();
        //  }

        int x = static_cast<int>(nearbyint(
                m_grid.getRulerScale()->getXForTime(eventStart)));
        int width = static_cast<int>(nearbyint(
                m_grid.getRulerScale()->getWidthForDuration(
                        eventStart, eventEnd - eventStart)));

        //RG_DEBUG << "NotationPreview::createEventRects: x = " << x << ", width = " << width << " (time = " << eventStart << ", duration = " << eventEnd - eventStart << ")";

        if (x <= segStartX) {
            ++x;
            if (width > 1) --width;
        }
        if (width > 1) --width;
        if (width < 1) ++width;

        const double y0 = 0;
        const double y1 = m_grid.getYSnap();
        double y = y1 + ((y0 - y1) * (pitch - 16)) / 96;

        int height = 1;

        if (isPercussion) {
            height = 2;
            if (width > 2) width = 2;
        }

        if (y < y0) y = y0;
        if (y > y1 - height + 1) y = y1 - height + 1;

        // ??? static_cast<int>(nearbyint(y))?
        QRect r(x, static_cast<int>(y), width, height);

        npData->push_back(r);
    }
}


}

// We're not a QObject just yet.
//#include "NotationPreview.moc"
#endif
