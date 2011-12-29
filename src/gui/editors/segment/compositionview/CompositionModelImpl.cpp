/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "CompositionModelImpl.h"
#include "SegmentOrderer.h"
#include "AudioPreviewThread.h"
#include "AudioPreviewUpdater.h"
#include "AudioPreviewPainter.h"
#include "CompositionItemHelper.h"
#include "CompositionItemImpl.h"
#include "CompositionModel.h"
#include "CompositionRect.h"
#include "CompositionColourCache.h"

#include "base/BaseProperties.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Composition.h"
#include "base/Event.h"
#include "base/MidiProgram.h"
#include "base/NotationTypes.h"
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
#include <QPen>
#include <QPoint>
#include <QRect>
#include <QRegExp>
#include <QSize>
#include <QString>

#include <cmath>
#include <algorithm>


namespace Rosegarden
{

CompositionModelImpl::CompositionModelImpl(Composition& compo,
        Studio& studio,
        RulerScale *rulerScale,
        int vStep)
        : m_composition(compo),
        m_studio(studio),
        m_grid(rulerScale, vStep),
        m_pointerTimePos(0),
        m_audioPreviewThread(0)
{
    m_composition.addObserver(this);

    setTrackHeights();

    Composition::segmentcontainer& segments = m_composition.getSegments();
    Composition::segmentcontainer::iterator segEnd = segments.end();

    for (Composition::segmentcontainer::iterator i = segments.begin();
            i != segEnd; ++i) {

        (*i)->addObserver(this);
    }
}

CompositionModelImpl::~CompositionModelImpl()
{
    RG_DEBUG << "CompositionModelImpl::~CompositionModelImpl()";

    if (!isCompositionDeleted()) {

        m_composition.removeObserver(this);

        Composition::segmentcontainer& segments = m_composition.getSegments();
        Composition::segmentcontainer::iterator segEnd = segments.end();

        for (Composition::segmentcontainer::iterator i = segments.begin();
                i != segEnd; ++i) {

            (*i)->removeObserver(this);
        }
    }

    RG_DEBUG << "CompositionModelImpl::~CompositionModelImpl(): removal from Segment & Composition observers OK";

    if (m_audioPreviewThread) {
        while (!m_audioPreviewUpdaterMap.empty()) {
            // Cause any running previews to be cancelled
            delete m_audioPreviewUpdaterMap.begin()->second;
            m_audioPreviewUpdaterMap.erase(m_audioPreviewUpdaterMap.begin());
        }
    }

    for (NotationPreviewDataCache::iterator i = m_notationPreviewDataCache.begin();
         i != m_notationPreviewDataCache.end(); ++i) {
        delete i->second;
    }
    for (AudioPreviewDataCache::iterator i = m_audioPreviewDataCache.begin();
         i != m_audioPreviewDataCache.end(); ++i) {
        delete i->second;
    }
}

struct RectCompare {
    bool operator()(const QRect &r1, const QRect &r2) const {
        return r1.x() < r2.x();
    }
};

void CompositionModelImpl::makeNotationPreviewRects(RectRanges* npRects, QPoint basePoint,
        const Segment* segment, const QRect& clipRect)
{
    Profiler profiler("CompositionModelImpl::makeNotationPreviewRects");

    rectlist* cachedNPData = getNotationPreviewData(segment);

    if (cachedNPData->empty())
        return ;

    rectlist::iterator npEnd = cachedNPData->end();

    rectlist::iterator npi = std::lower_bound(cachedNPData->begin(), npEnd, clipRect, RectCompare());

    if (npi == npEnd)
        return ;

    if (npi != cachedNPData->begin())
        --npi;

    RectRange interval;

    interval.range.first = npi;

    int segEndX = int(nearbyint(m_grid.getRulerScale()->getXForTime(segment->getEndMarkerTime())));
    int xLim = std::min(clipRect.topRight().x(), segEndX);

    //RG_DEBUG << "CompositionModelImpl::makeNotationPreviewRects : basePoint.x : "
    //         << basePoint.x();

    // move iterator forward
    //
    while (npi != npEnd && npi->x() < xLim)
        ++npi;

    interval.range.second = npi;
    interval.basePoint.setX(0);
    interval.basePoint.setY(basePoint.y());
    interval.color = computeSegmentPreviewColor(segment);

    npRects->push_back(interval);
}

void CompositionModelImpl::makeNotationPreviewRectsMovingSegment(RectRanges* npRects, QPoint basePoint,
        const Segment* segment, const QRect& currentSR)
{
    CompositionRect unmovedSR = computeSegmentRect(*segment);

    rectlist* cachedNPData = getNotationPreviewData(segment);

    if (cachedNPData->empty())
        return ;

    rectlist::iterator npEnd = cachedNPData->end(),
                               npBegin = cachedNPData->begin();

    rectlist::iterator npi;

    if (getChangeType() == ChangeResizeFromStart)
        npi = std::lower_bound(npBegin, npEnd, currentSR, RectCompare());
    else
        npi = std::lower_bound(npBegin, npEnd, unmovedSR, RectCompare());

    if (npi == npEnd)
        return ;

    if (npi != npBegin && getChangeType() != ChangeResizeFromStart) {
        --npi;
    }

    RectRange interval;

    interval.range.first = npi;

    int xLim = getChangeType() == ChangeMove ? unmovedSR.topRight().x() : currentSR.topRight().x();

    //RG_DEBUG << "CompositionModelImpl::makeNotationPreviewRectsMovingSegment : basePoint.x : "
    //         << basePoint.x();

    // move iterator forward
    //
    while (npi != npEnd && npi->x() < xLim)
        ++npi;

    interval.range.second = npi;
    interval.basePoint.setY(basePoint.y());

    if (getChangeType() == ChangeMove)
        interval.basePoint.setX(basePoint.x() - unmovedSR.x());
    else
        interval.basePoint.setX(0);

    interval.color = computeSegmentPreviewColor(segment);

    npRects->push_back(interval);
}

void CompositionModelImpl::makeAudioPreviewRects(AudioPreviewDrawData* apRects, const Segment* segment,
        const CompositionRect& segRect, const QRect& /*clipRect*/)
{
    Profiler profiler("CompositionModelImpl::makeAudioPreviewRects");

    RG_DEBUG << "CompositionModelImpl::makeAudioPreviewRects - segRect = " << segRect;

    PixmapArray previewImage = getAudioPreviewPixmap(segment);

    QPoint basePoint = segRect.topLeft();

    AudioPreviewDrawDataItem previewItem(previewImage, basePoint, segRect);

    if (getChangeType() == ChangeResizeFromStart) {
        CompositionRect originalRect = computeSegmentRect(*segment);
        previewItem.resizeOffset = segRect.x() - originalRect.x();
    }

    apRects->push_back(previewItem);
}

void CompositionModelImpl::computeRepeatMarks(CompositionItem& item)
{
    Segment* s = CompositionItemHelper::getSegment(item);
    CompositionRect& sr = dynamic_cast<CompositionItemImpl*>((_CompositionItem*)item)->getCompRect();
    computeRepeatMarks(sr, s);
}

void CompositionModelImpl::computeRepeatMarks(CompositionRect& sr, const Segment* s)
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

        CompositionRect::repeatmarks repeatMarks;

        //RG_DEBUG << "CompositionModelImpl::computeRepeatMarks : repeatStart = "
        //         << repeatStart << " - repeatEnd = " << repeatEnd;

        for (timeT repeatMark = repeatStart; repeatMark < repeatEnd; repeatMark += repeatInterval) {
            int mark = int(nearbyint(m_grid.getRulerScale()->getXForTime(repeatMark)));
            //RG_DEBUG << "CompositionModelImpl::computeRepeatMarks : mark at " << mark;
            repeatMarks.push_back(mark);
        }
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

void CompositionModelImpl::setAudioPreviewThread(AudioPreviewThread *thread)
{
    //RG_DEBUG << "\nCompositionModelImpl::setAudioPreviewThread()";

    while (!m_audioPreviewUpdaterMap.empty()) {
        // Cause any running previews to be cancelled
        delete m_audioPreviewUpdaterMap.begin()->second;
        m_audioPreviewUpdaterMap.erase(m_audioPreviewUpdaterMap.begin());
    }

    m_audioPreviewThread = thread;
}

void CompositionModelImpl::clearPreviewCache()
{
    //RG_DEBUG << "CompositionModelImpl::clearPreviewCache";

    for (NotationPreviewDataCache::iterator i = m_notationPreviewDataCache.begin();
         i != m_notationPreviewDataCache.end(); ++i) {
        delete i->second;
    }
    for (AudioPreviewDataCache::iterator i = m_audioPreviewDataCache.begin();
         i != m_audioPreviewDataCache.end(); ++i) {
        delete i->second;
    }

    m_notationPreviewDataCache.clear();
    m_audioPreviewDataCache.clear();

    m_audioSegmentPreviewMap.clear();

    for (AudioPreviewUpdaterMap::iterator i = m_audioPreviewUpdaterMap.begin();
         i != m_audioPreviewUpdaterMap.end(); ++i) {
        i->second->cancel();
    }

    const Composition::segmentcontainer& segments = m_composition.getSegments();
    Composition::segmentcontainer::const_iterator segEnd = segments.end();

    for (Composition::segmentcontainer::const_iterator i = segments.begin();
            i != segEnd; ++i) {

        if ((*i)->getType() == Segment::Audio) {
            // This will create the audio preview updater.  The
            // preview won't be calculated and cached until the
            // updater completes and calls back.
            updatePreviewCacheForAudioSegment((*i), 0);
        }
    }
}

void CompositionModelImpl::updatePreviewCacheForNotationSegment(const Segment* segment, rectlist* npData)
{
    npData->clear();

    int segStartX = int(nearbyint(m_grid.getRulerScale()->getXForTime(segment->getStartTime())));

    bool isPercussion = false;
    Track *track = m_composition.getTrackById(segment->getTrack());
    if (track) {
        InstrumentId iid = track->getInstrument();
        Instrument *instrument = m_studio.getInstrumentById(iid);
        if (instrument && instrument->isPercussion()) isPercussion = true;
    }

    for (Segment::const_iterator i = segment->begin();
         i != segment->end(); ++i) {

        long pitch = 0;
        if (!(*i)->isa(Note::EventType) ||
            !(*i)->get<Int>(BaseProperties::PITCH, pitch)) {
            continue;
        }

        timeT eventStart = (*i)->getAbsoluteTime();
        timeT eventEnd = eventStart + (*i)->getDuration();
        // 	if (eventEnd > segment->getEndMarkerTime()) {
        // 	    eventEnd = segment->getEndMarkerTime();
        // 	}

        int x = int(nearbyint(m_grid.getRulerScale()->getXForTime(eventStart)));
        int width = int(nearbyint(m_grid.getRulerScale()->getWidthForDuration
                                  (eventStart,
                                   eventEnd - eventStart)));

        //RG_DEBUG << "CompositionModelImpl::updatePreviewCacheForNotationSegment: x = " << x << ", width = " << width << " (time = " << eventStart << ", duration = " << eventEnd - eventStart << ")";

        if (x <= segStartX) {
            ++x;
            if (width > 1) --width;
        }
        if (width > 1) --width;
        if (width < 1) ++width;

        double y0 = 0;
        double y1 = m_grid.getYSnap();
        double y = y1 + ((y0 - y1) * (pitch - 16)) / 96;

        int height = 1;

        if (isPercussion) {
            height = 2;
            if (width > 2) width = 2;
        }

        if (y < y0) y = y0;
        if (y > y1 - height + 1) y = y1 - height + 1;

        QRect r(x, (int)y, width, height);

        npData->push_back(r);
    }
}

QColor CompositionModelImpl::computeSegmentPreviewColor(const Segment* segment)
{
    // compute the preview color so it's as visible as possible over the segment's color
    QColor segColor = GUIPalette::convertColour(m_composition.getSegmentColourMap().getColourByIndex(segment->getColourIndex()));

    int intensity = qGray(segColor.rgb());
    if (intensity > 127) {
        segColor = Qt::black;
    } else {
        segColor = Qt::white;
    }

    return segColor;
}

void CompositionModelImpl::updatePreviewCacheForAudioSegment(const Segment* segment, AudioPreviewData* apData)
{
    if (m_audioPreviewThread) {
        //RG_DEBUG << "CompositionModelImpl::updatePreviewCacheForAudioSegment() - new audio preview started";

        CompositionRect segRect = computeSegmentRect(*segment);
        segRect.setWidth(segRect.getBaseWidth()); // don't use repeating area
        segRect.moveTopLeft(QPoint(0, 0));

        if (apData)
            apData->setSegmentRect(segRect);

        if (m_audioPreviewUpdaterMap.find(segment) ==
                m_audioPreviewUpdaterMap.end()) {

            AudioPreviewUpdater *updater = new AudioPreviewUpdater
                                           (*m_audioPreviewThread, m_composition, segment, segRect, this);

            connect(updater, SIGNAL(audioPreviewComplete(AudioPreviewUpdater*)),
                    this, SLOT(slotAudioPreviewComplete(AudioPreviewUpdater*)));

            m_audioPreviewUpdaterMap[segment] = updater;

        } else {

            m_audioPreviewUpdaterMap[segment]->setDisplayExtent(segRect);
        }

        m_audioPreviewUpdaterMap[segment]->update();

    } else {
        RG_DEBUG << "CompositionModelImpl::updatePreviewCacheForAudioSegment() - no audio preview thread set";
    }
}

void CompositionModelImpl::slotAudioPreviewComplete(AudioPreviewUpdater* apu)
{
    RG_DEBUG << "CompositionModelImpl::slotAudioPreviewComplete()";

    AudioPreviewData *apData = getAudioPreviewData(apu->getSegment());
    QRect updateRect;

    if (apData) {
        RG_DEBUG << "CompositionModelImpl::slotAudioPreviewComplete(" << apu << "): apData contains " << apData->getValues().size() << " values already";
        unsigned int channels = 0;
        const std::vector<float> &values = apu->getComputedValues(channels);
        if (channels > 0) {
            RG_DEBUG << "CompositionModelImpl::slotAudioPreviewComplete: set "
                << values.size() << " samples on " << channels << " channels";
            apData->setChannels(channels);
            apData->setValues(values);
            updateRect = postProcessAudioPreview(apData, apu->getSegment());
        }
    }

    if (!updateRect.isEmpty())
        emit needContentUpdate(updateRect);
}

QRect CompositionModelImpl::postProcessAudioPreview(AudioPreviewData* apData, const Segment* segment)
{
    //RG_DEBUG << "CompositionModelImpl::postProcessAudioPreview()";

    AudioPreviewPainter previewPainter(*this, apData, m_composition, segment);
    previewPainter.paintPreviewImage();

    m_audioSegmentPreviewMap[segment] = previewPainter.getPreviewImage();

    return previewPainter.getSegmentRect();
}

void CompositionModelImpl::slotInstrumentParametersChanged(InstrumentId id)
{
    RG_DEBUG << "CompositionModelImpl::slotInstrumentParametersChanged()";
    const Composition::segmentcontainer& segments = m_composition.getSegments();
    Composition::segmentcontainer::const_iterator segEnd = segments.end();

    for (Composition::segmentcontainer::const_iterator i = segments.begin();
         i != segEnd; ++i) {

        const Segment* s = *i;
        TrackId trackId = s->getTrack();
        Track *track = getComposition().getTrackById(trackId);

        // We need to update the cache for audio segments, because the
        // instrument playback level is reflected in the audio
        // preview.  And we need to update it for midi segments,
        // because the preview style differs depending on whether the
        // segment is on a percussion instrument or not

        if (track && track->getInstrument() == id) {
            removePreviewCache(s);
            emit needContentUpdate(computeSegmentRect(*s));
        }
    }
}

void CompositionModelImpl::slotAudioFileFinalized(Segment* s)
{
    //RG_DEBUG << "CompositionModelImpl::slotAudioFileFinalized()";
    removePreviewCache(s);
}

PixmapArray CompositionModelImpl::getAudioPreviewPixmap(const Segment* s)
{
    getAudioPreviewData(s);
    return m_audioSegmentPreviewMap[s];
}

void CompositionModelImpl::eventAdded(const Segment *s, Event *)
{
    //RG_DEBUG << "CompositionModelImpl::eventAdded()";
    removePreviewCache(s);
    emit needContentUpdate(computeSegmentRect(*s));
}

void CompositionModelImpl::eventRemoved(const Segment *s, Event *)
{
    //RG_DEBUG << "CompositionModelImpl::eventRemoved";
    removePreviewCache(s);
    emit needContentUpdate(computeSegmentRect(*s));
}

void CompositionModelImpl::appearanceChanged(const Segment *s)
{
    //RG_DEBUG << "CompositionModelImpl::appearanceChanged";
    clearInCache(s, true);
    emit needContentUpdate(computeSegmentRect(*s));
}

void CompositionModelImpl::endMarkerTimeChanged(const Segment *s, bool shorten)
{
    //RG_DEBUG << "CompositionModelImpl::endMarkerTimeChanged(" << shorten << ")";
    clearInCache(s, true);
    if (shorten) {
        emit needContentUpdate(); // no longer know former segment dimension
    } else {
        emit needContentUpdate(computeSegmentRect(*s));
    }
}

void CompositionModelImpl::makePreviewCache(const Segment *s)
{
    if (s->getType() == Segment::Internal) {
        makeNotationPreviewDataCache(s);
    } else {
        makeAudioPreviewDataCache(s);
    }
}

void CompositionModelImpl::removePreviewCache(const Segment *s)
{
    if (s->getType() == Segment::Internal) {
        rectlist *rl = m_notationPreviewDataCache[s];
        delete rl;
        m_notationPreviewDataCache.erase(s);
    } else {
        AudioPreviewData *apd = m_audioPreviewDataCache[s];
        delete apd;
        m_audioPreviewDataCache.erase(s);
        m_audioSegmentPreviewMap.erase(s);
    }

}

void CompositionModelImpl::segmentAdded(const Composition *, Segment *s)
{
    RG_DEBUG << "CompositionModelImpl::segmentAdded: segment " << s << " on track " << s->getTrack() << ": calling setTrackHeights";
    setTrackHeights(s);

    makePreviewCache(s);
    s->addObserver(this);
    emit needContentUpdate();
}

void CompositionModelImpl::segmentRemoved(const Composition *, Segment *s)
{
    setTrackHeights();

    QRect r = computeSegmentRect(*s);

    m_selectedSegments.erase(s);

    clearInCache(s, true);
    s->removeObserver(this);
    m_recordingSegments.erase(s); // this could be a recording segment
    emit needContentUpdate(r);
}

void CompositionModelImpl::segmentTrackChanged(const Composition *, Segment *s, TrackId tid)
{
    RG_DEBUG << "CompositionModelImpl::segmentTrackChanged: segment " << s << " on track " << tid << ", calling setTrackHeights";

    // we don't call setTrackHeights(s), because some of the tracks
    // above s may have changed height as well (if s was moved off one
    // of them)
    if (setTrackHeights()) {
        RG_DEBUG << "... changed, updating";
        emit needContentUpdate();
    }
}

void CompositionModelImpl::segmentStartChanged(const Composition *, Segment *s, timeT)
{
//    RG_DEBUG << "CompositionModelImpl::segmentStartChanged: segment " << s << " on track " << s->getTrack() << ": calling setTrackHeights";
    if (setTrackHeights(s)) emit needContentUpdate();
}

void CompositionModelImpl::segmentEndMarkerChanged(const Composition *, Segment *s, bool)
{
//    RG_DEBUG << "CompositionModelImpl::segmentEndMarkerChanged: segment " << s << " on track " << s->getTrack() << ": calling setTrackHeights";
    if (setTrackHeights(s)) {
//        RG_DEBUG << "... changed, updating";
        emit needContentUpdate();
    }
}

void CompositionModelImpl::segmentRepeatChanged(const Composition *, Segment *s, bool)
{
    clearInCache(s);
    setTrackHeights(s);
    emit needContentUpdate();
}

void CompositionModelImpl::endMarkerTimeChanged(const Composition *, bool)
{
    emit needSizeUpdate();
}

void CompositionModelImpl::setSelectionRect(const QRect& r)
{
    m_selectionRect = r.normalized();
//    if (m_selectionRect.y() < 0) m_selectionRect.setTop(0);

    RG_DEBUG << "setSelectionRect: " << r << " -> " << m_selectionRect;

    m_previousTmpSelectedSegments = m_tmpSelectedSegments;
    m_tmpSelectedSegments.clear();

    const Composition::segmentcontainer& segments = m_composition.getSegments();
    Composition::segmentcontainer::const_iterator segEnd = segments.end();

    QRect updateRect = m_selectionRect;

    for (Composition::segmentcontainer::const_iterator i = segments.begin();
         i != segEnd; ++i) {
        
        const Segment* s = *i;
        CompositionRect sr = computeSegmentRect(*s);
        if (sr.intersects(m_selectionRect)) {
            m_tmpSelectedSegments.insert(const_cast<Segment *>(s));
            updateRect |= sr;
        }
    }

    updateRect = updateRect.normalized();

    if (!updateRect.isNull() && !m_previousSelectionUpdateRect.isNull()) {

        if (m_tmpSelectedSegments != m_previousTmpSelectedSegments)
            emit needContentUpdate(updateRect | m_previousSelectionUpdateRect);

        emit needArtifactsUpdate();
    }


    m_previousSelectionUpdateRect = updateRect;

}

void CompositionModelImpl::finalizeSelectionRect()
{
    const Composition::segmentcontainer& segments = m_composition.getSegments();
    Composition::segmentcontainer::const_iterator segEnd = segments.end();

    for (Composition::segmentcontainer::const_iterator i = segments.begin();
         i != segEnd; ++i) {

        const Segment* s = *i;
        CompositionRect sr = computeSegmentRect(*s);
        if (sr.intersects(m_selectionRect)) {
            setSelected(s);
        }
    }

    m_previousSelectionUpdateRect = m_selectionRect = QRect();
    m_tmpSelectedSegments.clear();
}

QRect CompositionModelImpl::getSelectionContentsRect()
{
    QRect selectionRect;

    SegmentSelection sel = getSelectedSegments();
    for (SegmentSelection::iterator i = sel.begin();
            i != sel.end(); ++i) {

        Segment* s = *i;
        CompositionRect sr = computeSegmentRect(*s);
        selectionRect |= sr;
    }

    return selectionRect;
}

void CompositionModelImpl::addRecordingItem(const CompositionItem& item)
{
    m_recordingSegments.insert(CompositionItemHelper::getSegment(item));
    emit needContentUpdate();

    RG_DEBUG << "CompositionModelImpl::addRecordingItem: now have "
             << m_recordingSegments.size() << " recording items";
}

void CompositionModelImpl::removeRecordingItem(const CompositionItem &item)
{
    Segment* s = CompositionItemHelper::getSegment(item);

    m_recordingSegments.erase(s);
    clearInCache(s, true);

    emit needContentUpdate();

    RG_DEBUG << "CompositionModelImpl::removeRecordingItem: now have "
             << m_recordingSegments.size() << " recording items";
}

void CompositionModelImpl::clearRecordingItems()
{
    for (recordingsegmentset::iterator i = m_recordingSegments.begin();
            i != m_recordingSegments.end(); ++i)
        clearInCache(*i, true);

    m_recordingSegments.clear();

    emit needContentUpdate();
    RG_DEBUG << "CompositionModelImpl::clearRecordingItem";
}

bool CompositionModelImpl::isMoving(const Segment* sm) const
{
    itemcontainer::const_iterator movEnd = m_changingItems.end();

    for (itemcontainer::const_iterator i = m_changingItems.begin(); i != movEnd; ++i) {
        const CompositionItemImpl* ci = dynamic_cast<const CompositionItemImpl*>((_CompositionItem*)(*i));
        const Segment* s = ci->getSegment();
        if (sm == s)
            return true;
    }

    return false;
}

bool CompositionModelImpl::isRecording(const Segment* s) const
{
    return m_recordingSegments.find(const_cast<Segment*>(s)) != m_recordingSegments.end();
}

CompositionModel::itemcontainer CompositionModelImpl::getItemsAt(const QPoint& point)
{
    //RG_DEBUG << "CompositionModelImpl::getItemsAt()";

    itemcontainer res;

    const Composition::segmentcontainer& segments = m_composition.getSegments();

    for (Composition::segmentcontainer::const_iterator i = segments.begin();
         i != segments.end(); ++i) {

        const Segment* s = *i;

        CompositionRect sr = computeSegmentRect(*s);
        if (sr.contains(point)) {
            //RG_DEBUG << "CompositionModelImpl::getItemsAt() adding " << sr << " for segment " << s;
            CompositionItem item(new CompositionItemImpl(*const_cast<Segment *>(s),
                                                         sr));
            unsigned int z = computeZForSegment(s);
            //RG_DEBUG << "CompositionModelImpl::getItemsAt() z = " << z;
            item->setZ(z);
            res.insert(item);
        } else {
            //RG_DEBUG << "CompositionModelImpl::getItemsAt() skiping " << sr;
        }

    }

    if (res.size() == 1) { // only one segment under click point
        Segment* s = CompositionItemHelper::getSegment(*(res.begin()));
        m_segmentOrderer.segmentClicked(s);
    }

    return res;
}

void CompositionModelImpl::setPointerPos(int xPos)
{
    //RG_DEBUG << "CompositionModelImpl::setPointerPos() begin";
    m_pointerTimePos = grid().getRulerScale()->getTimeForX(xPos);

    for (recordingsegmentset::iterator i = m_recordingSegments.begin();
            i != m_recordingSegments.end(); ++i) {
        emit needContentUpdate(computeSegmentRect(**i));
    }
    //RG_DEBUG << "CompositionModelImpl::setPointerPos() end";
}

void CompositionModelImpl::setSelected(const CompositionItem& item, bool selected)
{
    const CompositionItemImpl* itemImpl = dynamic_cast<const CompositionItemImpl*>((_CompositionItem*)item);
    if (itemImpl) {
        Segment* segment = const_cast<Segment*>(itemImpl->getSegment());
        setSelected(segment, selected);
    }
}

void CompositionModelImpl::setSelected(const itemcontainer& items)
{
    for (itemcontainer::const_iterator i = items.begin(); i != items.end(); ++i) {
        setSelected(*i);
    }
}

void CompositionModelImpl::setSelected(const Segment* segment, bool selected)
{
    if( ! segment ){
        RG_DEBUG << "WARNING : CompositionModelImpl::setSelected - segment is NULL ";
        return;
    }
    RG_DEBUG << "CompositionModelImpl::setSelected " << segment << " - " << selected;
    if (selected) {
        if (!isSelected(segment))
            m_selectedSegments.insert(const_cast<Segment*>(segment));
    } else {
        SegmentSelection::iterator i = m_selectedSegments.find(const_cast<Segment*>(segment));
        if (i != m_selectedSegments.end())
            m_selectedSegments.erase(i);
    }
    emit needContentUpdate();
}

void CompositionModelImpl::signalSelection()
{
    //RG_DEBUG << "CompositionModelImpl::signalSelection()";
    emit selectedSegments(getSelectedSegments());
}

void CompositionModelImpl::signalContentChange()
{
    //RG_DEBUG << "CompositionModelImpl::signalContentChange";
    emit needContentUpdate();
}

void CompositionModelImpl::clearSelected()
{
    RG_DEBUG << "CompositionModelImpl::clearSelected";
    m_selectedSegments.clear();
    emit needContentUpdate();
}

bool CompositionModelImpl::isSelected(const CompositionItem& ci) const
{
    const CompositionItemImpl* itemImpl = dynamic_cast<const CompositionItemImpl*>((_CompositionItem*)ci);
    return itemImpl ? isSelected(itemImpl->getSegment()) : 0;
}

bool CompositionModelImpl::isSelected(const Segment* s) const
{
    return m_selectedSegments.find(const_cast<Segment*>(s)) != m_selectedSegments.end();
}

bool CompositionModelImpl::isTmpSelected(const Segment* s) const
{
    return m_tmpSelectedSegments.find(const_cast<Segment*>(s)) != m_tmpSelectedSegments.end();
}

bool CompositionModelImpl::wasTmpSelected(const Segment* s) const
{
    return m_previousTmpSelectedSegments.find(const_cast<Segment*>(s)) != m_previousTmpSelectedSegments.end();
}

void CompositionModelImpl::startChange(const CompositionItem& item, CompositionModel::ChangeType change)
{
    m_changeType = change;

    itemcontainer::iterator i = m_changingItems.find(item);

    // if an "identical" composition item has already been inserted, drop this one
    if (i != m_changingItems.end()) {
        RG_DEBUG << "CompositionModelImpl::startChange : item already in";
        m_itemGC.push_back(item);
    } else {
        item->saveRect();
        m_changingItems.insert(item);
    }
}

void CompositionModelImpl::startChangeSelection(CompositionModel::ChangeType change)
{
    SegmentSelection::iterator i = m_selectedSegments.begin();
    for (; i != m_selectedSegments.end(); ++i) {
        Segment* s = *i;
        CompositionRect sr = computeSegmentRect(*s);
        startChange(CompositionItem(new CompositionItemImpl(*s, sr)), change);
    }

}

void CompositionModelImpl::endChange()
{
    for (itemcontainer::const_iterator i = m_changingItems.begin(); i != m_changingItems.end(); ++i) {
        delete *i;
    }

    m_changingItems.clear();

    for (itemgc::iterator i = m_itemGC.begin(); i != m_itemGC.end(); ++i) {
        delete *i;
    }
    m_itemGC.clear();
    RG_DEBUG << "CompositionModelImpl::endChange";
    emit needContentUpdate();
}

void CompositionModelImpl::setLength(int width)
{
    timeT endMarker = m_grid.snapX(width);
    m_composition.setEndMarker(endMarker);
}

int CompositionModelImpl::getLength()
{
    timeT endMarker = m_composition.getEndMarker();
    int w = int(nearbyint(m_grid.getRulerScale()->getWidthForDuration(0, endMarker)));
    return w;
}

unsigned int CompositionModelImpl::getHeight()
{
    return (unsigned int)m_grid.getYBinCoordinate(getNbRows());
}

timeT CompositionModelImpl::getRepeatTimeAt(const QPoint& p, const CompositionItem& cItem)
{
    //     timeT timeAtClick = m_grid.getRulerScale()->getTimeForX(p.x());

    CompositionItemImpl* itemImpl = dynamic_cast<CompositionItemImpl*>((_CompositionItem*)cItem);

    const Segment* s = itemImpl->getSegment();

    timeT startTime = s->getStartTime();
    timeT endTime = s->getEndMarkerTime();
    timeT repeatInterval = endTime - startTime;

    int rWidth = int(nearbyint(m_grid.getRulerScale()->getXForTime(repeatInterval)));

    int count = (p.x() - int(itemImpl->rect().x())) / rWidth;
    RG_DEBUG << "CompositionModelImpl::getRepeatTimeAt() : count = " << count;

    return count != 0 ? startTime + (count * (s->getEndMarkerTime() - s->getStartTime())) : 0;
}

bool CompositionModelImpl::setTrackHeights(Segment *s)
{
    bool heightsChanged = false;

//    RG_DEBUG << "CompositionModelImpl::setTrackHeights";

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
//        RG_DEBUG << "CompositionModelImpl::setTrackHeights: heights have changed";
        for (Composition::segmentcontainer::iterator i = m_composition.begin();
             i != m_composition.end(); ++i) {
            computeSegmentRect(**i);
        }
    }

    return heightsChanged;
}

QPoint CompositionModelImpl::computeSegmentOrigin(const Segment& s)
{
    Profiler profiler("CompositionModelImpl::computeSegmentOrigin");

    int trackPosition = m_composition.getTrackPositionById(s.getTrack());
    timeT startTime = s.getStartTime();

    QPoint res;

    res.setX(int(nearbyint(m_grid.getRulerScale()->getXForTime(startTime))));

    res.setY(m_grid.getYBinCoordinate(trackPosition) +
             m_composition.getSegmentVoiceIndex(&s) *
             m_grid.getYSnap() + 1);

    return res;
}

bool CompositionModelImpl::isCachedRectCurrent(const Segment& s, const CompositionRect& r, QPoint cachedSegmentOrigin, timeT cachedSegmentEndTime)
{
    return s.isRepeating() == r.isRepeating() &&
           ((cachedSegmentOrigin.x() != r.x() && s.getEndMarkerTime() != cachedSegmentEndTime) ||
            (cachedSegmentOrigin.x() == r.x() && s.getEndMarkerTime() == cachedSegmentEndTime));
}

void CompositionModelImpl::clearInCache(const Segment* s, bool clearPreview)
{
    if (s) {
        m_segmentRectMap.erase(s);
        m_segmentEndTimeMap.erase(s);
        if (clearPreview)
            removePreviewCache(s);
    } else { // clear the whole cache
        m_segmentRectMap.clear();
        m_segmentEndTimeMap.clear();
        if (clearPreview)
            clearPreviewCache();
    }
}

void CompositionModelImpl::putInCache(const Segment*s, const CompositionRect& cr)
{
    m_segmentRectMap[s] = cr;
    m_segmentEndTimeMap[s] = s->getEndMarkerTime();
}

CompositionRect CompositionModelImpl::computeSegmentRect(const Segment& s, bool /*computeZ*/)
{
    Profiler profiler("CompositionModelImpl::computeSegmentRect");

    QPoint origin = computeSegmentOrigin(s);

    bool isRecordingSegment = isRecording(&s);

    if (!isRecordingSegment) {
        timeT endTime = 0;

        CompositionRect cachedCR = getFromCache(&s, endTime);
        // don't cache repeating segments - it's just hopeless, because the segment's rect may have to be recomputed
        // in other cases than just when the segment itself is moved,
        // for instance if another segment is moved over it
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
                //                 CompositionRect::repeatmarks repeatMarks = cachedCR.getRepeatMarks();
                //                 for(size_t i = 0; i < repeatMarks.size(); ++i) {
                //                     repeatMarks[i] += deltaX;
                //                 }
                //                 cachedCR.setRepeatMarks(repeatMarks);
                computeRepeatMarks(cachedCR, &s);
            }
            putInCache(&s, cachedCR);
            return cachedCR;
        }
    }

    timeT startTime = s.getStartTime();
    timeT endTime = isRecordingSegment ? m_pointerTimePos /*s.getEndTime()*/ : s.getEndMarkerTime();


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

    CompositionRect cr(origin, QSize(w, h));
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

    putInCache(&s, cr);

    return cr;
}

unsigned int CompositionModelImpl::computeZForSegment(const Rosegarden::Segment* s)
{
    return m_segmentOrderer.getZForSegment(s);
}

const CompositionRect& CompositionModelImpl::getFromCache(const Rosegarden::Segment* s, timeT& endTime)
{
    endTime = m_segmentEndTimeMap[s];
    return m_segmentRectMap[s];
}

unsigned int CompositionModelImpl::getNbRows()
{
    return m_composition.getNbTracks();
}

const CompositionModel::rectcontainer& CompositionModelImpl::getRectanglesIn(const QRect& rect,
        RectRanges* npData,
        AudioPreviewDrawData* apData)
{
    Profiler profiler("CompositionModelImpl::getRectanglesIn");

    m_res.clear();

    //RG_DEBUG << "CompositionModelImpl::getRectanglesIn: ruler scale is "
    //         << (dynamic_cast<SimpleRulerScale *>(m_grid.getRulerScale()))->getUnitsPerPixel();

    const Composition::segmentcontainer& segments = m_composition.getSegments();
    Composition::segmentcontainer::const_iterator segEnd = segments.end();

    for (Composition::segmentcontainer::const_iterator i = segments.begin();
         i != segEnd; ++i) {

        //RG_DEBUG << "CompositionModelImpl::getRectanglesIn: Composition contains segment " << *i << " (" << (*i)->getStartTime() << "->" << (*i)->getEndTime() << ")";
        
        const Segment* s = *i;

        if (isMoving(s))
            continue;

        CompositionRect sr = computeSegmentRect(*s);
        //RG_DEBUG << "CompositionModelImpl::getRectanglesIn: seg rect = " << sr;

        if (sr.intersects(rect)) {
            bool tmpSelected = isTmpSelected(s),
                 pTmpSelected = wasTmpSelected(s);

//            RG_DEBUG << "CompositionModelImpl::getRectanglesIn: segment " << s 
//                     << " selected : " << isSelected(s) << " - tmpSelected : " << isTmpSelected(s);
                       
            if (isSelected(s) || isTmpSelected(s) || sr.intersects(m_selectionRect)) {
                sr.setSelected(true);
            }

            if (pTmpSelected != tmpSelected)
                sr.setNeedsFullUpdate(true);

            bool isAudio = (s && s->getType() == Segment::Audio);

            if (!isRecording(s)) {
                QColor brushColor = GUIPalette::convertColour(m_composition.
                                    getSegmentColourMap().getColourByIndex(s->getColourIndex()));
                Qt::BrushStyle brushPattern =
                    s->isTrulyLinked() ? Qt::Dense2Pattern : Qt::SolidPattern;
                sr.setBrush(QBrush(brushColor, brushPattern));
                sr.setPen(CompositionColourCache::getInstance()->SegmentBorder);
            } else {
                // border is the same for both audio and MIDI
                sr.setPen(CompositionColourCache::getInstance()->RecordingSegmentBorder);
                // audio color
                if (isAudio) {
                    sr.setBrush(CompositionColourCache::getInstance()->RecordingAudioSegmentBlock);
                    // MIDI/default color
                } else {
                    sr.setBrush(CompositionColourCache::getInstance()->RecordingInternalSegmentBlock);
                }
            }

            // Notation preview data
            if (npData && s->getType() == Segment::Internal) {
                makeNotationPreviewRects(npData, QPoint(0, sr.y()), s, rect);
                // Audio preview data
            } else if (apData && s->getType() == Segment::Audio) {
                makeAudioPreviewRects(apData, s, sr, rect);
            }

            m_res.push_back(sr);
        } else {
            //RG_DEBUG << "CompositionModelImpl::getRectanglesIn: - segment out of rect";
        }

    }

    // changing items

    itemcontainer::iterator movEnd = m_changingItems.end();
    for (itemcontainer::iterator i = m_changingItems.begin(); i != movEnd; ++i) {
        CompositionRect sr((*i)->rect());
        if (sr.intersects(rect)) {
            Segment* s = CompositionItemHelper::getSegment(*i);
            sr.setSelected(true);
            QColor brushColor = GUIPalette::convertColour(m_composition.getSegmentColourMap().getColourByIndex(s->getColourIndex()));
            sr.setBrush(brushColor);

            sr.setPen(CompositionColourCache::getInstance()->SegmentBorder);

            // Notation preview data
            if (npData && s->getType() == Segment::Internal) {
                makeNotationPreviewRectsMovingSegment(npData, sr.topLeft(), s, sr);
                // Audio preview data
            } else if (apData && s->getType() == Segment::Audio) {
                makeAudioPreviewRects(apData, s, sr, rect);
            }

            m_res.push_back(sr);
        }
    }

    return m_res;
}

CompositionModel::heightlist CompositionModelImpl::getTrackDividersIn(const QRect& rect)
{
    int top = m_grid.getYBin(rect.y());
    int bottom = m_grid.getYBin(rect.y() + rect.height());

//    RG_DEBUG << "CompositionModelImpl::getTrackDividersIn: rect "
//              << rect.x() << ", " << rect.y() << ", "
//              << rect.width() << "x" << rect.height() << ", top = " << top
//              << ", bottom = " << bottom;
    
    setTrackHeights();
    
    CompositionModel::heightlist list;

    for (int pos = top; pos <= bottom; ++pos) {
        int divider = m_grid.getYBinCoordinate(pos);
        list.push_back(divider);
//        RG_DEBUG << "divider at " << divider;
    }

    return list;
}

CompositionModel::rectlist* CompositionModelImpl::getNotationPreviewData(const Segment* s)
{
    rectlist* npData = m_notationPreviewDataCache[s];

    if (!npData) {
        npData = makeNotationPreviewDataCache(s);
    }

    return npData;
}

CompositionModel::AudioPreviewData* CompositionModelImpl::getAudioPreviewData(const Segment* s)
{
    Profiler profiler("CompositionModelImpl::getAudioPreviewData");
    RG_DEBUG << "CompositionModelImpl::getAudioPreviewData";

    AudioPreviewData* apData = m_audioPreviewDataCache[s];

    if (!apData) {
        apData = makeAudioPreviewDataCache(s);
    }

    RG_DEBUG << "CompositionModelImpl::getAudioPreviewData returning";
    return apData;
}

CompositionModel::rectlist* CompositionModelImpl::makeNotationPreviewDataCache(const Segment *s)
{
    rectlist* npData = new rectlist();
    updatePreviewCacheForNotationSegment(s, npData);
    m_notationPreviewDataCache[s] = npData;
    return npData;
}

CompositionModel::AudioPreviewData* CompositionModelImpl::makeAudioPreviewDataCache(const Segment *s)
{
    RG_DEBUG << "CompositionModelImpl::makeAudioPreviewDataCache(" << s << ")";

    AudioPreviewData* apData = new AudioPreviewData(false, 0); // 0 channels -> empty
    updatePreviewCacheForAudioSegment(s, apData);
    m_audioPreviewDataCache[s] = apData;
    return apData;
}

}
#include "CompositionModelImpl.moc"
