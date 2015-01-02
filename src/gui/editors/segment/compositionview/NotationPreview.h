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

#ifndef RG_NOTATIONPREVIEW_H
#define RG_NOTATIONPREVIEW_H

// Rosegarden
#include "base/SnapGrid.h"

// Qt
#include <QRect>
#include <QColor>

// System
#include <map>
#include <vector>

namespace Rosegarden
{


// Forward declares
class Composition;
class CompositionModelImpl;
class Studio;
class Segment;

/// Draw Event objects on a QPainter
/**
 * WORK ABANDONED
 *
 * This will likely be abandoned.  I've come to the conclusion that although
 * this is a good idea, the code isn't currently organized in a way that is
 * amenable to cleanly teasing out the concept of a Notation Preview.  That's
 * ok.  I'm considering attacking this from a different direction.  One way or
 * another, we'll get there.
 *
 * WORK IN PROGRESS
 *
 * This code is not used by any part of the system yet.  I am working on
 * pulling the notation preview code out of CompositionModelImpl.  This
 * will provide a class for an important concept.  And that's really what
 * it's all about.
 *
 * TODO
 * - Finish bringing in all necessary pieces.
 * - Evaluate all use of m_notationPreviewDataCache in CompositionModelImpl.
 *   We need to bring all of it in here.
 * - Come up with a clever way to swap this in without disturbing anything
 *   so we can easily go back to the old way.
 *
 * We should probably be heading towards a MIDISegmentView class that renders
 * a segment (or all the segments in a composition) on a QPainter.
 *
 * One key question is whether this class
 * should handle a single segment, or all the segments in the composition.
 * Not sure which results in the simpler implementation.  Handling a single
 * segment means the client would need to manage a collection of these
 * objects.  Handling all segments means it would need to juggle a cache
 * for each segment, much like the current implementation.  Maybe we could
 * do something in between.  Perhaps it could look like it handles all of
 * the segments, but internally it will manage "info" structs for each
 * segment.  This really doesn't have the same issues as other systems I've
 * worked on, so I'm not sure that the one-for-one approach has any benefit.
 *
 * Let's go with managing all segments first as that is in keeping with the
 * code it is coming from.  Then we'll think through whether pulling out
 * the "aggregation" has any benefits.
 */
class NotationPreview
{
public:
    NotationPreview(Composition &composition,
                    CompositionModelImpl &compositionModelImpl,
                    Studio &studio,
                    RulerScale *rulerScale,
                    int vStep);

    typedef std::vector<QRect> RectList;

    struct RectRange {
        std::pair<RectList::iterator, RectList::iterator> range;
        QPoint basePoint;
        QColor color;
    };

    typedef std::vector<RectRange> RectRanges;

    // From CompositionView::drawSegments().
    // ??? Should this class be able to draw itself?
    //     This would mean that CompositionView could own the object.
    //     I think that in the end this is indeed the right thing to do.
    //     NotationPreview is a "view" class.  It is a view of the
    //     events in a segment drawn on a QPainter.  I guess it is really
    //     part of a SegmentView.  So maybe we should eventually have a
    //     MIDISegmentView and an AudioSegmentView.  CompositionView would
    //     bring these together to draw the composition.
    void draw();

    // From CompositionModelImpl::getSegmentRects()
    // ??? Or should it just provide this and let CompositionView draw?
    //     This would mean that CompositionModelImpl might own the object.
    //     Probably easiest to go with this first, then see if moving
    //     draw into here is possible and whether it helps simplify things.
    void getSegmentRects(const QRect &clipRect, RectRanges *notationPreview);

private:
    Composition &m_composition;
    CompositionModelImpl &m_compositionModelImpl;
    Studio &m_studio;
    SnapGrid m_grid;

    struct RectCompare {
        bool operator()(const QRect &r1, const QRect &r2) const {
            return r1.left() < r2.left();
        }
    };

    // From CompositionModelImpl::makeNotationPreviewRects()
    void makeNotationPreviewRects(QPoint basePoint,
            const Segment* segment, const QRect& clipRect, RectRanges* npRects);

    // From CompositionModelImpl::makeNotationPreviewRectsMovingSegment()
    void makeNotationPreviewRectsMovingSegment(QPoint basePoint,
            const Segment* segment, const QRect& currentSR, RectRanges* npRects);

    // From CompositionModelImpl::getNotationPreviewData()
    RectList *getNotationPreviewData(const Segment* s);

    // From CompositionModelImpl::makeNotationPreviewDataCache()
    RectList *makeNotationPreviewDataCache(const Segment *s);

    // From CompositionModelImpl::createEventRects()
    void createEventRects(const Segment *segment, RectList *rects);

    typedef std::map<const Segment *, RectList *> NotationPreviewDataCache;
    // ??? Need to bring in all code from CompositionModelImpl that maintains
    //     this cache.
    NotationPreviewDataCache m_notationPreviewDataCache;
};


}

#endif
