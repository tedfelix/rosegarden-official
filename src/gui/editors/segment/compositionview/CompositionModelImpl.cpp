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
#include "ChangingSegment.h"
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
    m_selectedSegments(),
    m_tmpSelectedSegments(),
    m_previousTmpSelectedSegments(),
    m_selectionRect(),
    m_previousSelectionUpdateRect(),
    m_recordingSegments(),
    m_pointerTime(0),
    m_changeType(ChangeMove),
    m_changingSegments()
{
    m_composition.addObserver(this);

    updateAllTrackHeights();

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

        SegmentRect segmentRect;
        getSegmentRect(*segment, segmentRect);

        // If this segment isn't in the clip rect, try the next.
        if (!segmentRect.rect.intersects(clipRect))
            continue;

        // Update the SegmentRect's selected state.

        segmentRect.selected = (
                isSelected(segment)  ||
                isTmpSelected(segment)  ||
                segmentRect.rect.intersects(m_selectionRect));

        // Update the SegmentRect's pen and brush.

        if (!isRecording(segment)) {
            // Pen
            segmentRect.pen = colourCache->SegmentBorder;

            // Brush
            QColor brushColor = GUIPalette::convertColour(
                    m_composition.getSegmentColourMap().getColourByIndex(
                            segment->getColourIndex()));
            Qt::BrushStyle brushPattern =
                    segment->isTrulyLinked() ?
                            Qt::Dense2Pattern : Qt::SolidPattern;
            segmentRect.brush = QBrush(brushColor, brushPattern);
        } else {  // Recording
            // Pen
            segmentRect.pen = colourCache->RecordingSegmentBorder;

            // Brush
            if (segment->isMIDI())
                segmentRect.brush = colourCache->RecordingInternalSegmentBlock;
            else  // Audio Segment
                segmentRect.brush = colourCache->RecordingAudioSegmentBlock;
        }

        // Generate the requested previews.

        if (segment->isMIDI()) {
            makeNotationPreviewRange(
                    QPoint(0, segmentRect.rect.y()), segment, clipRect,
                    notationPreviewRanges);
        } else {  // Audio Segment
            makeAudioPreview(segment, segmentRect, audioPreviews);
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
        if (!segmentRect.rect.intersects(clipRect))
            continue;

        // Set up the SegmentRect

        // Selected
        segmentRect.selected = true;

        // Brush
        Segment *segment = (*i)->getSegment();
        Colour segmentColour =
                m_composition.getSegmentColourMap().getColourByIndex(
                        segment->getColourIndex());
        segmentRect.brush = GUIPalette::convertColour(segmentColour);

        // Pen
        segmentRect.pen = colourCache->SegmentBorder;

        // Generate the preview

        if (segment->isMIDI()) {
            makeNotationPreviewRangeCS(
                    segmentRect.rect.topLeft(), segment, segmentRect.rect,
                    clipRect, notationPreviewRanges);
        } else {  // Audio Segment
            makeAudioPreview(segment, segmentRect, audioPreviews);
        }

        segmentRects->push_back(segmentRect);
    }
}

ChangingSegmentPtr CompositionModelImpl::getSegmentAt(const QPoint &pos)
{
    const SegmentMultiSet &segments = m_composition.getSegments();

    // For each segment in the composition
    for (SegmentMultiSet::iterator i = segments.begin();
         i != segments.end();
         ++i) {

        Segment &segment = **i;

        SegmentRect segmentRect;
        getSegmentRect(segment, segmentRect);

        if (segmentRect.rect.contains(pos)) {
            return ChangingSegmentPtr(
                    new ChangingSegment(segment, segmentRect));
        }
    }

    // Not found.
    return ChangingSegmentPtr();
}

void CompositionModelImpl::getSegmentQRect(const Segment &segment, QRect &rect)
{
    // This routine does no caching.  Caching will be implemented later
    // if performance measurements indicate a need and we can come up
    // with an effective caching approach.  Initial performance
    // measurements show that this uses the same amount of CPU when
    // recording as the previous approach.

    const timeT startTime = segment.getStartTime();

    // Compute X (left)
    rect.setX(lround(m_grid.getRulerScale()->getXForTime(startTime)));

    // Compute Y (top)
    const int trackPosition =
            m_composition.getTrackPositionById(segment.getTrack());
    rect.setY(m_grid.getYBinCoordinate(trackPosition) +
              m_composition.getSegmentVoiceIndex(&segment) *
              m_grid.getYSnap() + 1);

    // Compute height
    rect.setHeight(m_grid.getYSnap() - 2);

    // Compute width
    int width;
    if (segment.isRepeating()) {
        width = lround(m_grid.getRulerScale()->getWidthForDuration(
                           startTime,
                           segment.getRepeatEndTime() - startTime));
    } else {
        const timeT endTime = isRecording(&segment) ?
                                  m_pointerTime :
                                  segment.getEndMarkerTime();
        width = lround(m_grid.getRulerScale()->getWidthForDuration(
                           startTime, endTime - startTime));
    }
    rect.setWidth(width);
}

void CompositionModelImpl::getSegmentRect(
        const Segment &segment, SegmentRect &segmentRect)
{
    getSegmentQRect(segment, segmentRect.rect);

    QString label = strtoqstr(segment.getLabel());
    if (segment.isAudio()) {
        // Remove anything in parens and the filename suffix.
        static QRegExp re1("( *\\([^)]*\\))*$"); // (inserted) (copied) (etc)
        static QRegExp re2("\\.[^.]+$"); // filename suffix
        label.replace(re1, "").replace(re2, "");
    }
    segmentRect.label = label;

    if (segment.isRepeating()) {
        computeRepeatMarks(segment, segmentRect);
    } else {
        segmentRect.repeatMarks.clear();
        segmentRect.baseWidth = segmentRect.rect.width();
    }

    // Reset remaining fields.
    segmentRect.selected = false;
    segmentRect.brush = SegmentRect::DefaultBrushColor;
    segmentRect.pen = SegmentRect::DefaultPenColor;
}

void CompositionModelImpl::updateAllTrackHeights()
{
    // For each track in the composition
    for (Composition::trackcontainer::const_iterator i =
             m_composition.getTracks().begin();
         i != m_composition.getTracks().end();
         ++i) {

        const TrackId trackId = i->first;
        const Track *track = i->second;

        int heightMultiple =
                m_composition.getMaxContemporaneousSegmentsOnTrack(trackId);
        if (heightMultiple == 0)
            heightMultiple = 1;

        const int bin = track->getPosition();

        m_grid.setBinHeightMultiple(bin, heightMultiple);
    }
}

void CompositionModelImpl::computeRepeatMarks(
        const Segment &segment, SegmentRect &segmentRect) const
{
    if (!segment.isRepeating())
        return;

    const timeT startTime = segment.getStartTime();
    const timeT endTime = segment.getEndMarkerTime();
    const timeT repeatInterval = endTime - startTime;

    if (repeatInterval <= 0)
        return;

    const timeT repeatStart = endTime;
    const timeT repeatEnd = segment.getRepeatEndTime();

    segmentRect.rect.setWidth(
            lround(m_grid.getRulerScale()->getWidthForDuration(
                    startTime, repeatEnd - startTime)));

    segmentRect.repeatMarks.clear();

    // For each repeat mark
    for (timeT repeatMark = repeatStart;
         repeatMark < repeatEnd;
         repeatMark += repeatInterval) {

        const int markX =
                lround(m_grid.getRulerScale()->getXForTime(repeatMark));
        segmentRect.repeatMarks.push_back(markX);
    }

    if (!segmentRect.repeatMarks.empty()) {
        segmentRect.baseWidth =
                segmentRect.repeatMarks[0] - segmentRect.rect.x();
    } else {
        segmentRect.baseWidth = segmentRect.rect.width();
    }
}

void CompositionModelImpl::segmentAdded(const Composition *, Segment *s)
{
    // Keep tabs on it.
    s->addObserver(this);

    // TrackEditor::commandExecuted() already updates us.  However, it
    // shouldn't.  This is the right thing to do.
    emit needUpdate();
}

void CompositionModelImpl::segmentRemoved(const Composition *, Segment *s)
{
    // Be tidy or else Segment's dtor will complain.
    s->removeObserver(this);

    deleteCachedPreview(s);
    m_selectedSegments.erase(s);
    m_recordingSegments.erase(s);

    // TrackEditor::commandExecuted() already updates us.  However, it
    // shouldn't.  This is the right thing to do.
    emit needUpdate();
}

void CompositionModelImpl::segmentTrackChanged(
        const Composition *, Segment *, TrackId /*tid*/)
{
    // TrackEditor::commandExecuted() already updates us.  However, it
    // shouldn't.  This is the right thing to do.
    emit needUpdate();
}

void CompositionModelImpl::segmentStartChanged(
        const Composition *, Segment *, timeT)
{
    // TrackEditor::commandExecuted() already updates us.  However, it
    // shouldn't.  This is the right thing to do.
    emit needUpdate();
}

void CompositionModelImpl::segmentEndMarkerChanged(
        const Composition *, Segment *, bool)
{
    Profiler profiler("CompositionModelImpl::segmentEndMarkerChanged()");

    // ??? This routine gets hit really hard when recording.
    //     Just holding down a single note results in 50 calls
    //     per second.

    // TrackEditor::commandExecuted() already updates us.  However, it
    // shouldn't.  This is the right thing to do.
    emit needUpdate();
}

void CompositionModelImpl::segmentRepeatChanged(
        const Composition *, Segment *, bool)
{
    // TrackEditor::commandExecuted() already updates us.  However, it
    // shouldn't.  This is the right thing to do.
    emit needUpdate();
}

void CompositionModelImpl::endMarkerTimeChanged(const Composition *, bool)
{
    // The size of the composition has changed.

    // TrackEditor::commandExecuted() already updates us.  However, it
    // shouldn't.  This is the right thing to do.
    emit needSizeUpdate();
}

// --- Recording ----------------------------------------------------

void CompositionModelImpl::addRecordingItem(ChangingSegmentPtr changingSegment)
{
    m_recordingSegments.insert(changingSegment->getSegment());

    emit needUpdate();
}

void CompositionModelImpl::pointerPosChanged(int x)
{
    // Update the end point for the recording segments.
    m_pointerTime = m_grid.getRulerScale()->getTimeForX(x);

    // For each recording segment
    for (RecordingSegmentSet::iterator i = m_recordingSegments.begin();
         i != m_recordingSegments.end();
         ++i) {

        QRect rect;
        getSegmentQRect(**i, rect);
        // Ask CompositionView to update.
        emit needUpdate(rect);
    }
}

void CompositionModelImpl::clearRecordingItems()
{
    // For each recording segment
    for (RecordingSegmentSet::iterator i = m_recordingSegments.begin();
         i != m_recordingSegments.end();
         ++i)
        deleteCachedPreview(*i);

    m_recordingSegments.clear();

    emit needUpdate();
}

bool CompositionModelImpl::isRecording(const Segment *s) const
{
    return (m_recordingSegments.find(const_cast<Segment *>(s)) !=
            m_recordingSegments.end());
}

void CompositionModelImpl::slotAudioFileFinalized(Segment *s)
{
    // Recording is finished, and the Audio preview is ready to display.
    // Clear the old one out of the cache so the new one will appear.
    // Works fine even without this line, but I suspect this is
    // because of TrackEditor::commandExecuted().  If so, then this
    // needs to be here.
    deleteCachedPreview(s);
}

// --- Changing -----------------------------------------------------

void CompositionModelImpl::startChange(
        ChangingSegmentPtr changingSegment, ChangeType changeType)
{
    m_changeType = changeType;

    // If we weren't aware that this segment is changing
    if (m_changingSegments.find(changingSegment) == m_changingSegments.end()) {
        // Save the original rectangle for this segment
        changingSegment->saveRect();

        m_changingSegments.insert(changingSegment);
    }
}

void CompositionModelImpl::startChangeSelection(ChangeType changeType)
{
    // For each selected segment
    for (SegmentSelection::iterator i = m_selectedSegments.begin();
         i != m_selectedSegments.end();
         ++i) {

        Segment &segment = **i;

        // Make a ChangingSegment
        SegmentRect segmentRect;
        getSegmentRect(segment, segmentRect);
        ChangingSegmentPtr changingSegment(
                new ChangingSegment(segment, segmentRect));

        startChange(changingSegment, changeType);
    }
}

ChangingSegmentPtr CompositionModelImpl::findChangingSegment(
        const Segment *segment)
{
    // For each changing segment
    for (ChangingSegmentSet::const_iterator it = m_changingSegments.begin();
         it != m_changingSegments.end();
         ++it) {

        ChangingSegmentPtr changingSegment = *it;

        // If this one has the Segment we're looking for, return it.
        if (changingSegment->getSegment() == segment)
            return changingSegment;
    }

    // Not found.
    return ChangingSegmentPtr();
}

void CompositionModelImpl::endChange()
{
    m_changingSegments.clear();

    emit needUpdate();
}

bool CompositionModelImpl::isChanging(const Segment *s) const
{
    // For each ChangingSegment
    for (ChangingSegmentSet::const_iterator i = m_changingSegments.begin();
         i != m_changingSegments.end();
         ++i) {

        // If we've found it
        if ((*i)->getSegment() == s)
            return true;
    }

    return false;
}

// --- Notation Previews --------------------------------------------

void CompositionModelImpl::eventAdded(const Segment *s, Event *)
{
    Profiler profiler("CompositionModelImpl::eventAdded()");

    // ??? This routine gets hit really hard when recording.
    //     Just holding down a single note results in 50 calls
    //     per second.

    deleteCachedPreview(s);

    QRect rect;
    getSegmentQRect(*s, rect);
    emit needUpdate(rect);
}

void CompositionModelImpl::eventRemoved(const Segment *s, Event *)
{
    Profiler profiler("CompositionModelImpl::eventRemoved()");

    // ??? This routine gets hit really hard when recording.
    //     Just holding down a single note results in 50 calls
    //     per second.

    deleteCachedPreview(s);

    QRect rect;
    getSegmentQRect(*s, rect);
    emit needUpdate(rect);
}

void CompositionModelImpl::allEventsChanged(const Segment *s)
{
    // This is called by Segment::setStartTime(timeT t).  And this
    // is the only handler in the entire system.

    deleteCachedPreview(s);

    QRect rect;
    getSegmentQRect(*s, rect);
    emit needUpdate(rect);
}

void CompositionModelImpl::appearanceChanged(const Segment *s)
{
    // Called by Segment::setLabel() and Segment::setColourIndex().

    // Preview gets regenerated anyway.
    //deleteCachedPreview(s);

    QRect rect;
    getSegmentQRect(*s, rect);
    emit needUpdate(rect);
}

void CompositionModelImpl::endMarkerTimeChanged(const Segment *s, bool shorten)
{
    Profiler profiler("CompositionModelImpl::endMarkerTimeChanged(Segment *, bool)");

    // ??? This routine gets hit really hard when recording.
    //     Just holding down a single note results in 50 calls
    //     per second.

    // Preview gets regenerated anyway.
    //deleteCachedPreview(s);

    if (shorten) {
        emit needUpdate(); // no longer know former segment dimension
    } else {
        QRect rect;
        getSegmentQRect(*s, rect);
        emit needUpdate(rect);
    }
}

void CompositionModelImpl::makeNotationPreviewRange(
        QPoint basePoint, const Segment *segment,
        const QRect &clipRect, NotationPreviewRanges *ranges)
{
    Profiler profiler("CompositionModelImpl::makeNotationPreviewRange");

    if (!ranges)
        return;

    const NotationPreview *notationPreview = getNotationPreview(segment);

    if (notationPreview->empty())
        return;

    NotationPreview::const_iterator npIter = notationPreview->begin();

    // Search for the first event that is likely to be visible.
    // ??? Performance: LINEAR SEARCH.  While recording, this uses more
    //     and more CPU.
    while (npIter != notationPreview->end()  &&
           npIter->right() < clipRect.left())
        ++npIter;

    // If no preview rects were within the clipRect, bail.
    if (npIter == notationPreview->end())
        return;

    NotationPreviewRange interval;
    interval.begin = npIter;

    // Compute the rightmost x coord
    const int segmentEndX = lround(m_grid.getRulerScale()->getXForTime(
            segment->getEndMarkerTime()));
    const int right = std::min(clipRect.right(), segmentEndX);

    // Search sequentially for the last visible preview rect.
    while (npIter != notationPreview->end()  &&  npIter->left() < right)
        ++npIter;

    interval.end = npIter;

    interval.segmentTop = basePoint.y();
    interval.moveXOffset = 0;
    interval.color = segment->getPreviewColour();

    // Add the interval to the caller's interval list.
    ranges->push_back(interval);
}

void CompositionModelImpl::makeNotationPreviewRangeCS(
        QPoint basePoint, const Segment *segment,
        const QRect &currentRect, const QRect &clipRect,
        NotationPreviewRanges *ranges)
{
    if (!ranges)
        return;

    const NotationPreview *notationPreview = getNotationPreview(segment);

    if (notationPreview->empty())
        return;

    QRect originalRect;
    getSegmentQRect(*segment, originalRect);

    int moveXOffset = 0;
    if (m_changeType == ChangeMove)
        moveXOffset = basePoint.x() - originalRect.x();

    int left;

    if (m_changeType == ChangeResizeFromStart) {
        left = currentRect.left();
    } else {
        left = originalRect.left();
    }

    left = std::max(clipRect.left() - moveXOffset, left);

    NotationPreview::const_iterator npIter = notationPreview->begin();

    // Search for the first event that is likely to be visible.
    while (npIter != notationPreview->end()  &&
           npIter->right() < left)
        ++npIter;

    // Nothing found, bail.
    if (npIter == notationPreview->end())
        return;

    NotationPreviewRange interval;
    interval.begin = npIter;

    // Compute the rightmost x coord
    int right = (m_changeType == ChangeMove) ?
            originalRect.right() :
            currentRect.right();

    right = std::min(clipRect.right() - moveXOffset, right);

    // Search sequentially for the last visible preview rect.
    while (npIter != notationPreview->end()  &&  npIter->left() < right)
        ++npIter;

    interval.end = npIter;
    interval.segmentTop = basePoint.y();
    interval.moveXOffset = moveXOffset;
    interval.color = segment->getPreviewColour();

    ranges->push_back(interval);
}

const CompositionModelImpl::NotationPreview *
CompositionModelImpl::getNotationPreview(const Segment *segment)
{
    // Try the cache.
    NotationPreviewCache::const_iterator previewIter =
            m_notationPreviewCache.find(segment);

    // If it was in the cache, return it.
    if (previewIter != m_notationPreviewCache.end())
        return previewIter->second;

    NotationPreview *notationPreview = makeNotationPreview(segment);

    m_notationPreviewCache[segment] = notationPreview;

    return notationPreview;
}

CompositionModelImpl::NotationPreview *
CompositionModelImpl::makeNotationPreview(
        const Segment *segment)
{
    Profiler profiler("CompositionModelImpl::makeNotationPreview()");

    // ??? This routine is called 50 times a second when recording and
    //     a single key is held down.
    // ??? This routine is one possible source of the increasing CPU
    //     usage while recording.  For the recording case, the obvious
    //     optimization would be to add the new notes to the existing
    //     cached preview rather than regenerating the preview.

    NotationPreview *notationPreview = new NotationPreview;

    int segStartX = lround(
            m_grid.getRulerScale()->getXForTime(segment->getStartTime()));

    bool isPercussion = false;
    Track *track = m_composition.getTrackById(segment->getTrack());
    if (track) {
        InstrumentId iid = track->getInstrument();
        Instrument *instrument = m_studio.getInstrumentById(iid);
        if (instrument  &&  instrument->isPercussion())
            isPercussion = true;
    }

    // For each event in the segment
    for (Segment::const_iterator i = segment->begin();
         i != segment->end();
         ++i) {

        Event *event = *i;

        // If this isn't a note, try the next event.
        if (!event->isa(Note::EventType))
            continue;

        long pitch = 0;
        // Get the pitch.  If there is no pitch property, try the next event.
        if (!event->get<Int>(BaseProperties::PITCH, pitch))
            continue;

        const timeT eventStart = event->getAbsoluteTime();
        const timeT eventEnd = eventStart + event->getDuration();

        int x = lround(
                m_grid.getRulerScale()->getXForTime(eventStart));
        int width = lround(
                m_grid.getRulerScale()->getWidthForDuration(
                        eventStart, eventEnd - eventStart));

        // If the event starts on or before the segment border
        if (x <= segStartX) {
            // Move the left edge to the right by 1
            ++x;
            // But leave the right edge alone.
            if (width > 1)
                --width;
        }

        // Make sure we draw something.
        if (width < 1)
            width = 1;

        const int y0 = 0;
        const int y1 = m_grid.getYSnap();
        int y = lround(y1 + ((y0 - y1) * (pitch - 16)) / 96.0);

        int height = 1;

        // On a percussion track...
        if (isPercussion) {
            height = 2;
            // Make events appear as dots instead of lines.
            if (width > 2)
                width = 2;
        }

        if (y < y0)
            y = y0;
        if (y > y1 - height + 1)
            y = y1 - height + 1;

        QRect r(x, y, width, height);

        notationPreview->push_back(r);
    }

    return notationPreview;
}

// --- Audio Previews -----------------------------------------------

void CompositionModelImpl::setAudioPreviewThread(AudioPreviewThread *thread)
{
    // For each AudioPreviewUpdater
    while (!m_audioPreviewUpdaterMap.empty()) {
        // Delete it
        delete m_audioPreviewUpdaterMap.begin()->second;
        m_audioPreviewUpdaterMap.erase(m_audioPreviewUpdaterMap.begin());
    }

    m_audioPreviewThread = thread;
}

void CompositionModelImpl::makeAudioPreview(
        const Segment *segment, const SegmentRect &segmentRect,
        AudioPreviews *audioPreviews)
{
    Profiler profiler("CompositionModelImpl::makeAudioPreview");

    if (!audioPreviews)
        return;

    // If needed, begin the asynchronous process of generating an
    // audio preview.
    getAudioPeaks(segment);

    // ??? COPY.  This copies a vector of QImage objects.  That seems
    //     wasteful.  Some sort of pointer approach should be better.
    //     Would it be ok for AudioPreview to just have a pointer
    //     into the m_audioPreviewImageCache?  Perhaps a QSharedPointer
    //     for safety?  Are the AudioPreview objects ephemeral enough?
    AudioPreview audioPreview(
            m_audioPreviewImageCache[segment], segmentRect.rect);

    if (m_changeType == ChangeResizeFromStart) {
        int originalRectX =
                lround(m_grid.getRulerScale()->getXForTime(
                        segment->getStartTime()));

        audioPreview.resizeOffset = segmentRect.rect.x() - originalRectX;
    }

    audioPreviews->push_back(audioPreview);
}

CompositionModelImpl::AudioPeaks *CompositionModelImpl::getAudioPeaks(
        const Segment *s)
{
    Profiler profiler("CompositionModelImpl::getAudioPeaks");

    // We must use find() because C++ makes no guarantee that a new
    // map element is zeroed out.  IOW, m_audioPeaksCache[s] may be undefined.
    AudioPeaksCache::const_iterator audioPeaksIter = m_audioPeaksCache.find(s);

    // If not found in the cache, begin the cache update process
    if (audioPeaksIter == m_audioPeaksCache.end())
        return updateCachedAudioPeaks(s);

    return audioPeaksIter->second;
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

        SegmentRect segRect;
        getSegmentRect(*segment, segRect);
        segRect.rect.setWidth(segRect.baseWidth); // don't use repeating area
        segRect.rect.moveTopLeft(QPoint(0, 0));

        if (m_audioPreviewUpdaterMap.find(segment) ==
                m_audioPreviewUpdaterMap.end()) {

            AudioPreviewUpdater *updater =
                    new AudioPreviewUpdater(
                            *m_audioPreviewThread, m_composition,
                            segment, segRect.rect, this);

            connect(updater, SIGNAL(audioPreviewComplete(AudioPreviewUpdater*)),
                    this, SLOT(slotAudioPeaksComplete(AudioPreviewUpdater*)));

            m_audioPreviewUpdaterMap[segment] = updater;

        } else {

            m_audioPreviewUpdaterMap[segment]->setDisplayExtent(segRect.rect);
        }

        m_audioPreviewUpdaterMap[segment]->update();

    } else {
        RG_DEBUG << "makeAudioPeaksAsync() - no audio preview thread set";
    }
}

void CompositionModelImpl::slotAudioPeaksComplete(AudioPreviewUpdater* apu)
{
    // ??? We already know it's in m_audioPeaksCache.
    //     updateCachedAudioPeaks() put it there.  Just pull it out of
    //     m_audioPeaksCache.
    //     No need for this "recursive" call to the function that called us.
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

    return previewPainter.getSegmentRect().rect;
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
            QRect rect;
            getSegmentQRect(*s, rect);
            emit needUpdate(rect);
        }
    }
}

void CompositionModelImpl::deleteCachedPreview(const Segment *s)
{
    // For NULL, delete everything.
    if (!s) {
        deleteCachedPreviews();
        return;
    }

    if (s->getType() == Segment::Internal) {
        // ??? Need to handle the case when it isn't there.
        NotationPreview *notationPreview = m_notationPreviewCache[s];
        delete notationPreview;
        m_notationPreviewCache.erase(s);
    } else {
        // ??? Need to handle the case when it isn't there.
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

        QRect segmentRect;
        getSegmentQRect(**i, segmentRect);

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

        QRect segmentRect;
        getSegmentQRect(**i, segmentRect);

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

        QRect sr;
        getSegmentQRect(**i, sr);
        selectionRect |= sr;
    }

    return selectionRect;
}

bool CompositionModelImpl::isTmpSelected(const Segment *s) const
{
    return m_tmpSelectedSegments.find(const_cast<Segment *>(s)) != m_tmpSelectedSegments.end();
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
    
    updateAllTrackHeights();
    
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
