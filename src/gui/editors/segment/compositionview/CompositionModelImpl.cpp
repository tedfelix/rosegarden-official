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

#define RG_MODULE_STRING "[CompositionModelImpl]"

#include "CompositionModelImpl.h"
#include "SegmentOrderer.h"
#include "AudioPreviewThread.h"
#include "AudioPreviewUpdater.h"
#include "AudioPreviewPainter.h"
#include "CompositionItem.h"
#include "SegmentRect.h"
#include "CompositionColourCache.h"

#include "base/BaseProperties.h"
#include "misc/Debug.h"
#include "misc/Strings.h"  // strtoqstr()
#include "base/Composition.h"
#include "base/Event.h"
#include "base/Instrument.h"
#include "base/InstrumentStaticSignals.h"
#include "base/Profiler.h"
#include "base/RulerScale.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "base/SnapGrid.h"
#include "base/Studio.h"
#include "base/Track.h"
#include "gui/general/GUIPalette.h"

#include <QBrush>
#include <QColor>
#include <QPoint>
#include <QRect>
#include <QRegExp>
#include <QSize>
#include <QString>

#include <math.h>
#include <algorithm>  // std::lower_bound() and std::min()


namespace Rosegarden
{


CompositionModelImpl::CompositionModelImpl(
        QObject *parent,
        Composition &composition,
        Studio &studio,
        RulerScale *rulerScale,
        int trackCellHeight) :
    QObject(parent),
    m_composition(composition),
    m_studio(studio),
    m_grid(rulerScale, trackCellHeight),
    m_instrumentStaticSignals(),
    m_notationPreviewCache(),
    m_audioPreviewThread(0),
    m_audioPreviewUpdaterMap(),
    m_audioPeaksCache(),
    m_audioPreviewImageCache(),
    m_trackHeights(),
    m_segmentOrderer(),
    m_segmentRectMap(),
    m_segmentEndTimeMap(),
    m_selectedSegments(),
    m_tmpSelectedSegments(),
    m_previousTmpSelectedSegments(),
    m_selectionRect(),
    m_previousSelectionUpdateRect(),
    m_recordingSegments(),
    m_pointerTime(0),
    m_changeType(ChangeMove),
    m_changingSegments(),
    m_changingSegmentGC()
{
    m_composition.addObserver(this);

    // Update the track heights for all tracks.
    updateTrackHeight();

    SegmentMultiSet &segments = m_composition.getSegments();

    // For each segment in the Composition
    for (SegmentMultiSet::iterator i = segments.begin();
         i != segments.end();
         ++i) {

        // Subscribe
        (*i)->addObserver(this);
    }

    // Hold on to this to make sure it stays around as long as we do.
    m_instrumentStaticSignals = Instrument::getStaticSignals();

    connect(m_instrumentStaticSignals.data(),
            SIGNAL(changed(Instrument *)),
            this,
            SLOT(slotInstrumentChanged(Instrument *)));
}

CompositionModelImpl::~CompositionModelImpl()
{
    if (!isCompositionDeleted()) {

        m_composition.removeObserver(this);

        SegmentMultiSet &segments = m_composition.getSegments();

        // For each segment in the Composition
        for (SegmentMultiSet::iterator i = segments.begin();
             i != segments.end(); ++i) {

            // Unsubscribe
            (*i)->removeObserver(this);
        }
    }

    if (m_audioPreviewThread) {
        // For each audio peaks updater
        // ??? This is similar to setAudioPreviewThread().
        while (!m_audioPreviewUpdaterMap.empty()) {
            // Cause any running previews to be cancelled
            delete m_audioPreviewUpdaterMap.begin()->second;
            m_audioPreviewUpdaterMap.erase(m_audioPreviewUpdaterMap.begin());
        }
    }

    // ??? The following code is similar to deleteCachedPreviews().
    //     The problem is that deleteCachedPreviews() regenerates the
    //     audio previews.  If we can make it stop doing that, then
    //     we can call it from here.

    // Delete the notation previews
    for (NotationPreviewCache::iterator i = m_notationPreviewCache.begin();
         i != m_notationPreviewCache.end(); ++i) {
        delete i->second;
    }

    // Delete the audio peaks
    for (AudioPeaksCache::iterator i = m_audioPeaksCache.begin();
         i != m_audioPeaksCache.end(); ++i) {
        delete i->second;
    }
}

// --- Segments -----------------------------------------------------

void CompositionModelImpl::getSegmentRects(
        const QRect &clipRect,
        SegmentRects *segmentRects,
        NotationPreviewRanges *notationPreviewRanges,
        AudioPreviews *audioPreviews)
{
    Profiler profiler("CompositionModelImpl::getSegmentRects()");

    // Start with a clean slate.
    segmentRects->clear();

    // For readability
    CompositionColourCache *colourCache =
            CompositionColourCache::getInstance();

    const SegmentMultiSet &segments = m_composition.getSegments();

    // For each segment in the composition
    for (SegmentMultiSet::const_iterator i = segments.begin();
         i != segments.end();
         ++i) {

        const Segment *segment = *i;

        // Changing segments are handled in the next for loop.
        if (isChanging(segment))
            continue;

        SegmentRect segmentRect = computeSegmentRect(*segment);

        // If this segment isn't in the clip rect, try the next.
        if (!segmentRect.intersects(clipRect))
            continue;

        const bool isTmpSelected2 = isTmpSelected(segment);

        // Update the SegmentRect's selected state.

        segmentRect.setSelected(
                isSelected(segment)  ||
                isTmpSelected2  ||
                segmentRect.intersects(m_selectionRect));

        // Update the SegmentRect's "needs full update".

        segmentRect.setNeedsFullUpdate(
                wasTmpSelected(segment) != isTmpSelected2);

        // Update the SegmentRect's pen and brush.

        if (!isRecording(segment)) {
            // Pen
            segmentRect.setPen(colourCache->SegmentBorder);

            // Brush
            QColor brushColor = GUIPalette::convertColour(
                    m_composition.getSegmentColourMap().getColourByIndex(
                            segment->getColourIndex()));
            Qt::BrushStyle brushPattern =
                    segment->isTrulyLinked() ?
                            Qt::Dense2Pattern : Qt::SolidPattern;
            segmentRect.setBrush(QBrush(brushColor, brushPattern));
        } else {  // Recording
            // Pen
            segmentRect.setPen(colourCache->RecordingSegmentBorder);

            // Brush
            if (segment->isMIDI())
                segmentRect.setBrush(colourCache->RecordingInternalSegmentBlock);
            else  // Audio Segment
                segmentRect.setBrush(colourCache->RecordingAudioSegmentBlock);
        }

        // Generate the requested previews.

        if (segment->isMIDI()) {
            makeNotationPreviewRange(
                    QPoint(0, segmentRect.y()), segment, clipRect,
                    notationPreviewRanges);
        } else {  // Audio Segment
            makeAudioPreview(audioPreviews, segment, segmentRect);
        }

        segmentRects->push_back(segmentRect);
    }

    // Changing Segments (Moving/Resizing)

    // For each changing segment
    for (ChangingSegmentSet::iterator i = m_changingSegments.begin();
         i != m_changingSegments.end();
         ++i) {

        SegmentRect segmentRect((*i)->rect());

        // If it doesn't intersect, try the next one.
        if (!segmentRect.intersects(clipRect))
            continue;

        // Set up the SegmentRect

        // Selected
        segmentRect.setSelected(true);

        // Brush
        Segment *segment = (*i)->getSegment();
        Colour segmentColour =
                m_composition.getSegmentColourMap().getColourByIndex(
                        segment->getColourIndex());
        segmentRect.setBrush(GUIPalette::convertColour(segmentColour));

        // Pen
        segmentRect.setPen(colourCache->SegmentBorder);

        // Generate the preview

        if (segment->isMIDI()) {
            makeNotationPreviewRangeCS(
                    segmentRect.topLeft(), segment, segmentRect,
                    notationPreviewRanges);
        } else {  // Audio Segment
            makeAudioPreview(audioPreviews, segment, segmentRect);
        }

        segmentRects->push_back(segmentRect);
    }
}

CompositionItemPtr CompositionModelImpl::getSegmentAt(const QPoint &pos)
{
    const SegmentMultiSet &segments = m_composition.getSegments();

    // For each segment in the composition
    for (SegmentMultiSet::iterator i = segments.begin();
         i != segments.end();
         ++i) {

        Segment &segment = **i;

        SegmentRect segmentRect = computeSegmentRect(segment);

        if (segmentRect.contains(pos)) {
            // ??? Need to make CompositionItemPtr into a QSharedPointer
            //     to simplify memory management.
            CompositionItemPtr changingSegment(
                    new CompositionItem(segment, segmentRect));

            return changingSegment;
        }
    }

    // Not found.
    return NULL;
}

SegmentRect CompositionModelImpl::computeSegmentRect(const Segment& s, bool /*computeZ*/)
{
    Profiler profiler("CompositionModelImpl::computeSegmentRect");

    // ??? This shouldn't return a SegmentRect by value.  It's huge.
    //     Make it a pointer parameter.
    // ??? The second argument is ignored.  Remove it.
    // ??? Various callers to computeSegmentRect() need various
    //     levels of SegmentRect completion.  E.g. getSegmentRects()
    //     needs everything filled in, but eventAdded() only needs the
    //     QRect.  That's annoying.  Analyze all callers and find out if
    //     there is a pattern we can take advantage of.  Maybe offer
    //     two versions of computeSegmentRect().  One that just worries
    //     about the QRect and one that fills everything in.

    QPoint origin = topLeft(s);

    bool isRecordingSegment = isRecording(&s);

    if (!isRecordingSegment) {
        // ??? Why is endTime cached?  Seems unnecessary.
        timeT endTime = 0;

        // ??? This is the only caller.  Inline this function.
        SegmentRect cachedCR = getFromCache(&s, endTime);
        // don't cache repeating segments - it's just hopeless, because the segment's rect may have to be recomputed
        // in other cases than just when the segment itself is moved,
        // for instance if another segment is moved over it
        // ??? Inline isCachedRectCurrent().  This is the only caller.
        //     Make it a bool isCachedRectCurrent.
        if (!s.isRepeating() && cachedCR.isValid() && isCachedRectCurrent(s, cachedCR, origin, endTime)) {
            //RG_DEBUG << "CompositionModelImpl::computeSegmentRect() : using cache for seg "
            //         << &s << " - cached rect repeating = " << cachedCR.isRepeating() << " - base width = "
            //         << cachedCR.getBaseWidth();

            bool xChanged = origin.x() != cachedCR.x();
            bool yChanged = origin.y() != cachedCR.y();

            cachedCR.moveTopLeft(origin);

            if (s.isRepeating() && (xChanged || yChanged)) { // update repeat marks

                // this doesn't work in the general case (if there's another segment on the same track for instance),
                // it's better to simply recompute all the marks
                //                 SegmentRect::repeatmarks repeatMarks = cachedCR.getRepeatMarks();
                //                 for(size_t i = 0; i < repeatMarks.size(); ++i) {
                //                     repeatMarks[i] += deltaX;
                //                 }
                //                 cachedCR.setRepeatMarks(repeatMarks);
                computeRepeatMarks(cachedCR, &s);
            }
            updateCachedSegment(&s, cachedCR);
            return cachedCR;
        }
    }

    timeT startTime = s.getStartTime();
    timeT endTime = isRecordingSegment ? m_pointerTime : s.getEndMarkerTime();


    int h = m_grid.getYSnap() - 2;
    int w;

    if (s.isRepeating()) {
//        timeT repeatStart = endTime;
        timeT repeatEnd = s.getRepeatEndTime();
        w = int(nearbyint(m_grid.getRulerScale()->getWidthForDuration(startTime,
                          repeatEnd - startTime)));
        //RG_DEBUG << "CompositionModelImpl::computeSegmentRect : s is repeating - repeatStart = "
        //         << repeatStart << " - repeatEnd : " << repeatEnd
        //         << " w = " << w;
    } else {
        w = int(nearbyint(m_grid.getRulerScale()->getWidthForDuration(startTime, endTime - startTime)));
        //RG_DEBUG << "CompositionModelImpl::computeSegmentRect : s is NOT repeating"
        //         << " w = " << w << " (x for time at start is " << m_grid.getRulerScale()->getXForTime(startTime) << ", end is " << m_grid.getRulerScale()->getXForTime(endTime) << ")";
    }


    //RG_DEBUG << "CompositionModelImpl::computeSegmentRect: x " << origin.x() << ", y " << origin.y() << " startTime " << startTime << ", endTime " << endTime << ", w " << w << ", h " << h;

    SegmentRect cr(origin, QSize(w, h));
    QString label = strtoqstr(s.getLabel());
    if (s.getType() == Segment::Audio) {
        static QRegExp re1("( *\\([^)]*\\))*$"); // (inserted) (copied) (etc)
        static QRegExp re2("\\.[^.]+$"); // filename suffix
        label.replace(re1, "").replace(re2, "");
    }
    cr.setLabel(label);

    if (s.isRepeating()) {
        computeRepeatMarks(cr, &s);
    } else {
        cr.setBaseWidth(cr.width());
    }

    updateCachedSegment(&s, cr);

    return cr;
}

QPoint CompositionModelImpl::topLeft(const Segment& s) const
{
    Profiler profiler("CompositionModelImpl::topLeft");

    int trackPosition = m_composition.getTrackPositionById(s.getTrack());
    timeT startTime = s.getStartTime();

    QPoint res;

    res.setX(int(nearbyint(m_grid.getRulerScale()->getXForTime(startTime))));

    res.setY(m_grid.getYBinCoordinate(trackPosition) +
             m_composition.getSegmentVoiceIndex(&s) *
             m_grid.getYSnap() + 1);

    return res;
}

bool CompositionModelImpl::updateTrackHeight(const Segment *s)
{
    bool heightsChanged = false;

    //RG_DEBUG << "updateTrackHeight()";

    for (Composition::trackcontainer::const_iterator i =
             m_composition.getTracks().begin();
         i != m_composition.getTracks().end(); ++i) {

        if (s && i->first != s->getTrack()) continue;

        int max = m_composition.getMaxContemporaneousSegmentsOnTrack(i->first);
        if (max == 0) max = 1;

//        RG_DEBUG << "for track " << i->first << ": height = " << max << ", old height = " << m_trackHeights[i->first];

        if (max != m_trackHeights[i->first]) {
            heightsChanged = true;
            m_trackHeights[i->first] = max;
        }

        m_grid.setBinHeightMultiple(i->second->getPosition(), max);
    }

    if (heightsChanged) {
        //RG_DEBUG << "updateTrackHeight(): heights have changed";
        for (SegmentMultiSet::iterator i = m_composition.begin();
             i != m_composition.end(); ++i) {
            computeSegmentRect(**i);
        }
    }

    return heightsChanged;
}

void CompositionModelImpl::computeRepeatMarks(SegmentRect& sr, const Segment* s) const
{
    if (s->isRepeating()) {

        timeT startTime = s->getStartTime();
        timeT endTime = s->getEndMarkerTime();
        timeT repeatInterval = endTime - startTime;

        if (repeatInterval <= 0) {
            //RG_DEBUG << "WARNING: CompositionModelImpl::computeRepeatMarks: Segment at " << startTime << " has repeatInterval " << repeatInterval;
            //RG_DEBUG << kdBacktrace();
            return ;
        }

        timeT repeatStart = endTime;
        timeT repeatEnd = s->getRepeatEndTime();
        sr.setWidth(int(nearbyint(m_grid.getRulerScale()->getWidthForDuration(startTime,
                                  repeatEnd - startTime))));

        SegmentRect::repeatmarks repeatMarks;

        //RG_DEBUG << "CompositionModelImpl::computeRepeatMarks : repeatStart = "
        //         << repeatStart << " - repeatEnd = " << repeatEnd;

        for (timeT repeatMark = repeatStart; repeatMark < repeatEnd; repeatMark += repeatInterval) {
            int mark = int(nearbyint(m_grid.getRulerScale()->getXForTime(repeatMark)));
            //RG_DEBUG << "CompositionModelImpl::computeRepeatMarks : mark at " << mark;
            repeatMarks.push_back(mark);
        }

        // ??? COPY.  If we could get a non-const reference to
        //     m_repeatMarks, we could avoid the copy.
        // ??? Or move this routine
        //     (CompositionModelImpl::computeRepeatMarks()) to
        //     SegmentRect::updateRepeatMarks(segment, grid).
        sr.setRepeatMarks(repeatMarks);

        if (repeatMarks.size() > 0)
            sr.setBaseWidth(repeatMarks[0] - sr.x());
        else {
            //RG_DEBUG << "CompositionModelImpl::computeRepeatMarks : no repeat marks";
            sr.setBaseWidth(sr.width());
        }

        //RG_DEBUG << "CompositionModelImpl::computeRepeatMarks : s = "
        //         << s << " base width = " << sr.getBaseWidth()
        //         << " - nb repeat marks = " << repeatMarks.size();

    }
}

unsigned int CompositionModelImpl::computeZForSegment(const Rosegarden::Segment* s)
{
    /// ??? z-order is obsolete.  Remove.

    return m_segmentOrderer.getZForSegment(s);
}

void CompositionModelImpl::updateCachedSegment(const Segment*s, const SegmentRect& cr)
{
    m_segmentRectMap[s] = cr;
    // ??? Why not add begin and end times to SegmentRect and get
    //     rid of this?
    m_segmentEndTimeMap[s] = s->getEndMarkerTime();
}

const SegmentRect& CompositionModelImpl::getFromCache(const Rosegarden::Segment* s, timeT& endTime)
{
    endTime = m_segmentEndTimeMap[s];

    // ??? Trying to figure out how the cached value is different from the one
    //     we already have in the Segment.
    //     This is different right off the bat.  Creating a segment tests this.
    // ??? DO NOT COMMIT THIS!!!  Delete it if I accidentally commit this!
    //Q_ASSERT_X(endTime == s->getEndMarkerTime(),
    //           "CompositionModelImpl::getFromCache()",
    //           "Cached end time really is different.");

    return m_segmentRectMap[s];
}

bool CompositionModelImpl::isCachedRectCurrent(
        const Segment &segment,
        const SegmentRect &cachedCR,  // = m_segmentRectMap[segment]
        QPoint segmentTopLeft,            // = topLeft(segment)
        timeT cachedSegmentEndTime)       // = m_segmentEndTimeMap[segment]
{
    //RG_DEBUG << "isCachedRectCurrent()...";

    // ??? ISTM that the cache should work as follows...  The segment begin
    //     and end times (and repeat mode) should be used to determine
    //     whether the cache is
    //     current.  If the begin or end have changed, then the cached
    //     SegmentRect is outdated and needs to be recomputed.  So,
    //     we need to cache the begin and end time for each segment, then
    //     cache the composition rect.  Or add the begin and end time to
    //     SegmentRect.  Repeat mode is already there.

    // If the repeating mode changes, the rect is very different.
    bool repeatingMatches = (segment.isRepeating() == cachedCR.isRepeating());

    //RG_DEBUG << "  repeatingMatches: " << repeatingMatches;

    // ??? This happens when we are moving a segment.  So, when moving a
    //     segment, this indicates that the cache is ok.  That seems
    //     a bit too clever.
    bool nothingMatches =
            (segmentTopLeft.x() != cachedCR.x()  &&
             segment.getEndMarkerTime() != cachedSegmentEndTime);

    //RG_DEBUG << "  nothingMatches: " << nothingMatches;

    // If the begin and end are different, the rect is different.
    bool everythingMatches =
            (segmentTopLeft.x() == cachedCR.x()  &&
             segment.getEndMarkerTime() == cachedSegmentEndTime);

    //RG_DEBUG << "  everythingMatches: " << everythingMatches;

    /*
       Use cases.

       (Note that this routine is never called for a repeating segment.)

       When adjusting the beginning of a segment, we return false since
       nothingMatches is false and everythingMatches is false.

       When adjusting the end of a segment, we return true since
       everythingMatches is true.  I'm not sure
       why.  I assume that the cache has been updated before we get here.

       When moving a segment, we return true since nothingMatches is true.
    */

    bool isCurrent = (repeatingMatches && (nothingMatches || everythingMatches));

    //RG_DEBUG << "  isCurrent: " << isCurrent;

    return isCurrent;

    // The original code.  It has been broken apart above for analysis.
//    return segment.isRepeating() == cachedCR.isRepeating() &&
//           ((segmentTopLeft.x() != cachedCR.x() && segment.getEndMarkerTime() != cachedSegmentEndTime) ||
//            (segmentTopLeft.x() == cachedCR.x() && segment.getEndMarkerTime() == cachedSegmentEndTime));
}

void CompositionModelImpl::deleteCachedSegment(
        const Segment *s, bool previewToo)
{
    if (s) {
        m_segmentRectMap.erase(s);
        m_segmentEndTimeMap.erase(s);
        if (previewToo)
            deleteCachedPreview(s);
    } else { // clear the whole cache
        m_segmentRectMap.clear();
        m_segmentEndTimeMap.clear();
        if (previewToo)
            deleteCachedPreviews();
    }
}

void CompositionModelImpl::segmentAdded(const Composition *, Segment *s)
{
    //RG_DEBUG << "segmentAdded(): segment " << s << " on track " << s->getTrack() << ": calling updateTrackHeight()";
    updateTrackHeight(s);

    // ??? Why do it now?  Why not let getNotationPreview() do it
    //     on its own when/if it is needed?
    //makePreviewCache(s);

    s->addObserver(this);

    emit needUpdate();
}

void CompositionModelImpl::segmentRemoved(const Composition *, Segment *s)
{
    // Update track height for all tracks.
    updateTrackHeight();

    QRect r = computeSegmentRect(*s);

    m_selectedSegments.erase(s);

    deleteCachedSegment(s, true);
    s->removeObserver(this);
    m_recordingSegments.erase(s); // this could be a recording segment
    emit needUpdate(r);
}

void CompositionModelImpl::segmentTrackChanged(const Composition *, Segment *s, TrackId tid)
{
    RG_DEBUG << "CompositionModelImpl::segmentTrackChanged: segment " << s << " on track " << tid << ", calling updateTrackHeight()";

    // Update the height of all tracks.
    // we don't call updateTrackHeight(s), because some of the tracks
    // above s may have changed height as well (if s was moved off one
    // of them)
    if (updateTrackHeight()) {
        RG_DEBUG << "... changed, updating";
        emit needUpdate();
    }
}

void CompositionModelImpl::segmentStartChanged(const Composition *, Segment *s, timeT)
{
//    RG_DEBUG << "CompositionModelImpl::segmentStartChanged: segment " << s << " on track " << s->getTrack() << ": calling updateTrackHeight()";
    if (updateTrackHeight(s)) emit needUpdate();
}

void CompositionModelImpl::segmentEndMarkerChanged(const Composition *, Segment *s, bool)
{
    Profiler profiler("CompositionModelImpl::segmentEndMarkerChanged()");
//    RG_DEBUG << "CompositionModelImpl::segmentEndMarkerChanged: segment " << s << " on track " << s->getTrack() << ": calling updateTrackHeight()";
    if (updateTrackHeight(s)) {
//        RG_DEBUG << "... changed, updating";
        emit needUpdate();
    }
}

void CompositionModelImpl::segmentRepeatChanged(const Composition *, Segment *s, bool)
{
    deleteCachedSegment(s);
    updateTrackHeight(s);
    emit needUpdate();
}

void CompositionModelImpl::endMarkerTimeChanged(const Composition *, bool)
{
    emit needSizeUpdate();
}

// --- Recording ----------------------------------------------------

void CompositionModelImpl::addRecordingItem(CompositionItemPtr item)
{
    m_recordingSegments.insert(item->getSegment());

    emit needUpdate();

    //RG_DEBUG << "CompositionModelImpl::addRecordingItem: now have "
    //         << m_recordingSegments.size() << " recording items";
}

void CompositionModelImpl::pointerPosChanged(int x)
{
    //RG_DEBUG << "CompositionModelImpl::pointerPosChanged() begin";

    // Update the end point for the recording segments.
    m_pointerTime = m_grid.getRulerScale()->getTimeForX(x);

    // For each recording segment
    for (RecordingSegmentSet::iterator i = m_recordingSegments.begin();
            i != m_recordingSegments.end(); ++i) {
        // Ask CompositionView to update.
        emit needUpdate(computeSegmentRect(**i));
    }

    //RG_DEBUG << "CompositionModelImpl::pointerPosChanged() end";
}

void CompositionModelImpl::clearRecordingItems()
{
    //RG_DEBUG << "CompositionModelImpl::clearRecordingItem";

    // For each recording segment, remove it from the caches.
    for (RecordingSegmentSet::iterator i = m_recordingSegments.begin();
            i != m_recordingSegments.end(); ++i)
        deleteCachedSegment(*i, true);

    m_recordingSegments.clear();

    emit needUpdate();
}

bool CompositionModelImpl::isRecording(const Segment* s) const
{
    return m_recordingSegments.find(const_cast<Segment*>(s)) != m_recordingSegments.end();
}

void CompositionModelImpl::slotAudioFileFinalized(Segment* s)
{
    //RG_DEBUG << "CompositionModelImpl::slotAudioFileFinalized()";
    deleteCachedPreview(s);
}

// --- Changing -----------------------------------------------------

void CompositionModelImpl::startChange(CompositionItemPtr item, ChangeType change)
{
    m_changeType = change;

    // If we already know this segment is changing
    if (m_changingSegments.find(item) != m_changingSegments.end()) {
        //RG_DEBUG << "CompositionModelImpl::startChange : item already in";

        // Put this one on the garbage collection list for later cleanup
        // by endChange().
        // ??? Why can't we just delete it now?  It's not like we need it.
        //     Maybe the caller still needs it for the time being?
        // ??? If CompositionItemPtr were a QSharedPointer,
        //     m_changingSegmentGC would be unnecessary.
        // ??? Even better, get rid of all the pointers and go with
        //     objects.
        m_changingSegmentGC.push_back(item);
    } else {
        // Save the original rectangle for this segment
        item->saveRect();

        m_changingSegments.insert(item);
    }
}

void CompositionModelImpl::startChangeSelection(ChangeType change)
{
    // For each selected segment
    for (SegmentSelection::iterator i = m_selectedSegments.begin();
            i != m_selectedSegments.end(); ++i) {
        CompositionItemPtr item =
                new CompositionItem(**i, computeSegmentRect(**i));
        startChange(item, change);
    }
}

void CompositionModelImpl::endChange()
{
    //RG_DEBUG << "CompositionModelImpl::endChange";

    // For each segment that was changing
    for (ChangingSegmentSet::const_iterator i = m_changingSegments.begin();
            i != m_changingSegments.end(); ++i) {
        delete *i;
    }

    m_changingSegments.clear();

    // For each segment in the garbage collection list
    // ??? If CompositionItemPtr were a QSharedPointer,
    //     m_changingSegmentGC would be unnecessary.
    for (ChangingSegmentGC::iterator i = m_changingSegmentGC.begin();
            i != m_changingSegmentGC.end(); ++i) {
        delete *i;
    }

    m_changingSegmentGC.clear();

    emit needUpdate();
}

bool CompositionModelImpl::isChanging(const Segment* sm) const
{
    ChangingSegmentSet::const_iterator movEnd = m_changingSegments.end();

    for (ChangingSegmentSet::const_iterator i = m_changingSegments.begin(); i != movEnd; ++i) {
        const CompositionItem* ci = *i;
        const Segment* s = ci->getSegment();
        if (sm == s)
            return true;
    }

    return false;
}

// --- Notation Previews --------------------------------------------

void CompositionModelImpl::eventAdded(const Segment *s, Event *)
{
    //RG_DEBUG << "CompositionModelImpl::eventAdded()";
    Profiler profiler("CompositionModelImpl::eventAdded()");
    deleteCachedPreview(s);
    emit needUpdate(computeSegmentRect(*s));
}

void CompositionModelImpl::eventRemoved(const Segment *s, Event *)
{
    //RG_DEBUG << "CompositionModelImpl::eventRemoved";
    Profiler profiler("CompositionModelImpl::eventRemoved()");
    deleteCachedPreview(s);
    emit needUpdate(computeSegmentRect(*s));
}

void CompositionModelImpl::allEventsChanged(const Segment *s)
{
     Profiler profiler("CompositionModelImpl::allEventsChanged()");
     deleteCachedPreview(s);
    emit needUpdate(computeSegmentRect(*s));
}

void CompositionModelImpl::appearanceChanged(const Segment *s)
{
    //RG_DEBUG << "CompositionModelImpl::appearanceChanged";
    deleteCachedSegment(s, true);
    emit needUpdate(computeSegmentRect(*s));
}

void CompositionModelImpl::endMarkerTimeChanged(const Segment *s, bool shorten)
{
    Profiler profiler("CompositionModelImpl::endMarkerTimeChanged(Segment *, bool)");
    //RG_DEBUG << "CompositionModelImpl::endMarkerTimeChanged(" << shorten << ")";
    deleteCachedSegment(s, true);
    if (shorten) {
        emit needUpdate(); // no longer know former segment dimension
    } else {
        emit needUpdate(computeSegmentRect(*s));
    }
}

struct RectCompare {
    bool operator()(const QRect &r1, const QRect &r2) const {
        return r1.left() < r2.left();
    }
};

void CompositionModelImpl::makeNotationPreviewRange(
        QPoint basePoint, const Segment *segment,
        const QRect &clipRect, NotationPreviewRanges *ranges)
{
    Profiler profiler("CompositionModelImpl::makeNotationPreviewRange");

    if (!ranges)
        return;

    NotationPreview* cachedNPData = getNotationPreview(segment);

    if (cachedNPData->empty())
        return ;

    NotationPreview::iterator npEnd = cachedNPData->end();

    // Find the first preview rect that *starts within* the clipRect.
    // Probably not the right thing to do as this means any event that starts
    // prior to the clipRect but stretches through the clipRect will be
    // dropped.  And this explains why long notes disappear from the segment
    // previews.
    // Note that NotationPreview is a std::vector, so this call will take increasing
    // amounts of time as the number of events to the left of the clipRect
    // increases.  This is probably at least a small part of the "CPU usage
    // increasing over time" issue.
    // If cachedNPData is sorted by start time, we could at least do a binary
    // search.
    NotationPreview::iterator npi = std::lower_bound(cachedNPData->begin(), npEnd, clipRect, RectCompare());

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

    NotationPreviewRange interval;
    interval.begin = npi;

    // Compute the rightmost x coord (xLim)
    int segEndX = int(nearbyint(m_grid.getRulerScale()->getXForTime(segment->getEndMarkerTime())));
    int xLim = std::min(clipRect.right(), segEndX);

    //RG_DEBUG << "makeNotationPreviewRange : basePoint.x : " << basePoint.x();

    // Search sequentially for the last preview rect in the segment.
    while (npi != npEnd  &&  npi->x() < xLim)
        ++npi;

    interval.end = npi;
    interval.moveXOffset = 0;
    interval.segmentTop = basePoint.y();
    interval.color = segment->getPreviewColour();

    // Add the interval to the caller's interval list.
    ranges->push_back(interval);
}

void CompositionModelImpl::makeNotationPreviewRangeCS(
        QPoint basePoint, const Segment *segment,
        const QRect &currentRect, NotationPreviewRanges *ranges)
{
    if (!ranges)
        return;

    // ??? rename: originalRect?
    SegmentRect unmovedSR = computeSegmentRect(*segment);

    NotationPreview* cachedNPData = getNotationPreview(segment);

    if (cachedNPData->empty())
        return ;

    NotationPreview::iterator npBegin = cachedNPData->begin();
    NotationPreview::iterator npEnd = cachedNPData->end();

    NotationPreview::iterator npi;

    if (m_changeType == ChangeResizeFromStart)
        npi = std::lower_bound(npBegin, npEnd, currentRect, RectCompare());
    else
        npi = std::lower_bound(npBegin, npEnd, unmovedSR, RectCompare());

    if (npi == npEnd)
        return ;

    // ??? Bump iterator back one to try and pick up the previous event
    //     rectangle which might be needed.
    if (npi != npBegin  &&  m_changeType != ChangeResizeFromStart) {
        --npi;
    }

    // Compute the interval within the Notation Preview for this segment.

    NotationPreviewRange interval;
    interval.begin = npi;

    // Compute the rightmost x coord (xLim)
    int xLim = m_changeType == ChangeMove ? unmovedSR.right() : currentRect.right();

    //RG_DEBUG << "makeNotationPreviewRangeCS : basePoint.x : " << basePoint.x();

    // Search sequentially for the last preview rect in the segment.
    while (npi != npEnd  &&  npi->x() < xLim)
        ++npi;

    interval.end = npi;
    interval.segmentTop = basePoint.y();

    if (m_changeType == ChangeMove)
        interval.moveXOffset = basePoint.x() - unmovedSR.x();
    else
        interval.moveXOffset = 0;

    interval.color = segment->getPreviewColour();

    ranges->push_back(interval);
}

CompositionModelImpl::NotationPreview *
CompositionModelImpl::getNotationPreview(const Segment *s)
{
    // ??? Consider combining getNotationPreview() and
    //     updateCachedNotationPreview().  The only caller of
    //     updateCachedNotationPreview() (segmentAdded()) has no need
    //     to call it at all.

    // Try the cache.
    NotationPreview *notationPreview = m_notationPreviewCache[s];

    // If there was nothing in the cache for this segment, generate it.
    if (!notationPreview)
        notationPreview = updateCachedNotationPreview(s);

    return notationPreview;
}

CompositionModelImpl::NotationPreview *
CompositionModelImpl::updateCachedNotationPreview(const Segment *segment)
{
    NotationPreview *notationPreview = new NotationPreview();

    // Create the preview
    makeNotationPreview(segment, notationPreview);

    // Avoid potential memory leaks.
    //delete m_notationPreviewCache[segment];

    // Store in the cache.
    // Callers guarantee that m_notationPreviewCache[segment] is not
    // currently pointing to anything.
    m_notationPreviewCache[segment] = notationPreview;

    return notationPreview;
}

void CompositionModelImpl::makeNotationPreview(
        const Segment *segment, NotationPreview *npData)
{
    //RG_DEBUG << "makeNotationPreview()";

    // ??? This routine is called constantly while recording and events
    //     are coming in.  It is one possible source of the increasing CPU
    //     usage while recording.  For the recording case, the obvious
    //     optimization would be to add the new notes to the existing
    //     cached preview rather than regenerating the preview.
    Profiler profiler("CompositionModelImpl::makeNotationPreview()");

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

        //RG_DEBUG << "CompositionModelImpl::makeNotationPreview(): x = " << x << ", width = " << width << " (time = " << eventStart << ", duration = " << eventEnd - eventStart << ")";

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

// --- Audio Previews -----------------------------------------------

void CompositionModelImpl::setAudioPreviewThread(AudioPreviewThread *thread)
{
    //RG_DEBUG << "\nCompositionModelImpl::setAudioPreviewThread()";

    // For each AudioPreviewUpdater
    while (!m_audioPreviewUpdaterMap.empty()) {
        // Cause any running previews to be cancelled
        delete m_audioPreviewUpdaterMap.begin()->second;
        m_audioPreviewUpdaterMap.erase(m_audioPreviewUpdaterMap.begin());
    }

    m_audioPreviewThread = thread;
}

void CompositionModelImpl::makeAudioPreview(
        AudioPreviews* apRects, const Segment* segment,
        const SegmentRect& segRect)
{
    Profiler profiler("CompositionModelImpl::makeAudioPreview");

    RG_DEBUG << "CompositionModelImpl::makeAudioPreview - segRect = " << segRect;

    if (!apRects)
        return;

    // ??? Parameter order is wrong.  audioPreviews, the out
    //     parameter, belongs at the end.

    // ??? This is the only call to this function.  Inline it.
    QImageVector previewImage = getAudioPreviewImage(segment);

    // ??? COPY.  Why not create this object earlier and build the
    //     preview in it?  That would avoid the copy.
    AudioPreview previewItem(previewImage, segRect);

    if (m_changeType == ChangeResizeFromStart) {
        SegmentRect originalRect = computeSegmentRect(*segment);
        previewItem.resizeOffset = segRect.x() - originalRect.x();
    }

    apRects->push_back(previewItem);
}

CompositionModelImpl::QImageVector
CompositionModelImpl::getAudioPreviewImage(const Segment* s)
{
    // If needed, begin the asynchronous process of generating an
    // audio preview.
    getAudioPeaks(s);

    return m_audioPreviewImageCache[s];
}

CompositionModelImpl::AudioPeaks* CompositionModelImpl::getAudioPeaks(const Segment* s)
{
    Profiler profiler("CompositionModelImpl::getAudioPeaks");
    //RG_DEBUG << "CompositionModelImpl::getAudioPeaks";

    /**
     * ??? This is called recursively.  This triggers the async preview
     *     generation process (updateCachedAudioPeaks()) and is called
     *     again once the process completes
     *     (by slotAudioPeaksComplete()) to get the info from the cache.
     *     This is too tangled.  Simplify.
     */

    AudioPeaks* apData = m_audioPeaksCache[s];

    if (!apData) {
        apData = updateCachedAudioPeaks(s);
    }

    //RG_DEBUG << "CompositionModelImpl::getAudioPeaks returning";
    return apData;
}

CompositionModelImpl::AudioPeaks *
CompositionModelImpl::updateCachedAudioPeaks(const Segment *s)
{
    //RG_DEBUG << "updateCachedAudioPeaks(" << s << ")";

    // ??? makePreviewCache() calls this.  That's probably not necessary
    //     given that one way or another the preview generation will occur.
    //     If we can get rid of the call from makePreviewCache(), we can
    //     inline this into its only remaining caller, getAudioPeaks().

    AudioPeaks* apData = new AudioPeaks();
    makeAudioPeaksAsync(s);

    // Avoid potential memory leaks.
    //delete m_audioPeaksCache[s];

    m_audioPeaksCache[s] = apData;

    return apData;
}

void CompositionModelImpl::makeAudioPeaksAsync(const Segment* segment)
{
    if (m_audioPreviewThread) {
        //RG_DEBUG << "makeAudioPeaksAsync() - new audio preview started";

        SegmentRect segRect = computeSegmentRect(*segment);
        segRect.setWidth(segRect.getBaseWidth()); // don't use repeating area
        segRect.moveTopLeft(QPoint(0, 0));

        if (m_audioPreviewUpdaterMap.find(segment) ==
                m_audioPreviewUpdaterMap.end()) {

            AudioPreviewUpdater *updater =
                    new AudioPreviewUpdater(
                            *m_audioPreviewThread, m_composition,
                            segment, segRect, this);

            connect(updater, SIGNAL(audioPreviewComplete(AudioPreviewUpdater*)),
                    this, SLOT(slotAudioPeaksComplete(AudioPreviewUpdater*)));

            m_audioPreviewUpdaterMap[segment] = updater;

        } else {

            m_audioPreviewUpdaterMap[segment]->setDisplayExtent(segRect);
        }

        m_audioPreviewUpdaterMap[segment]->update();

    } else {
        RG_DEBUG << "makeAudioPeaksAsync() - no audio preview thread set";
    }
}

void CompositionModelImpl::slotAudioPeaksComplete(AudioPreviewUpdater* apu)
{
    RG_DEBUG << "CompositionModelImpl::slotAudioPeaksComplete()";

    AudioPeaks *apData = getAudioPeaks(apu->getSegment());
    QRect updateRect;

    if (apData) {
        RG_DEBUG << "CompositionModelImpl::slotAudioPeaksComplete(" << apu << "): apData contains " << apData->values.size() << " values already";
        unsigned int channels = 0;
        const std::vector<float> &values = apu->getComputedValues(channels);
        if (channels > 0) {
            RG_DEBUG << "CompositionModelImpl::slotAudioPeaksComplete: set "
                << values.size() << " samples on " << channels << " channels";
            apData->channels = channels;
            apData->values = values;  // ??? COPY performance issue?
            // ??? This is the only call to this function.  Inline it.
            updateRect = updateCachedPreviewImage(apData, apu->getSegment());
        }
    }

    if (!updateRect.isEmpty())
        emit needUpdate(updateRect);
}

QRect CompositionModelImpl::updateCachedPreviewImage(AudioPeaks* apData, const Segment* segment)
{
    //RG_DEBUG << "updateCachedPreviewImage()";

    AudioPreviewPainter previewPainter(*this, apData, m_composition, segment);
    previewPainter.paintPreviewImage();

    m_audioPreviewImageCache[segment] = previewPainter.getPreviewImage();

    return previewPainter.getSegmentRect();
}

// --- Previews -----------------------------------------------------

void CompositionModelImpl::slotInstrumentChanged(Instrument *instrument)
{
    RG_DEBUG << "slotInstrumentChanged()";
    const SegmentMultiSet& segments = m_composition.getSegments();
    SegmentMultiSet::const_iterator segEnd = segments.end();

    for (SegmentMultiSet::const_iterator i = segments.begin();
         i != segEnd; ++i) {

        const Segment* s = *i;
        TrackId trackId = s->getTrack();
        Track *track = getComposition().getTrackById(trackId);

        // We need to update the cache for audio segments, because the
        // instrument playback level is reflected in the audio
        // preview.  And we need to update it for midi segments,
        // because the preview style differs depending on whether the
        // segment is on a percussion instrument or not

        if (track && track->getInstrument() == instrument->getId()) {
            deleteCachedPreview(s);
            emit needUpdate(computeSegmentRect(*s));
        }
    }
}

void CompositionModelImpl::deleteCachedPreview(const Segment *s)
{
    if (s->getType() == Segment::Internal) {
        NotationPreview *notationPreview = m_notationPreviewCache[s];
        delete notationPreview;
        m_notationPreviewCache.erase(s);
    } else {
        AudioPeaks *apd = m_audioPeaksCache[s];
        delete apd;
        m_audioPeaksCache.erase(s);
        m_audioPreviewImageCache.erase(s);
    }

}

void CompositionModelImpl::deleteCachedPreviews()
{
    //RG_DEBUG << "deleteCachedPreviews";

    for (NotationPreviewCache::iterator i = m_notationPreviewCache.begin();
         i != m_notationPreviewCache.end(); ++i) {
        delete i->second;
    }
    for (AudioPeaksCache::iterator i = m_audioPeaksCache.begin();
         i != m_audioPeaksCache.end(); ++i) {
        delete i->second;
    }

    m_notationPreviewCache.clear();
    m_audioPeaksCache.clear();

    m_audioPreviewImageCache.clear();

    for (AudioPreviewUpdaterMap::iterator i = m_audioPreviewUpdaterMap.begin();
         i != m_audioPreviewUpdaterMap.end(); ++i) {
        i->second->cancel();
    }

    const SegmentMultiSet& segments = m_composition.getSegments();
    SegmentMultiSet::const_iterator segEnd = segments.end();

    // Regenerate all of the audio previews.
    // ??? Why?  This routine is supposed to delete all the previews.
    //     Why is it regenerating the audio previews?  Split this out
    //     into an updateCachedAudioPreviews() and call it where it's
    //     needed.  Then determine whether it is really needed.

    for (SegmentMultiSet::const_iterator i = segments.begin();
            i != segEnd; ++i) {

        if ((*i)->getType() == Segment::Audio) {
            // This will create the audio preview updater.  The
            // preview won't be calculated and cached until the
            // updater completes and calls back.
            makeAudioPeaksAsync((*i));
        }
    }
}

// --- Selection ----------------------------------------------------

void CompositionModelImpl::setSelected(Segment *segment, bool selected)
{
    if (!segment) {
        RG_DEBUG << "WARNING : CompositionModelImpl::setSelected() - segment is NULL";
        return;
    }

    //RG_DEBUG << "CompositionModelImpl::setSelected " << segment << " - " << selected;

    // Update m_selectedSegments
    if (selected) {
        if (!isSelected(segment))
            m_selectedSegments.insert(segment);
    } else {
        SegmentSelection::iterator i = m_selectedSegments.find(segment);
        if (i != m_selectedSegments.end())
            m_selectedSegments.erase(i);
    }

    emit needUpdate();
}

void CompositionModelImpl::selectSegments(const SegmentSelection &segments)
{
    m_selectedSegments = segments;
    emit needUpdate();
}

void CompositionModelImpl::clearSelected()
{
    //RG_DEBUG << "CompositionModelImpl::clearSelected";

    m_selectedSegments.clear();
    emit needUpdate();
}

void CompositionModelImpl::setSelectionRect(const QRect &rect)
{
    m_selectionRect = rect.normalized();

    //RG_DEBUG << "setSelectionRect: " << r << " -> " << m_selectionRect;

    m_previousTmpSelectedSegments = m_tmpSelectedSegments;
    m_tmpSelectedSegments.clear();

    const SegmentMultiSet& segments = m_composition.getSegments();
    SegmentMultiSet::iterator segEnd = segments.end();

    QRect updateRect = m_selectionRect;

    // For each segment in the composition
    for (SegmentMultiSet::iterator i = segments.begin();
         i != segEnd; ++i) {

        SegmentRect segmentRect = computeSegmentRect(**i);

        if (segmentRect.intersects(m_selectionRect)) {
            m_tmpSelectedSegments.insert(*i);
            updateRect |= segmentRect;
        }
    }

    updateRect = updateRect.normalized();

    if (!updateRect.isNull() && !m_previousSelectionUpdateRect.isNull()) {

        if (m_tmpSelectedSegments != m_previousTmpSelectedSegments)
            emit needUpdate(updateRect | m_previousSelectionUpdateRect);

        emit needArtifactsUpdate();
    }

    m_previousSelectionUpdateRect = updateRect;
}

void CompositionModelImpl::finalizeSelectionRect()
{
    const SegmentMultiSet &segments = m_composition.getSegments();
    SegmentMultiSet::const_iterator segEnd = segments.end();

    // For each segment in the composition
    for (SegmentMultiSet::const_iterator i = segments.begin();
         i != segEnd; ++i) {

        SegmentRect segmentRect = computeSegmentRect(**i);

        if (segmentRect.intersects(m_selectionRect)) {
            setSelected(*i);
        }
    }

    // Clear the selection rect state for the next time.
    m_previousSelectionUpdateRect = m_selectionRect = QRect();
    m_tmpSelectedSegments.clear();
}

void CompositionModelImpl::selectionHasChanged()
{
    emit selectionChanged(getSelectedSegments());
}

bool CompositionModelImpl::isSelected(const Segment *s) const
{
    return m_selectedSegments.find(const_cast<Segment *>(s)) != m_selectedSegments.end();
}

QRect CompositionModelImpl::getSelectedSegmentsRect()
{
    QRect selectionRect;

    // For each selected segment, accumulate the selection rect
    for (SegmentSelection::iterator i = m_selectedSegments.begin();
            i != m_selectedSegments.end(); ++i) {

        SegmentRect sr = computeSegmentRect(**i);
        selectionRect |= sr;
    }

    return selectionRect;
}

bool CompositionModelImpl::isTmpSelected(const Segment *s) const
{
    return m_tmpSelectedSegments.find(const_cast<Segment *>(s)) != m_tmpSelectedSegments.end();
}

bool CompositionModelImpl::wasTmpSelected(const Segment *s) const
{
    return m_previousTmpSelectedSegments.find(const_cast<Segment *>(s)) != m_previousTmpSelectedSegments.end();
}

// --- Misc ---------------------------------------------------------

#if 0
void CompositionModelImpl::makePreviewCache(const Segment *s)
{
    if (s->getType() == Segment::Internal) {
        updateCachedNotationPreview(s);
    } else {
        updateCachedAudioPeaks(s);
    }
}
#endif

int CompositionModelImpl::getCompositionHeight()
{
    return m_grid.getYBinCoordinate(m_composition.getNbTracks());
}

CompositionModelImpl::YCoordVector CompositionModelImpl::getTrackYCoords(const QRect& rect)
{
    int top = m_grid.getYBin(rect.y());
    int bottom = m_grid.getYBin(rect.y() + rect.height());

//    RG_DEBUG << "CompositionModelImpl::getTrackYCoords: rect "
//              << rect.x() << ", " << rect.y() << ", "
//              << rect.width() << "x" << rect.height() << ", top = " << top
//              << ", bottom = " << bottom;
    
    // Update track height for all tracks.
    updateTrackHeight();
    
    CompositionModelImpl::YCoordVector list;

    for (int pos = top; pos <= bottom; ++pos) {
        int divider = m_grid.getYBinCoordinate(pos);
        list.push_back(divider);
//        RG_DEBUG << "divider at " << divider;
    }

    return list;
}

}
#include "CompositionModelImpl.moc"
