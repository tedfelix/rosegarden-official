// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <cmath>
#include <qpainter.h>

#include <kmessagebox.h>

#include "Profiler.h"

#include "AudioLevel.h"
#include "BaseProperties.h"
#include "SnapGrid.h"
#include "Composition.h"
#include "RulerScale.h"

#include "audiopreviewupdater.h"
#include "compositioncolourcache.h"
#include "compositionview.h"
#include "compositionitemhelper.h"
#include "colours.h"
#include "rosegardenguidoc.h"
#include "rosegardencanvasview.h" // for NoFollow,FollowVertical,FollowHorizontal constants
#include "rosestrings.h"

using Rosegarden::SnapGrid;
using Rosegarden::Segment;
using Rosegarden::Composition;
using Rosegarden::timeT;
using Rosegarden::GUIPalette;

const QColor CompositionRect::DefaultPenColor = Qt::black;
const QColor CompositionRect::DefaultBrushColor = QColor(COLOUR_DEF_R, COLOUR_DEF_G, COLOUR_DEF_B);


timeT CompositionItemHelper::getStartTime(const CompositionItem& item, const Rosegarden::SnapGrid& grid)
{
    timeT t = 0;

    if (item) {
 	// t = std::max(grid.snapX(item->rect().x()) - 1, 0L); - wrong, we can have negative start times,
        // and if we do this we 'crop' segments when they are moved before the start of the composition
	t = grid.snapX(item->rect().x()) - 1;

        RG_DEBUG << "CompositionItemHelper::getStartTime(): item is repeating : " << item->isRepeating()
                 << " - startTime = " << t
                 << endl;
    }

    return t;
}

timeT CompositionItemHelper::getEndTime(const CompositionItem& item, const Rosegarden::SnapGrid& grid)
{
    timeT t = 0;

    if (item) {
        QRect itemRect = item->rect();
        
        t = std::max(grid.snapX(itemRect.x() + itemRect.width()) - 1, 0L);

        RG_DEBUG << "CompositionItemHelper::getEndTime() : rect width = "
                 << itemRect.width()
                 << " - item is repeating : " << item->isRepeating()
                 << " - endTime = " << t
                 << endl;

    }

    return t;
}

void CompositionItemHelper::setStartTime(CompositionItem& item, timeT time,
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
            CompositionRect& sr = dynamic_cast<CompositionItemImpl*>((_CompositionItem*)item)->getCompRect();
            int curW = sr.getBaseWidth();
            sr.setBaseWidth(curW + deltaX);
        }
        
    }
    
}

void CompositionItemHelper::setEndTime(CompositionItem& item, timeT time,
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
            CompositionRect& sr = dynamic_cast<CompositionItemImpl*>((_CompositionItem*)item)->getCompRect();
            sr.setBaseWidth(r.width());
        }
    }
}

int CompositionItemHelper::getTrackPos(const CompositionItem& item, const Rosegarden::SnapGrid& grid)
{
    return grid.getYBin(item->rect().y());
}

Rosegarden::Segment* CompositionItemHelper::getSegment(CompositionItem item)
{
    return (dynamic_cast<CompositionItemImpl*>((_CompositionItem*)item))->getSegment();
}

CompositionItem CompositionItemHelper::makeCompositionItem(Rosegarden::Segment* segment)
{
    return CompositionItem(new CompositionItemImpl(*segment, QRect()));
}

CompositionItem CompositionItemHelper::findSiblingCompositionItem(const CompositionModel::itemcontainer& items,
                                                                  const CompositionItem& referenceItem)
{
    CompositionModel::itemcontainer::const_iterator it;
    Rosegarden::Segment* currentSegment = CompositionItemHelper::getSegment(referenceItem);

    for (it = items.begin(); it != items.end(); it++) {
        CompositionItem item = *it;
        Rosegarden::Segment* segment = CompositionItemHelper::getSegment(item);
        if (segment == currentSegment) {
            return item;
        }
    }

    return referenceItem;
}

bool operator<(const CompositionRect& a, const CompositionRect& b)
{
    return a.width() < b.width();
}

bool operator<(const CompositionItem& a, const CompositionItem& b)
{
    return a->rect().width() < b->rect().width();
}


//
// CompositionModelImpl
//
CompositionModelImpl::CompositionModelImpl(Composition& compo,
                                           Rosegarden::Studio& studio,                                           
                                           Rosegarden::RulerScale *rulerScale,
                                           int vStep)
    : m_composition(compo),
      m_studio(studio),
      m_grid(rulerScale, vStep),
      m_audioPreviewThread(0)
{
    m_notationPreviewDataCache.setAutoDelete(true);
    m_audioPreviewDataCache.setAutoDelete(true);
    m_composition.addObserver(this);

    const Composition::segmentcontainer& segments = m_composition.getSegments();
    Composition::segmentcontainer::iterator segEnd = segments.end();

    for(Composition::segmentcontainer::iterator i = segments.begin();
        i != segEnd; ++i) {

        (*i)->addObserver(this);
    }

}

CompositionModelImpl::~CompositionModelImpl()
{
    if (!isCompositionDeleted()) {

        m_composition.removeObserver(this);

        const Composition::segmentcontainer& segments = m_composition.getSegments();
        Composition::segmentcontainer::iterator segEnd = segments.end();

        for(Composition::segmentcontainer::iterator i = segments.begin();
            i != segEnd; ++i) {

            (*i)->removeObserver(this);
        }
    }

    while (!m_audioPreviewUpdaters.empty()) {
	// Cause any running previews to be cancelled
	delete *(m_audioPreviewUpdaters.begin());
	m_audioPreviewUpdaters.erase(*m_audioPreviewUpdaters.begin());
    }
}

unsigned int CompositionModelImpl::getNbRows()
{
    return m_composition.getNbTracks();
}

const CompositionModel::rectcontainer& CompositionModelImpl::getRectanglesIn(const QRect& rect,
                                                                             PRectRanges* npData,
                                                                             previewrectlist* apData)
{
    Rosegarden::Profiler profiler("CompositionModelImpl::getRectanglesIn", true);

    m_res.clear();

//     RG_DEBUG << "CompositionModelImpl::getRectanglesIn: ruler scale is "
// 	     << (dynamic_cast<Rosegarden::SimpleRulerScale *>(m_grid.getRulerScale()))->getUnitsPerPixel() << endl;

    const Composition::segmentcontainer& segments = m_composition.getSegments();
    Composition::segmentcontainer::iterator segEnd = segments.end();

    for (Composition::segmentcontainer::iterator i = segments.begin();
        i != segEnd; ++i) {

// 	RG_DEBUG << "CompositionModelImpl::getRectanglesIn: Composition contains segment " << *i << " (" << (*i)->getStartTime() << "->" << (*i)->getEndTime() << ")"<<  endl;
	
        Segment* s = *i;
        if (isMoving(s))
            continue;
        
        CompositionRect sr = computeSegmentRect(*s);
        if (sr.intersects(rect)) {
            bool tmpSelected = isTmpSelected(s),
                pTmpSelected = wasTmpSelected(s);

            if (isSelected(s) || isTmpSelected(s) || sr.intersects(m_selectionRect)) {
                sr.setSelected(true);
            }
            
            if (pTmpSelected != tmpSelected)
                sr.setNeedsFullUpdate(true);

	    bool isAudio = (s && s->getType() == Rosegarden::Segment::Audio);

            if (!isRecording(s)) {
		QColor brushColor = GUIPalette::convertColour(m_composition.
			getSegmentColourMap().getColourByIndex(s->getColourIndex()));
                sr.setBrush(brushColor);
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
            if (npData && s->getType() == Rosegarden::Segment::Internal) {
                makeNotationPreviewRects(npData, sr.topLeft(), s, rect);                
            // Audio preview data
            } else if (apData && s->getType() == Rosegarden::Segment::Audio) {
                makeAudioPreviewRects(apData, sr.topLeft(), s, rect);
            }
            
            m_res.push_back(sr);
        } else {
//             RG_DEBUG << "CompositionModelImpl::getRectanglesIn: - segment out of rect\n";
        }
        
    }

    // moving items

    itemcontainer::iterator movEnd = m_movingItems.end();
    for(itemcontainer::iterator i = m_movingItems.begin(); i != movEnd; ++i) {
        CompositionRect sr((*i)->rect());
        if (sr.intersects(rect)) {
            Segment* s = CompositionItemHelper::getSegment(*i);
            sr.setSelected(true);
            QColor brushColor = GUIPalette::convertColour(m_composition.getSegmentColourMap().getColourByIndex(s->getColourIndex()));
            sr.setBrush(brushColor);

            sr.setPen(CompositionColourCache::getInstance()->SegmentBorder);
            
            m_res.push_back(sr);
        }
    }

    return m_res;
}

void CompositionModelImpl::makeNotationPreviewRects(PRectRanges* npRects, QPoint basePoint,
                                                    const Segment* segment, const QRect& clipRect)
{
    NotationPreviewData* cachedNPData = getNotationPreviewData(segment);

    NotationPreviewData::iterator npi = cachedNPData->lower_bound(clipRect),
        npEnd = cachedNPData->end();
    if (npi != cachedNPData->begin())
        --npi;

    PRectRange interval;
    
    interval.range.first = npi;

    int segEndX = int(nearbyint(m_grid.getRulerScale()->getXForTime(segment->getEndMarkerTime())));
    int xLim = std::min(clipRect.topRight().x(), segEndX);

    // move iterator forward
    //
    while (npi->x() < xLim && npi != npEnd) ++npi;

    interval.range.second = npi;
    interval.basePoint = QPoint(0, basePoint.y());
    npRects->push_back(interval);
}

void CompositionModelImpl::makeAudioPreviewRects(previewrectlist* apRects, QPoint basePoint,
                                                 const Segment* segment, const QRect& clipRect)
{
    AudioPreviewData* apData = getAudioPreviewData(segment);
    const std::vector<float>& values = apData->getValues();

    if (values.size() == 0)
        return;

    std::vector<float>::const_iterator npi = std::lower_bound(values.begin(), values.end(), clipRect.x()),
        npEnd = values.end();
    if (npi != values.begin())
        --npi;

    int segEndX = int(nearbyint(m_grid.getRulerScale()->getXForTime(segment->getEndMarkerTime())));
    int xLim = std::min(clipRect.topRight().x(), segEndX);

    const int height = m_grid.getYSnap()/2 - 2;
    const int halfRectHeight = m_grid.getYSnap()/2;

    //    RG_DEBUG << "CompositionModelImpl::makeAudioPreviewRects() : halfRectHeight = " << halfRectHeight << endl;

    float gain[2] = { 1.0, 1.0 };
    Rosegarden::TrackId trackId = segment->getTrack();
    Rosegarden::Track *track = getComposition().getTrackById(trackId);
    if (track) {
        Rosegarden::Instrument *instrument = getStudio().getInstrumentById(track->getInstrument());
        if (instrument) {
            float level = Rosegarden::AudioLevel::dB_to_multiplier(instrument->getLevel());
            float pan = instrument->getPan() - 100.0;
            gain[0] = level * ((pan > 0.0) ? (1.0 - (pan / 100.0)) : 1.0);
            gain[1] = level * ((pan < 0.0) ? ((pan + 100.0) / 100.0) : 1.0);
        }

    }

    bool showMinima = apData->showsMinima();
    unsigned int channels = apData->getChannels();
    if (channels == 0) {
        RG_DEBUG << "CompositionModelImpl::makeAudioPreviewRects() : problem with audio file for segment "
                 << segment->getLabel().c_str() << endl;
        return;
    }

    int samplePoints = values.size() / (channels * (showMinima ? 2 : 1));
    float h1, h2, l1 = 0, l2 = 0;

    while((*npi) <= xLim && npi != npEnd) {

        if (channels == 1) {

            h1 = *npi; ++npi;
            h2 = h1;

	    h1 *= gain[0];
	    h2 *= gain[1];

            if (apData->showsMinima()) {
                l1 = *npi; ++npi;
                l2 = l1;

		l1 *= gain[0];
		l2 *= gain[1];
            }
        } else {

            h1 = *npi * gain[0]; ++npi;
            if (showMinima) l1 = *npi * gain[0]; ++npi;

            h2 = *npi * gain[1]; ++npi;
            if (showMinima) l2 = *npi * gain[1]; ++npi;
            
        }

        int width = 1;
        const QColor defaultCol = CompositionColourCache::getInstance()->SegmentAudioPreview;

        QColor color;
	int baseY = halfRectHeight + basePoint.y();

	// h1 left, h2 right

	if (h1 >= 1.0) { h1 = 1.0; color = Qt::red; }
	else { color = defaultCol; }

	int h = Rosegarden::AudioLevel::multiplier_to_preview(h1, height);
	if (h < 0) h = 0;

        int i = npi - values.begin();
        
        PreviewRect r(i, baseY - h, width, h);
        r.setColor(color);

        apRects->push_back(r);

	if (h2 >= 1.0) { h2 = 1.0; color = Qt::red; }
	else { color = defaultCol; }

	h = Rosegarden::AudioLevel::multiplier_to_preview(h2, height);
	if (h < 0) h = 0;

        PreviewRect r2(i, baseY, width, h);
        r2.setColor(color);

        apRects->push_back(r2);
        
    }
    
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
        timeT repeatStart = endTime;
        timeT repeatEnd = s->getRepeatEndTime();
        sr.setWidth(int(nearbyint(m_grid.getRulerScale()->getWidthForDuration(startTime,
                                                                              repeatEnd - startTime))));

        CompositionRect::repeatmarks repeatMarks;

//         RG_DEBUG << "CompositionModelImpl::computeRepeatMarks : repeatStart = "
//                  << repeatStart << " - repeatEnd = " << repeatEnd << endl;

        for(timeT repeatMark = repeatStart; repeatMark < repeatEnd; repeatMark += repeatInterval) {
            int mark = int(nearbyint(m_grid.getRulerScale()->getXForTime(repeatMark)));
//             RG_DEBUG << "CompositionModelImpl::computeRepeatMarks : mark at " << mark << endl;
            repeatMarks.push_back(mark);
        }
        sr.setRepeatMarks(repeatMarks);
        if (repeatMarks.size() > 0)
            sr.setBaseWidth(repeatMarks[0] - sr.x());
        else {
//             RG_DEBUG << "CompositionModelImpl::computeRepeatMarks : no repeat marks\n";
            sr.setBaseWidth(sr.width());
        }

//         RG_DEBUG << "CompositionModelImpl::computeRepeatMarks : s = "
//                  << s << " base width = " << sr.getBaseWidth()
//                  << " - nb repeat marks = " << repeatMarks.size() << endl;

    }
}

void CompositionModelImpl::setAudioPreviewThread(AudioPreviewThread& thread)
{
    RG_DEBUG << "CompositionModelImpl::setAudioPreviewThread()\n";
    m_audioPreviewThread = &thread;
}


void CompositionModelImpl::refreshAllPreviews()
{
    RG_DEBUG << "CompositionModelImpl::refreshAllPreviews\n";

    while (!m_audioPreviewUpdaters.empty()) {
	// Cause any running previews to be cancelled
	delete *(m_audioPreviewUpdaters.begin());
	m_audioPreviewUpdaters.erase(*m_audioPreviewUpdaters.begin());
    }

    clearPreviewCache();

    const Composition::segmentcontainer& segments = m_composition.getSegments();
    Composition::segmentcontainer::iterator segEnd = segments.end();

    for (Composition::segmentcontainer::iterator i = segments.begin();
        i != segEnd; ++i) {

        makePreviewCache(*i);
    }
    
}

void CompositionModelImpl::clearPreviewCache()
{
    RG_DEBUG << "CompositionModelImpl::clearPreviewCache\n";

    m_notationPreviewDataCache.clear();
    m_audioPreviewDataCache.clear();
}

void CompositionModelImpl::updatePreviewCacheForNotationSegment(const Segment* segment, NotationPreviewData* npData)
{
    npData->clear();
    
    int segStartX = int(nearbyint(m_grid.getRulerScale()->getXForTime(segment->getStartTime())));
//     int segEndX = int(nearbyint(m_grid.getRulerScale()->getXForTime(segment->getEndMarkerTime())));
    QColor segColor = computeSegmentNotationPreviewColor(segment);

    for (Segment::iterator i = segment->begin();
	 segment->isBeforeEndMarker(i); ++i) {

        long pitch = 0;
        if (!(*i)->isa(Rosegarden::Note::EventType) ||
            !(*i)->get<Rosegarden::Int>
            (Rosegarden::BaseProperties::PITCH, pitch)) {
            continue;
        }

	timeT eventStart = (*i)->getAbsoluteTime();
	timeT eventEnd = eventStart + (*i)->getDuration();
	if (eventEnd > segment->getEndMarkerTime()) {
	    eventEnd = segment->getEndMarkerTime();
	}

        int x = int(nearbyint(m_grid.getRulerScale()->getXForTime(eventStart)));
        int width = int(nearbyint(m_grid.getRulerScale()->getWidthForDuration(eventStart,
                                                                              eventEnd - eventStart)));

	if (x <= segStartX) { ++x; if (width > 1) --width; }
	if (width > 1) --width;
	if (width < 1) ++width;

        double y0 = 0;
        double y1 = m_grid.getYSnap();
        double y = y1 + ((y0 - y1) * (pitch-16)) / 96;
        if (y < y0) y = y0;
        if (y > y1-1) y = y1-1;

        PreviewRect r(x, (int)y, width, 2);
        r.setColor(segColor);

//         RG_DEBUG << "CompositionModelImpl::updatePreviewCacheForNotationSegment() : npData = "
//                  << npData << ", preview rect = "
//                  << r << endl;
        npData->insert(r);
    }

}

QColor CompositionModelImpl::computeSegmentNotationPreviewColor(const Segment* segment) 
{
    // compute the preview color so it's as visible as possible over the segment's color
    QColor segColor = GUIPalette::convertColour(m_composition.getSegmentColourMap().getColourByIndex(segment->getColourIndex()));
    int h, s, v;
    segColor.hsv(&h, &s, &v);
  
    // colors with saturation lower than value should be pastel tints, and
    // they get a value of 0; yellow and green hues close to the dead center
    // point for that hue were taking a value of 255 with the (s < v)
    // formula, so I added an extra hack to force hues in those two narrow
    // ranges toward black.  Black always looks good, while white washes out
    // badly against intense yellow, and doesn't look very good against
    // intense green either...  hacky, but this produces pleasant results against
    // every bizarre extreme of color I could cook up to throw at it, plus
    // (the real reason for all this convoluted fiddling, it does all that while keeping
    // white against bright reds and blues, which looks better than black)
    if ( ((((h > 57) && (h < 66)) || ((h > 93) && (h < 131))) && (s > 127) && (v > 127) ) ||
	 (s < v) ) {
	v = 0;
    } else {
	v = 255;
    }
    s = 31;
    h += 180;

    segColor.setHsv(h, s, v);

    return segColor;
}


void CompositionModelImpl::updatePreviewCacheForAudioSegment(const Segment* segment, AudioPreviewData* apData)
{
    if (m_audioPreviewThread) {
        RG_DEBUG << "CompositionModelImpl::updatePreviewCacheForAudioSegment() - new audio preview started\n";
        QRect segRect = computeSegmentRect(*segment);
        segRect.moveTopLeft(QPoint(0,0));
        AudioPreviewUpdater* updater = new AudioPreviewUpdater(*m_audioPreviewThread,
                                                               m_composition, segment, segRect,
							       this);

	apData->setSegmentRect(segRect);

        connect(updater, SIGNAL(audioPreviewComplete(AudioPreviewUpdater*)),
                this, SLOT(slotAudioPreviewComplete(AudioPreviewUpdater*)));

	m_audioPreviewUpdaters.insert(updater);
        updater->update();

    } else {
        RG_DEBUG << "CompositionModelImpl::updatePreviewCacheForAudioSegment() - no audio preview thread set\n";
    }
}

void CompositionModelImpl::slotAudioPreviewComplete(AudioPreviewUpdater* apu)
{
    RG_DEBUG << "CompositionModelImpl::slotAudioPreviewComplete()\n";
    
    AudioPreviewData *apData = getAudioPreviewData(apu->getSegment());
    if (apData) {
	RG_DEBUG << "CompositionModelImpl::slotAudioPreviewComplete(" << apu << "): apData contains " << apData->getValues().size() << " values already" << endl;
	unsigned int channels = 0;
	const std::vector<float> &values = apu->getComputedValues(channels);
	if (channels > 0) {
	    RG_DEBUG << "CompositionModelImpl::slotAudioPreviewComplete: set "
                     << values.size() << " samples on " << channels << " channels\n";
	    apData->setChannels(channels);
	    apData->setValues(values);
//             postProcessAudioPreview(apData, apu->getSegment());
	}
    }

    m_audioPreviewUpdaters.erase(apu);
    delete apu;

    emit needUpdate(computeSegmentRect(*(apu->getSegment())));
}


/*
 * NO LONGER USED
 */
void CompositionModelImpl::postProcessAudioPreview(AudioPreviewData* apData, const Segment* segment)
{
    const int height = m_grid.getYSnap()/2 - 2;
    const int halfRectHeight = m_grid.getYSnap()/2;

    //    RG_DEBUG << "CompositionModelImpl::makeAudioPreviewRects() : halfRectHeight = " << halfRectHeight << endl;

    float gain[2] = { 1.0, 1.0 };
    Rosegarden::TrackId trackId = segment->getTrack();
    Rosegarden::Track *track = getComposition().getTrackById(trackId);
    if (track) {
        Rosegarden::Instrument *instrument = getStudio().getInstrumentById(track->getInstrument());
        if (instrument) {
            float level = Rosegarden::AudioLevel::dB_to_multiplier(instrument->getLevel());
            float pan = instrument->getPan() - 100.0;
            gain[0] = level * ((pan > 0.0) ? (1.0 - (pan / 100.0)) : 1.0);
            gain[1] = level * ((pan < 0.0) ? ((pan + 100.0) / 100.0) : 1.0);
        }

    }

    bool showMinima = apData->showsMinima();
    unsigned int channels = apData->getChannels();
    const std::vector<float>& values = apData->getValues();

    if (values.size() == 0)
        return;

    if (channels == 0) {
        RG_DEBUG << "CompositionModelImpl::makeAudioPreviewRects() : problem with audio file for segment "
                 << segment->getLabel().c_str() << endl;
        return;
    } else {
        //	RG_DEBUG << "CompositionModelImpl::makeAudioPreviewRects: Have "
        //		 << channels << " channels, " << values.size()
        //		 << " samples for audio preview" << endl;
    }
    
    int samplePoints = values.size() / (channels * (showMinima ? 2 : 1));
    float h1, h2, l1 = 0, l2 = 0;

    QRect tRect = apData->getSegmentRect();
    QPoint currentSegmentOrigin = computeSegmentOrigin(*segment);
    tRect.moveTopLeft(currentSegmentOrigin);

    double sampleScaleFactor = samplePoints / double(tRect.width());

//     int i0 = clipRect.x() - tRect.x();
//     i0 = std::max(i0, 0);

//     int i1 = i0 + clipRect.width();
//     i1 = std::min(i1, tRect.width());
    
    for (int i = 0; i < tRect.width(); ++i) {
        // For each i work get the sample starting point
        //
        int position = int(channels * i * sampleScaleFactor);
	if (position < 0) continue;
	if (position >= values.size() - channels) break;

        if (channels == 1) {

            h1 = values[position++];
            h2 = h1;

	    h1 *= gain[0];
	    h2 *= gain[1];

            if (apData->showsMinima()) {
                l1 = values[position++];
                l2 = l1;

		l1 *= gain[0];
		l2 *= gain[1];
            }
        } else {

            h1 = values[position++] * gain[0];
            if (showMinima) l1 = values[position++] * gain[0];

            h2 = values[position++] * gain[1];
            if (showMinima) l2 = values[position++] * gain[1];
            
        }

//        RG_DEBUG << "CompositionModelImpl::makeAudioPreviewRects() - h1 = " << h1
//                 << " - h2 : " << h2 << endl;

        int width = 1;
        const QColor defaultCol = CompositionColourCache::getInstance()->SegmentAudioPreview;

        QColor color;
	int baseY = halfRectHeight;

	// h1 left, h2 right

	if (h1 >= 1.0) { h1 = 1.0; color = Qt::red; }
	else { color = defaultCol; }

//	int h = int(h1 * height + 0.5);
	int h = Rosegarden::AudioLevel::multiplier_to_preview(h1, height);
	if (h < 0) h = 0;

        PreviewRect r(i, baseY - h, width, h);
        r.setColor(color);

//        RG_DEBUG << "CompositionModelImpl::makeAudioPreviewRects() - insert rect r1 "
//                 << r << " - height = " << height << endl;

        apData->addPreviewRect(r);

	if (h2 >= 1.0) { h2 = 1.0; color = Qt::red; }
	else { color = defaultCol; }

//        h = int(h2 * height + 0.5);
	h = Rosegarden::AudioLevel::multiplier_to_preview(h2, height);
	if (h < 0) h = 0;

        PreviewRect r2(i, baseY, width, h);
        r2.setColor(color);

        apData->addPreviewRect(r2);

//         RG_DEBUG << "CompositionModelImpl::makeAudioPreviewRects() - insert rect r " << r
//                  << " - r2 " << r2 << " - height = " << height << endl;
    }

    if (segment->isAutoFading()) {

        Composition &comp = getComposition();

        int audioFadeInEnd = int(
                                 m_grid.getRulerScale()->getXForTime(comp.
                                                                     getElapsedTimeForRealTime(segment->getFadeInTime()) +
                                                                     segment->getStartTime()) -
                                 m_grid.getRulerScale()->getXForTime(segment->getStartTime()));

        PreviewRect r3(0, tRect.height() - 1, audioFadeInEnd, 1);
        r3.setColor(Qt::blue);
        apData->addPreviewRect(r3);
    }

    RG_DEBUG << "CompositionModelImpl::postProcessAudioPreview: now have " << apData->getPreviewRects().size() << " preview rects in " << apData << endl;
    
}


CompositionModel::NotationPreviewData* CompositionModelImpl::getNotationPreviewData(const Rosegarden::Segment* s)
{
    NotationPreviewData* npData = m_notationPreviewDataCache[const_cast<Rosegarden::Segment*>(s)];
 
    if (!npData) {
        npData = makeNotationPreviewDataCache(s);
    }

    return npData;
}

CompositionModel::AudioPreviewData* CompositionModelImpl::getAudioPreviewData(const Rosegarden::Segment* s)
{
    AudioPreviewData* apData = m_audioPreviewDataCache[const_cast<Rosegarden::Segment*>(s)];

    if (!apData) {
        apData = makeAudioPreviewDataCache(s);
    }

    return apData;
}

void CompositionModelImpl::refreshDirtyPreviews()
{
//     RG_DEBUG << "CompositionModelImpl::refreshDirtyPreviews()\n";

    std::set<const Rosegarden::Segment*>::iterator i = m_dirtySegments.begin();
    std::set<const Rosegarden::Segment*>::iterator dirtySegmentsEnd = m_dirtySegments.end();
    
    for (;i != dirtySegmentsEnd; ++i) {
        const Segment* s = *i;

        if (s->getType() == Rosegarden::Segment::Audio) {
            AudioPreviewData* apData = getAudioPreviewData(s);
            RG_DEBUG << "CompositionModelImpl::refreshDirtyPreviews() - update audio preview cache for segment "
                     << s << endl;
            updatePreviewCacheForAudioSegment(s, apData);
        } else {
            NotationPreviewData* npData = getNotationPreviewData(s);
            updatePreviewCacheForNotationSegment(s, npData);
        }
    }
    clearDirtyPreviews();
}

void CompositionModelImpl::clearDirtyPreviews()
{
    m_dirtySegments.clear();
}
                                             
void CompositionModelImpl::eventAdded(const Rosegarden::Segment *s, Rosegarden::Event *)
{
//     RG_DEBUG << "CompositionModelImpl::eventAdded()\n";
    m_dirtySegments.insert(s);
}

void CompositionModelImpl::eventRemoved(const Rosegarden::Segment *s, Rosegarden::Event *)
{
    m_dirtySegments.insert(s);
}

void CompositionModelImpl::endMarkerTimeChanged(const Rosegarden::Segment *s, bool)
{
    clearInCache(s);
}


void CompositionModelImpl::makePreviewCache(Segment *s) 
{
    if (s->getType() == Rosegarden::Segment::Internal) {
        makeNotationPreviewDataCache(s);
    } else {
        makeAudioPreviewDataCache(s);
    }
}

void CompositionModelImpl::removePreviewCache(Segment *s) 
{
    if (s->getType() == Rosegarden::Segment::Internal) {
        m_notationPreviewDataCache.remove(s);
    } else {
        m_audioPreviewDataCache.remove(s);
    }

}

void CompositionModelImpl::segmentAdded(const Composition *, Segment *s) 
{
    makePreviewCache(s);
    s->addObserver(this);
}

void CompositionModelImpl::segmentRemoved(const Composition *, Segment *s)
{
    clearInCache(s);
    removePreviewCache(s);
    s->removeObserver(this);
}

void CompositionModelImpl::segmentRepeatChanged(const Composition *, Segment *s, bool)
{
    clearInCache(s);
}

CompositionModel::NotationPreviewData* CompositionModelImpl::makeNotationPreviewDataCache(const Segment *s)
{
    NotationPreviewData* npData = new NotationPreviewData();
    updatePreviewCacheForNotationSegment(s, npData);
    m_notationPreviewDataCache.insert(const_cast<Segment*>(s), npData);

    return npData;
}

CompositionModel::AudioPreviewData* CompositionModelImpl::makeAudioPreviewDataCache(const Segment *s)
{
    RG_DEBUG << "CompositionModelImpl::makeAudioPreviewDataCache(" << s << ")" << endl;

    AudioPreviewData* apData = new AudioPreviewData(false, 0); // 0 channels -> empty
    updatePreviewCacheForAudioSegment(s, apData);
    m_audioPreviewDataCache.insert(const_cast<Segment*>(s), apData);

    return apData;
}

void CompositionModelImpl::setSelectionRect(const QRect& r)
{
    m_selectionRect = r.normalize();

    m_previousTmpSelectedSegments = m_tmpSelectedSegments;
    m_tmpSelectedSegments.clear();
    
    const Composition::segmentcontainer& segments = m_composition.getSegments();
    Composition::segmentcontainer::iterator segEnd = segments.end();

    for(Composition::segmentcontainer::iterator i = segments.begin();
        i != segEnd; ++i) {

        Segment* s = *i;
        CompositionRect sr = computeSegmentRect(*s);
        if (sr.intersects(m_selectionRect)) {
            m_tmpSelectedSegments.insert(s);
        }
    }
}

void CompositionModelImpl::finalizeSelectionRect()
{
    const Composition::segmentcontainer& segments = m_composition.getSegments();
    Composition::segmentcontainer::iterator segEnd = segments.end();

    for(Composition::segmentcontainer::iterator i = segments.begin();
        i != segEnd; ++i) {

        Segment* s = *i;
        CompositionRect sr = computeSegmentRect(*s);
        if (sr.intersects(m_selectionRect)) {
            setSelected(s);
        }
    }

    m_selectionRect = QRect();
    m_tmpSelectedSegments.clear();
}

QRect CompositionModelImpl::getSelectionContentsRect()
{
    QRect selectionRect;

    Rosegarden::SegmentSelection sel = getSelectedSegments();
    for(Rosegarden::SegmentSelection::iterator i = sel.begin();
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

//     RG_DEBUG << "CompositionModelImpl::addRecordingItem: now have "
// 	     << m_recordingSegments.size() << " recording items\n";
}

void CompositionModelImpl::removeRecordingItem(const CompositionItem &item)
{
    m_recordingSegments.erase(CompositionItemHelper::getSegment(item));

//     RG_DEBUG << "CompositionModelImpl::removeRecordingItem: now have "
// 	     << m_recordingSegments.size() << " recording items\n";
}

void CompositionModelImpl::clearRecordingItems()
{
    m_recordingSegments.clear();
//     RG_DEBUG << "CompositionModelImpl::clearRecordingItem\n";
}

bool CompositionModelImpl::isMoving(const Segment* sm) const
{
    itemcontainer::const_iterator movEnd = m_movingItems.end();

    for(itemcontainer::const_iterator i = m_movingItems.begin(); i != movEnd; ++i) {
        const CompositionItemImpl* ci = dynamic_cast<const CompositionItemImpl*>((_CompositionItem*)(*i));
        const Segment* s = ci->getSegment();
        if (sm == s) return true;
    }

    return false;
}

bool CompositionModelImpl::isRecording(const Segment* s) const
{
    return m_recordingSegments.find(const_cast<Segment*>(s)) != m_recordingSegments.end();
}


CompositionModel::itemcontainer CompositionModelImpl::getItemsAt(const QPoint& point)
{
    itemcontainer res;

    const Composition::segmentcontainer& segments = m_composition.getSegments();

    for(Composition::segmentcontainer::iterator i = segments.begin();
        i != segments.end(); ++i) {

        Segment* s = *i;
        CompositionRect sr = computeSegmentRect(*s);
        if (sr.contains(point)) {
//             RG_DEBUG << "CompositionModelImpl::getItemsAt() adding " << sr << endl;
            res.insert(CompositionItem(new CompositionItemImpl(*s, sr)));
        } else {
//             RG_DEBUG << "CompositionModelImpl::getItemsAt() skiping " << sr << endl;
        }
        
    }

    return res;
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
    for(itemcontainer::const_iterator i = items.begin(); i != items.end(); ++i) {
        setSelected(*i);
    }
}

void CompositionModelImpl::setSelected(const Segment* segment, bool selected)
{
    if (selected) {
        if (!isSelected(segment))
            m_selectedSegments.insert(const_cast<Segment*>(segment));
    } else {
        Rosegarden::SegmentSelection::iterator i = m_selectedSegments.find(const_cast<Segment*>(segment));
        if (i != m_selectedSegments.end())
            m_selectedSegments.erase(i);
    }
}

void CompositionModelImpl::signalSelection()
{
    RG_DEBUG << "CompositionModelImpl::signalSelection()\n";
    emit selectedSegments(getSelectedSegments());
}

void CompositionModelImpl::clearSelected()
{
    m_selectedSegments.clear();
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

void CompositionModelImpl::startMove(const CompositionItem& item)
{
    itemcontainer::iterator i = m_movingItems.find(item);
    
    // if an "identical" composition item has already been inserted, drop this one
    if (i != m_movingItems.end()) {
        m_itemGC.push_back(item);
    } else {
        item->saveRect();
        m_movingItems.insert(item);
    }
}

void CompositionModelImpl::startMoveSelection()
{
    Rosegarden::SegmentSelection::iterator i = m_selectedSegments.begin();
    for(; i != m_selectedSegments.end(); ++i) {
        Segment* s = *i;
        CompositionRect sr = computeSegmentRect(*s);
        startMove(CompositionItem(new CompositionItemImpl(*s, sr)));
    }
    
}

void CompositionModelImpl::endMove()
{
    for(itemcontainer::const_iterator i = m_movingItems.begin(); i != m_movingItems.end(); ++i) {
        delete *i;
    }

    m_movingItems.clear();

    for(itemgc::iterator i = m_itemGC.begin(); i != m_itemGC.end(); ++i) {
        delete *i;
    }
    m_itemGC.clear();
    
}

void CompositionModelImpl::setLength(int width)
{
    Rosegarden::timeT endMarker = m_grid.snapX(width);
    m_composition.setEndMarker(endMarker);
}

int CompositionModelImpl::getLength()
{
    Rosegarden::timeT endMarker = m_composition.getEndMarker();
    int w = int(nearbyint(m_grid.getRulerScale()->getWidthForDuration(0, endMarker)));
    return w;
}

Rosegarden::timeT CompositionModelImpl::getRepeatTimeAt(const QPoint& p, const CompositionItem& cItem)
{
//     timeT timeAtClick = m_grid.getRulerScale()->getTimeForX(p.x());

    CompositionItemImpl* itemImpl = dynamic_cast<CompositionItemImpl*>((_CompositionItem*)cItem);

    const Segment* s = itemImpl->getSegment();

    timeT startTime = s->getStartTime();
    timeT endTime = s->getEndMarkerTime();
    timeT repeatInterval = endTime - startTime;

    int rWidth = int(nearbyint(m_grid.getRulerScale()->getXForTime(repeatInterval)));

    int count = (p.x() - int(itemImpl->rect().x())) / rWidth;
    RG_DEBUG << "CompositionModelImpl::getRepeatTimeAt() : count = " << count << endl;

    return count != 0 ? startTime + (count * (s->getEndMarkerTime() - s->getStartTime())) : 0;
}

QPoint CompositionModelImpl::computeSegmentOrigin(const Segment& s)
{
    Rosegarden::Profiler profiler("CompositionModelImpl::computeSegmentOrigin", true);

    int trackPosition = m_composition.getTrackById(s.getTrack())->getPosition();
    Rosegarden::timeT startTime = s.getStartTime();

    QPoint res;
    
    res.setX(int(nearbyint(m_grid.getRulerScale()->getXForTime(startTime))));
    res.setY(m_grid.getYBinCoordinate(trackPosition));

    return res;
}

bool CompositionModelImpl::isCachedRectCurrent(const Segment& s, const CompositionRect& r, QPoint cachedSegmentOrigin, timeT cachedSegmentEndTime) 
{
    return s.isRepeating() == r.isRepeating() &&
        ((cachedSegmentOrigin.x() != r.x() && s.getEndMarkerTime() != cachedSegmentEndTime) ||
         (cachedSegmentOrigin.x() == r.x() && s.getEndMarkerTime() == cachedSegmentEndTime));
}

void CompositionModelImpl::clearInCache(const Rosegarden::Segment* s)
{
    if (s) {
        m_segmentRectMap.erase(s);
        m_segmentEndTimeMap.erase(s);
    } else { // clear the whole cache
        m_segmentRectMap.clear();
        m_segmentEndTimeMap.clear();
    }
}

void CompositionModelImpl::putInCache(const Rosegarden::Segment*s, const CompositionRect& cr)
{
    m_segmentRectMap.insert(s, cr);
    m_segmentEndTimeMap.insert(s, s->getEndMarkerTime());
}

const CompositionRect& CompositionModelImpl::getFromCache(const Rosegarden::Segment* s, timeT& endTime)
{
    endTime = m_segmentEndTimeMap[s];
    return m_segmentRectMap[s];
}

CompositionRect CompositionModelImpl::computeSegmentRect(const Segment& s)
{
    Rosegarden::Profiler profiler("CompositionModelImpl::computeSegmentRect", true);

    QPoint origin = computeSegmentOrigin(s);

    bool isRecordingSegment = isRecording(&s);
    
    if (!isRecordingSegment) {
        timeT endTime = 0;
        
        CompositionRect cachedCR = getFromCache(&s, endTime);
        // don't cache repeating segments - it's just hopeless, because the segment's rect may have to be recomputed
        // in other cases than just when the segment itself is moved,
        // for instance if another segment is moved over it
        if (!s.isRepeating() && cachedCR.isValid() && isCachedRectCurrent(s, cachedCR, origin, endTime)) {
//         RG_DEBUG << "CompositionModelImpl::computeSegmentRect() : using cache for seg "
//                  << &s << " - cached rect repeating = " << cachedCR.isRepeating() << " - base width = "
//                  << cachedCR.getBaseWidth() << endl;

            bool xChanged = origin.x() != cachedCR.x();
            bool yChanged = origin.y() != cachedCR.y();

            cachedCR.moveTopLeft(origin);

            if (s.isRepeating() && (xChanged || yChanged)) { // update repeat marks

                // this doesn't work in the general case (if there's another segment on the same track for instance),
                // it's better to simply recompute all the marks
//                 CompositionRect::repeatmarks repeatMarks = cachedCR.getRepeatMarks();
//                 for(unsigned int i = 0; i < repeatMarks.size(); ++i) {
//                     repeatMarks[i] += deltaX;
//                 }
//                 cachedCR.setRepeatMarks(repeatMarks);
                computeRepeatMarks(cachedCR, &s);
            }
            putInCache(&s, cachedCR);
            return cachedCR;
        }
    }
    
    Rosegarden::timeT startTime = s.getStartTime();
    Rosegarden::timeT endTime   = isRecordingSegment ? s.getEndTime() : s.getEndMarkerTime();


    int h = m_grid.getYSnap();
    int w;

//     RG_DEBUG << "CompositionModelImpl::computeSegmentRect: x " << origin.x() << ", y " << origin.y() << " startTime " << startTime << ", endTime " << endTime << endl;

    if (s.isRepeating()) {
        timeT repeatStart = endTime;
        timeT repeatEnd   = s.getRepeatEndTime();
        w = int(nearbyint(m_grid.getRulerScale()->getWidthForDuration(startTime,
                                                                      repeatEnd - startTime)));
//         RG_DEBUG << "CompositionModelImpl::computeSegmentRect : s is repeating - repeatStart = "
//                  << repeatStart << " - repeatEnd : " << repeatEnd
//                  << " w = " << w << endl;
    } else {
        w = int(nearbyint(m_grid.getRulerScale()->getWidthForDuration(startTime, endTime - startTime)));
//          RG_DEBUG << "CompositionModelImpl::computeSegmentRect : s is NOT repeating"
//                   << " w = " << w << " (x for time at start is " << m_grid.getRulerScale()->getXForTime(startTime) << ", end is " << m_grid.getRulerScale()->getXForTime(endTime) << ")" << endl;
    }

    CompositionRect cr(origin, QSize(w, h));
    QString label = strtoqstr(s.getLabel());
    if (s.getType() == Rosegarden::Segment::Audio) {
	static QRegExp re1("( *\\([^)]*\\))*$"); // (inserted) (copied) (etc)
	static QRegExp re2("\\.[^.]+$"); // filename suffix
	label.replace(re1, "").replace(re2, "");
    }
    cr.setLabel(label);

    if (s.isRepeating())
        computeRepeatMarks(cr, &s);

    putInCache(&s, cr);

    return cr;
}

//
// CompositionItemImpl
//
CompositionItemImpl::CompositionItemImpl(Segment& s, const CompositionRect& rect)
    : m_segment(s),
      m_rect(rect)
{
}

QRect CompositionItemImpl::rect() const
{
    QRect res = m_rect;
    if (m_rect.isRepeating()) {
        CompositionRect::repeatmarks repeatMarks = m_rect.getRepeatMarks();
        int neww = m_rect.getBaseWidth();
        
//         RG_DEBUG << "CompositionItemImpl::rect() -  width = "
//                  << m_rect.width() << " - base w = " << neww << endl;
        res.setWidth(neww);
    } else {
//         RG_DEBUG << "CompositionItemImpl::rect() m_rect not repeating\n";
    }
    

    return res;
}


//
// CompositionView
//
CompositionView::CompositionView(RosegardenGUIDoc* doc,
                                 CompositionModel* model,
                                 QWidget * parent, const char * name, WFlags f)
#if KDE_VERSION >= KDE_MAKE_VERSION(3,2,0)
    : RosegardenScrollView(parent, name, f | WNoAutoErase|WStaticContents),
#else
    : RosegardenScrollView(parent, name, f | WRepaintNoErase|WResizeNoErase|WStaticContents),
#endif
      m_model(model),
      m_currentItem(0),
      m_tool(0),
      m_toolBox(0),
      m_showPreviews(false),
      m_showSegmentLabels(true),
      m_fineGrain(false),
      m_minWidth(m_model->getLength()),
      m_stepSize(0),
      m_rectFill(0xF0, 0xF0, 0xF0),
      m_selectedRectFill(0x00, 0x00, 0xF0),
      m_pointerPos(0),
      m_pointerColor(GUIPalette::getColour(GUIPalette::Pointer)),
      m_pointerWidth(4),
      m_pointerPen(QPen(m_pointerColor, m_pointerWidth)),
      m_tmpRect(QRect(QPoint(0,0),QPoint(-1,-1))),
      m_drawGuides(false),
      m_guideColor(GUIPalette::getColour(GUIPalette::MovementGuide)),
      m_topGuidePos(0),
      m_foreGuidePos(0),
      m_drawSelectionRect(false),
      m_drawTextFloat(false),
      m_drawBuffer(visibleWidth(), visibleHeight()),
      m_drawBufferNeedsRefresh(true)
{
    m_toolBox = new SegmentToolBox(this, doc);

    setDragAutoScroll(true);
    setBackgroundMode(NoBackground);
    viewport()->setBackgroundMode(NoBackground);
    viewport()->setPaletteBackgroundColor(GUIPalette::getColour(GUIPalette::SegmentCanvas));

    updateSize();
    
    QScrollBar* hsb = horizontalScrollBar();

    // dynamically adjust content size when scrolling past current composition's end
    connect(hsb, SIGNAL(nextLine()),
            this, SLOT(scrollRight()));
    connect(hsb, SIGNAL(prevLine()),
            this, SLOT(scrollLeft()));
    connect(this, SIGNAL(contentsMoving(int, int)),
            this, SLOT(slotDrawBufferNeedsRefresh()));

//     connect(this, SIGNAL(contentsMoving(int, int)),
//             this, SLOT(slotContentsMoving(int, int)));
    connect(model, SIGNAL(selectedSegments(const Rosegarden::SegmentSelection &)),
            this, SIGNAL(selectedSegments(const Rosegarden::SegmentSelection &)));

    connect(model, SIGNAL(needUpdate(QRect)),
            this, SLOT(slotUpdate(QRect)));

    connect(doc, SIGNAL(docColoursChanged()),
            this, SLOT(slotRefreshColourCache()));

    // recording-related signals
    connect(doc, SIGNAL(newMIDIRecordingSegment(Rosegarden::Segment*)),
            this, SLOT(slotNewMIDIRecordingSegment(Rosegarden::Segment*)));
    connect(doc, SIGNAL(newAudioRecordingSegment(Rosegarden::Segment*)),
            this, SLOT(slotNewAudioRecordingSegment(Rosegarden::Segment*)));
    connect(doc, SIGNAL(stoppedAudioRecording()),
            this, SLOT(slotStoppedRecording()));
    connect(doc, SIGNAL(stoppedMIDIRecording()),
            this, SLOT(slotStoppedRecording()));

    CompositionModelImpl* cmi = dynamic_cast<CompositionModelImpl*>(model);
    if (cmi) {
        cmi->setAudioPreviewThread(doc->getAudioPreviewThread());
//         cmi->refreshAllPreviews(); - no need, done by setZoom(1x)
    }

    doc->getAudioPreviewThread().setEmptyQueueListener(this);

    m_drawBuffer.setOptimization(QPixmap::BestOptim);
}

void CompositionView::initStepSize()
{
    QScrollBar* hsb = horizontalScrollBar();
    m_stepSize = hsb->lineStep();
}

void CompositionView::updateSize(bool shrinkWidth)
{
    int vStep = getModel()->grid().getYSnap();
    int height = std::max(getModel()->getNbRows(), 64u) * vStep;
    
    Rosegarden::RulerScale *ruler = grid().getRulerScale();
    int width = int(nearbyint(ruler->getTotalWidth()));

    if (!shrinkWidth && width < contentsWidth())
        width = contentsWidth();

    resizeContents(width, height);
}

void CompositionView::scrollRight()
{
    RG_DEBUG << "CompositionView::scrollRight()\n";
    if (m_stepSize == 0) initStepSize();

    if (horizontalScrollBar()->value() == horizontalScrollBar()->maxValue()) {

        resizeContents(contentsWidth() + m_stepSize, contentsHeight());
        setContentsPos(contentsX() + m_stepSize, contentsY());
        getModel()->setLength(contentsWidth());
    }
    
}

void CompositionView::scrollLeft()
{
    RG_DEBUG << "CompositionView::scrollLeft()\n";
    if (m_stepSize == 0) initStepSize();

    int cWidth = contentsWidth();
    
    if (horizontalScrollBar()->value() < cWidth && cWidth > m_minWidth) {
        resizeContents(cWidth - m_stepSize, contentsHeight());
        getModel()->setLength(contentsWidth());
    }
    
}

void CompositionView::setSelectionRectPos(const QPoint& pos)
{
    m_selectionRect.setTopLeft(pos);
    getModel()->setSelectionRect(m_selectionRect);
}

void CompositionView::setSelectionRectSize(int w, int h)
{
    m_selectionRect.setSize(QSize(w, h));
    getModel()->setSelectionRect(m_selectionRect);
}

void CompositionView::refreshAllPreviewsAndCache()
{
    dynamic_cast<CompositionModelImpl*>(getModel())->refreshAllPreviews();
    dynamic_cast<CompositionModelImpl*>(getModel())->clearDirtyPreviews();
    dynamic_cast<CompositionModelImpl*>(getModel())->clearSegmentRectsCache();    
}

void CompositionView::refreshDirtyPreviews()
{
    dynamic_cast<CompositionModelImpl*>(getModel())->refreshDirtyPreviews();
}

Rosegarden::SegmentSelection
CompositionView::getSelectedSegments()
{
    return (dynamic_cast<CompositionModelImpl*>(m_model))->getSelectedSegments();
}

void CompositionView::updateSelectionContents()
{
    if (!haveSelection()) return;

    
    QRect selectionRect = getModel()->getSelectionContentsRect();
    updateContents(selectionRect);
}

void CompositionView::slotContentsMoving(int x, int y)
{
    qDebug("contents moving : x=%d", x);
}

void CompositionView::slotSetTool(const QString& toolName)
{
    RG_DEBUG << "CompositionView::slotSetTool(" << toolName << ")"
                         << this << "\n";

    if (m_tool) m_tool->stow();

    m_tool = m_toolBox->getTool(toolName);

    if (m_tool) m_tool->ready();
    else {
        KMessageBox::error(0, QString("CompositionView::slotSetTool() : unknown tool name %1").arg(toolName));
    }
}

using Rosegarden::SegmentSelection;

void CompositionView::slotSelectSegments(const SegmentSelection &segments)
{
//     RG_DEBUG << "CompositionView::slotSelectSegments\n";

    static QRect dummy;

    getModel()->clearSelected();

    for(SegmentSelection::iterator i = segments.begin(); i != segments.end(); ++i) {
        getModel()->setSelected(CompositionItem(new CompositionItemImpl(**i, dummy)));
    }
    slotUpdate();
}

SegmentSelector*
CompositionView::getSegmentSelectorTool()
{
    return dynamic_cast<SegmentSelector*>(getToolBox()->getTool(SegmentSelector::ToolName));
}


// enter/exit selection add mode - this means that the SHIFT key
// (or similar) has been depressed and if we're in Select mode we
// can add Selections to the one we currently have
//
//
void CompositionView::slotSetSelectAdd(bool value)
{
    SegmentSelector* selTool = getSegmentSelectorTool();

    if (!selTool) return;

    selTool->setSegmentAdd(value);
}

// enter/exit selection copy mode - this means that the CTRL key
// (or similar) has been depressed and if we're in Select mode we
// can copy the current selection with a click and drag (overrides
// the default movement behaviour for selection).
//
//
void CompositionView::slotSetSelectCopy(bool value)
{
    SegmentSelector* selTool = getSegmentSelectorTool();

    if (!selTool) return;

    selTool->setSegmentCopy(value);
}

// Show the split line. This is where we perform Segment splits.
//
void CompositionView::slotShowSplitLine(int x, int y)
{
    m_splitLinePos.setX(x);
    m_splitLinePos.setY(y);
}

// Hide the split line
//
void CompositionView::slotHideSplitLine()
{
    m_splitLinePos.setX(-1);
    m_splitLinePos.setY(-1);
}


void CompositionView::slotExternalWheelEvent(QWheelEvent* e)
{
    e->accept();
    wheelEvent(e);
}

CompositionItem CompositionView::getFirstItemAt(QPoint pos)
{
    CompositionModel::itemcontainer items = getModel()->getItemsAt(pos);

    if (items.size()) {
        CompositionItem res = *(items.begin());

        items.erase(items.begin());

        // get rid of the rest;
        for(CompositionModel::itemcontainer::iterator i = items.begin();
            i != items.end(); ++i)
            delete *i;

        return res;
    }
    
    
    return CompositionItem();
}

void CompositionView::setSnapGrain(bool fine)
{
    if (m_fineGrain) {
	grid().setSnapTime(SnapGrid::NoSnap);
    } else {
	grid().setSnapTime(fine ? SnapGrid::SnapToBeat : SnapGrid::SnapToBar);
    }
}

void CompositionView::slotUpdate()
{
    RG_DEBUG << "CompositionView::slotUpdate()\n";
    slotDrawBufferNeedsRefresh();
    refreshDirtyPreviews();
    viewport()->repaint(false);
}

void CompositionView::slotUpdate(QRect rect)
{
    RG_DEBUG << "CompositionView::slotUpdate() rect " << rect << endl;
    slotDrawBufferNeedsRefresh();
    refreshDirtyPreviews();
    if (rect.isValid())
        viewport()->repaint(rect, false);
    else
        viewport()->repaint(false);
}

void CompositionView::slotRefreshColourCache()
{
    CompositionColourCache::getInstance()->init();
    refreshAllPreviewsAndCache();
    slotUpdate();
}

void CompositionView::slotNewMIDIRecordingSegment(Rosegarden::Segment* s)
{
    getModel()->addRecordingItem(CompositionItemHelper::makeCompositionItem(s));
}

void CompositionView::slotNewAudioRecordingSegment(Rosegarden::Segment* s)
{
    getModel()->addRecordingItem(CompositionItemHelper::makeCompositionItem(s));
}

void CompositionView::slotRecordMIDISegmentUpdated(Rosegarden::Segment*, Rosegarden::timeT)
{
    RG_DEBUG << "CompositionView::slotRecordMIDISegmentUpdated()\n";
    slotUpdate();
}

void CompositionView::slotStoppedRecording()
{
    getModel()->clearRecordingItems();
}

/// update size of draw buffer
void CompositionView::resizeEvent(QResizeEvent* e)
{
    QScrollView::resizeEvent(e);
    int w = std::max(m_drawBuffer.width(), visibleWidth());
    int h = std::max(m_drawBuffer.height(), visibleHeight());
    
    m_drawBuffer.resize(w, h);
    slotDrawBufferNeedsRefresh();
//     RG_DEBUG << "CompositionView::resizeEvent() : drawBuffer size = " << m_drawBuffer.size() << endl;
}

void CompositionView::viewportPaintEvent(QPaintEvent* e)
{
//     RG_DEBUG << "CompositionView::viewportPaintEvent() r = " << e->rect().normalize()
//              << " - drawbuffer size = " << m_drawBuffer.size() <<endl;

    if (m_drawBufferNeedsRefresh)
        refreshDrawBuffer(e->rect());

    bitBlt (viewport(), 0, 0, &m_drawBuffer);
}

void CompositionView::refreshDrawBuffer(const QRect& rect)
{
    Rosegarden::Profiler profiler("CompositionView::refreshDrawBuffer", true);
//     RG_DEBUG << "CompositionView::refreshDrawBuffer() r = "
//              << rect << endl;

    QPainter p;

    m_drawBuffer.fill(viewport(), 0, 0);
    p.begin(&m_drawBuffer, viewport());

    //     QPen framePen(Qt::red, 1);
    //     p.setPen(framePen);
    //     p.drawRect(rect);

    QRect r(contentsX(), contentsY(), m_drawBuffer.width(), m_drawBuffer.height());
    p.translate(-contentsX(), -contentsY());
    drawArea(&p, r);

    p.end();
    
    m_drawBufferNeedsRefresh = false;
}


void CompositionView::drawArea(QPainter *p, const QRect& clipRect)
{
    Rosegarden::Profiler profiler("CompositionView::drawArea", true);

//     RG_DEBUG << "CompositionView::drawArea() clipRect = " << clipRect << endl;

    CompositionModel::previewrectlist* audioPreviewData = 0;
    CompositionModel::PRectRanges* notationPreviewData = 0;

    //
    // Fetch previews
    //
    if (m_showPreviews) {
        notationPreviewData = &m_notationPreviewRects;
        m_notationPreviewRects.clear();
        audioPreviewData = &m_audioPreviewRects;
        m_audioPreviewRects.clear();
    }

    //
    // Fetch segment rectangles to draw
    //
    const CompositionModel::rectcontainer& rects = getModel()->getRectanglesIn(clipRect,
                                                                               notationPreviewData, audioPreviewData);
    CompositionModel::rectcontainer::const_iterator i = rects.begin();
    CompositionModel::rectcontainer::const_iterator end = rects.end();

    //
    // Draw Segment Rectangles
    //
    p->save();
    for(; i != end; ++i) {
        p->setBrush(i->getBrush());
        p->setPen(i->getPen());
        
//         RG_DEBUG << "CompositionView::drawArea : draw comp rect " << *i << endl;
        drawCompRect(*i, p, clipRect);
    }
    
    p->restore();
    
    if (rects.size() > 1) {
//         RG_DEBUG << "CompositionView::drawArea : drawing intersections\n";
        drawIntersections(rects, p, clipRect);
    }

    //
    // Previews
    //
    if (m_showPreviews) {
        p->save();
        refreshDirtyPreviews();

        // draw audio previews
        //
        CompositionModel::previewrectlist::const_iterator api = m_audioPreviewRects.begin();
        CompositionModel::previewrectlist::const_iterator apEnd = m_audioPreviewRects.end();
        
        for(; api != apEnd; ++api) {
            p->save();

            const PreviewRect& pr = *(api);
            QColor defaultCol = CompositionColourCache::getInstance()->SegmentAudioPreview;
            QColor col = pr.getColor().isValid() ? pr.getColor() : defaultCol;
            p->setBrush(col);
            p->setPen(col);
            p->drawLine(pr.topLeft(), pr.bottomLeft());

            p->restore();
        }
        
        // draw notation previews
        //
        CompositionModel::PRectRanges::const_iterator npi = m_notationPreviewRects.begin();
        CompositionModel::PRectRanges::const_iterator npEnd = m_notationPreviewRects.end();
        
        for(; npi != npEnd; ++npi) {
            CompositionModel::PRectRange interval = *npi;
            p->save();
            p->translate(interval.basePoint.x(), interval.basePoint.y());
            for(; interval.range.first != interval.range.second; ++interval.range.first) {

                const PreviewRect& pr = *(interval.range.first);
                QColor defaultCol = CompositionColourCache::getInstance()->SegmentInternalPreview;
                QColor col = pr.getColor().isValid() ? pr.getColor() : defaultCol;
                p->setBrush(col);
                p->setPen(col);
                p->drawRect(pr);
            }
            p->restore();
        }
        
        p->restore();
    }

    //
    // Draw segment labels (they must be drawn over the preview rects)
    //
    if (m_showSegmentLabels) {
        for(i = rects.begin(); i != end; ++i) {
            drawCompRectLabel(*i, p, clipRect);
        }
    }
    

    //
    // Playback Pointer
    //
    drawPointer(p, clipRect);

    //
    // Tmp rect (rect displayed while drawing a new segment)
    //
    if (m_tmpRect.isValid() && m_tmpRect.intersects(clipRect)) {
        p->setBrush(CompositionRect::DefaultBrushColor);
        p->setPen(CompositionColourCache::getInstance()->SegmentBorder);
        drawRect(m_tmpRect, p, clipRect);
    }

    //
    // Tool guides (crosshairs)
    //
    if (m_drawGuides)
        drawGuides(p, clipRect);

    //
    // Selection Rect
    //
    if (m_drawSelectionRect) {
        drawRect(m_selectionRect, p, clipRect, false, 0, false);
    }

    //
    // Floating Text
    //
    if (m_drawTextFloat)
        drawTextFloat(p, clipRect);

    //
    // Split line
    //
    if (m_splitLinePos.x() > 0 && clipRect.contains(m_splitLinePos)) {
        p->save();
        p->setPen(m_guideColor);
        p->drawLine(m_splitLinePos.x(), m_splitLinePos.y(),
                    m_splitLinePos.x(), m_splitLinePos.y() + getModel()->grid().getYSnap());
        p->restore();
    }
    
}

void CompositionView::drawGuides(QPainter * p, const QRect& /*clipRect*/)
{
    // no need to check for clipping, these guides are meant to follow the mouse cursor
    QPoint guideOrig(m_topGuidePos, m_foreGuidePos);

    p->save();
    p->setPen(m_guideColor);
    p->drawLine(guideOrig.x(), 0, guideOrig.x(), contentsHeight());
    p->drawLine(0, guideOrig.y(), contentsWidth(), guideOrig.y());
    p->restore();
}

void CompositionView::drawCompRect(const CompositionRect& r, QPainter *p, const QRect& clipRect,
                                   int intersectLvl, bool fill)
{
    p->save();

    QBrush brush = r.getBrush();
    
    if (r.isRepeating()) {
        QColor brushColor = brush.color();
        brush.setColor(brushColor.light(150));
    }
    
    p->setBrush(brush);
    p->setPen(r.getPen());
    drawRect(r, p, clipRect, r.isSelected(), intersectLvl, fill);

    if (r.isRepeating()) {

        CompositionRect::repeatmarks repeatMarks = r.getRepeatMarks();

//         RG_DEBUG << "CompositionView::drawCompRect() : drawing repeating rect " << r
//                  << " nb repeat marks = " << repeatMarks.size() << endl;

        // draw 'start' rectangle with original brush
        //
        QRect startRect = r;
        startRect.setWidth(repeatMarks[0] - r.x());
        p->setBrush(r.getBrush());
        drawRect(startRect, p, clipRect, r.isSelected(), intersectLvl, fill);
        

        // now draw the 'repeat' marks
        //
        p->setPen(CompositionColourCache::getInstance()->RepeatSegmentBorder);
        int penWidth = std::max(r.getPen().width(), 1u);

        for (unsigned int i = 0; i < repeatMarks.size(); ++i) {
            int pos = repeatMarks[i];
            if (pos > clipRect.right())
                break;
            
            if (pos >= clipRect.left()) {
                QPoint p1(pos, r.y() + penWidth),
                    p2(pos, r.y() + r.height() - penWidth - 1);
                
//                 RG_DEBUG << "CompositionView::drawCompRect() : drawing repeat mark at "
//                          << p1 << "-" << p2 << endl;
                p->drawLine(p1, p2);
            }
            
        }

    }

    p->restore();
}

void CompositionView::drawCompRectLabel(const CompositionRect& r, QPainter *p, const QRect& clipRect)
{
    // draw segment label
    //
#ifdef NOT_DEFINED
    if (!r.getLabel().isEmpty() /* && !r.isSelected() */) {
        p->save();
        p->setPen(Rosegarden::GUIPalette::getColour(Rosegarden::GUIPalette::SegmentLabel));
        p->setBrush(white);
        QRect textRect(r);
        textRect.setX(textRect.x() + 3);
        QString label = " " + r.getLabel() + " ";
        QRect textBoundingRect = p->boundingRect(textRect, Qt::AlignLeft|Qt::AlignVCenter, label);
        p->drawRect(textBoundingRect & r);
        p->drawText(textRect, Qt::AlignLeft|Qt::AlignVCenter, label);
        p->restore();
    }
#else
    if (!r.getLabel().isEmpty()) {

        p->save();

	QFont font;
	font.setPixelSize(r.height() / 2.2);
	font.setWeight(QFont::Bold);
	font.setItalic(false);
	p->setFont(font);

	QRect labelRect = QRect
	    (r.x(),
	     r.y() + ((r.height() - p->fontMetrics().height()) / 2) + 1,
	     r.width(),
	     p->fontMetrics().height());

        int x = labelRect.x() + p->fontMetrics().width('x');
        int y = labelRect.y();

	QBrush brush = r.getBrush();
	QColor surroundColour = brush.color().light(110);

	int h, s, v;
	surroundColour.hsv(&h, &s, &v);
	if (v < 150) surroundColour.setHsv(h, s, 225);
	p->setPen(surroundColour);

	for (int i = 0; i < 9; ++i) {

	    if (i == 4) continue;

	    int wx = x, wy = y;

	    if (i < 3) --wx;
	    if (i > 5) ++wx;
	    if (i % 3 == 0) --wy;
	    if (i % 3 == 2) ++wy;

	    labelRect.setX(wx);
	    labelRect.setY(wy);

	    p->drawText(labelRect,
			Qt::AlignLeft | Qt::AlignTop,
			r.getLabel());
	}

        labelRect.setX(x);
        labelRect.setY(y);

        p->setPen(Rosegarden::GUIPalette::getColour
		  (Rosegarden::GUIPalette::SegmentLabel));
        p->drawText(labelRect,
		    Qt::AlignLeft|Qt::AlignVCenter, r.getLabel());
        p->restore();
    }
#endif
}


void CompositionView::drawRect(const QRect& r, QPainter *p, const QRect& clipRect,
                               bool isSelected, int intersectLvl, bool fill)
{
//     RG_DEBUG << "CompositionView::drawRect : intersectLvl = " << intersectLvl
//              << " - brush col = " << p->brush().color() << endl;

//     RG_DEBUG << "CompositionView::drawRect " << r << " - xformed : " << p->xForm(r)
//              << " - contents x = " << contentsX() << ", contents y = " << contentsY() << endl;

    p->save();
    
    QRect rect = r;

    if (fill) {
        if (isSelected) {
            QColor fillColor = p->brush().color();
            fillColor = fillColor.dark(200);
            QBrush b = p->brush();
            b.setColor(fillColor);
            p->setBrush(b);
        }
    
        if (intersectLvl > 0) {
            QColor fillColor = p->brush().color();
            fillColor = fillColor.dark((intersectLvl) * 105);
            QBrush b = p->brush();
            b.setColor(fillColor);
            p->setBrush(b);
        }
    } else {
        p->setBrush(Qt::NoBrush);
    }

    // Paint using the small coordinates...
    QRect intersection = rect.intersect(clipRect);
    
    if (clipRect.contains(rect)) {
        p->drawRect(rect);
    } else {
        // draw only what's necessary
        if (!intersection.isEmpty() && fill)
            p->fillRect(intersection, p->brush());

        int rectTopY = rect.y();

        if (rectTopY >= clipRect.y() &&
            rectTopY <= (clipRect.y() + clipRect.height())) {
            // to prevent overflow, in case the original rect is too wide
            // the line would be drawn "backwards"
            p->drawLine(intersection.topLeft(), intersection.topRight());
        }

        int rectBottomY = rect.y() + rect.height();
        if (rectBottomY >= clipRect.y() &&
            rectBottomY <= (clipRect.y() + clipRect.height()))
            // to prevent overflow, in case the original rect is too wide
            // the line would be drawn "backwards"
            p->drawLine(intersection.bottomLeft(), intersection.bottomRight());

        int rectLeftX = rect.x();
        if (rectLeftX >= clipRect.x() &&
            rectLeftX <= (clipRect.x() + clipRect.width()))
            p->drawLine(rect.topLeft(), rect.bottomLeft());

        unsigned int rectRightX = rect.x() + rect.width(); // make sure we don't overflow
        if (rectRightX >= unsigned(clipRect.x()) &&
            rectRightX <= unsigned(clipRect.x() + clipRect.width()))
            p->drawLine(rect.topRight(), rect.bottomRight());

    }

    p->restore();
}

QColor CompositionView::mixBrushes(QBrush a, QBrush b)
{
    QColor ac = a.color(), bc = b.color();
    
    int aR = ac.red(), aG = ac.green(), aB = ac.blue(),
        bR = bc.red(), bG = bc.green(), bB = ac.blue();
    
    ac.setRgb((aR + bR) / 2, (aG + bG) / 2, (aB + bB) / 2);

    return ac;
}

void CompositionView::drawIntersections(const CompositionModel::rectcontainer& rects,
                                        QPainter * p, const QRect& clipRect)
{
    if (! (rects.size() > 1)) return;

    CompositionModel::rectcontainer intersections;

    CompositionModel::rectcontainer::const_iterator i = rects.begin(),
        j = rects.begin();

    for (; j != rects.end(); ++j) {

        CompositionRect testRect = *j;
        i = j; ++i; // set i to pos after j

        if (i == rects.end()) break;

        for(; i != rects.end(); ++i) {
            CompositionRect ri = testRect.intersect(*i);
            if (!ri.isEmpty()) {
                 CompositionModel::rectcontainer::iterator t = std::find(intersections.begin(),
                                                                         intersections.end(), ri);
                if (t == intersections.end()) {
                    ri.setBrush(mixBrushes(testRect.getBrush(), i->getBrush()));
                    intersections.push_back(ri);
                }
                
            }
        }
    }

    //
    // draw this level of intersections then compute and draw further ones
    //
    int intersectionLvl = 1;

    while(!intersections.empty()) {

        for(CompositionModel::rectcontainer::iterator intIter = intersections.begin();
            intIter != intersections.end(); ++intIter) {
            CompositionRect r = *intIter;
            drawCompRect(r, p, clipRect, intersectionLvl);
        }

        ++intersectionLvl;

        CompositionModel::rectcontainer intersections2;

        CompositionModel::rectcontainer::iterator i = intersections.begin(),
            j = intersections.begin();

        for (; j != intersections.end(); ++j) {

            CompositionRect testRect = *j;
            i = j; ++i; // set i to pos after j

            if (i == intersections.end()) break;

            for(; i != intersections.end(); ++i) {
                CompositionRect ri = testRect.intersect(*i);
                if (!ri.isEmpty() && ri != *i) {
                    CompositionModel::rectcontainer::iterator t = std::find(intersections2.begin(),
                                                                            intersections2.end(), ri);
                    if (t == intersections2.end())
                        ri.setBrush(mixBrushes(testRect.getBrush(), i->getBrush()));
                        intersections2.push_back(ri);
                }
            }
        }

        intersections = intersections2;
    }

}

void CompositionView::drawPointer(QPainter *p, const QRect& clipRect)
{
    if (m_pointerPos >= clipRect.x() && m_pointerPos <= (clipRect.x() + clipRect.width())) {
        p->save();
        p->setPen(m_pointerPen);
        p->drawLine(m_pointerPos, clipRect.y(), m_pointerPos, clipRect.y() + clipRect.height());
        p->restore();
    }
    
}

void CompositionView::drawTextFloat(QPainter *p, const QRect& clipRect)
{
    QRect bound = p->boundingRect(0, 0, 300, 40, AlignAuto, m_textFloatText);

    if (!bound.intersects(clipRect)) return;

    p->save();

    bound.setLeft(bound.left() - 2);
    bound.setRight(bound.right() + 2);
    bound.setTop(bound.top() - 2);
    bound.setBottom(bound.bottom() + 2);
    bound.moveTopLeft(m_textFloatPos);

    drawRect(bound, p, clipRect, false, 0, false);

    p->setPen(CompositionColourCache::getInstance()->RotaryFloatForeground);
    p->drawText(m_textFloatPos.x() + 2, m_textFloatPos.y() + 14, m_textFloatText);

    p->restore();
}

bool CompositionView::event(QEvent* e)
{
    if (e->type() == AudioPreviewThread::AudioPreviewQueueEmpty) {
        RG_DEBUG << "CompositionView::event - AudioPreviewQueueEmpty\n";
        slotDrawBufferNeedsRefresh();
        viewport()->update();
        return true;
    }

    return RosegardenScrollView::event(e);
}

void CompositionView::contentsMousePressEvent(QMouseEvent* e)
{
    slotDrawBufferNeedsRefresh();

    switch (e->button()) {
    case LeftButton:
    case MidButton:
        startAutoScroll();

        if (m_tool) m_tool->handleMouseButtonPress(e);
        else
            RG_DEBUG << "CompositionView::contentsMousePressEvent() :"
                     << this << " no tool\n";
        break;
    case RightButton:
        if (m_tool) m_tool->handleRightButtonPress(e);
        else
            RG_DEBUG << "CompositionView::contentsMousePressEvent() :"
                     << this << " no tool\n";
        break;
    default:
        break;
    }
}

void CompositionView::contentsMouseReleaseEvent(QMouseEvent* e)
{
    RG_DEBUG << "CompositionView::contentsMouseReleaseEvent()\n";

    slotDrawBufferNeedsRefresh();

    stopAutoScroll();

    if (!m_tool) return;

    if (e->button() == LeftButton ||
        e->button() == MidButton ) m_tool->handleMouseButtonRelease(e);
}

void CompositionView::contentsMouseDoubleClickEvent(QMouseEvent* e)
{
    m_currentItem = getFirstItemAt(e->pos());

    if (!m_currentItem) {
        RG_DEBUG << "CompositionView::contentsMouseDoubleClickEvent - no currentItem\n";
        return;
    }
    
    RG_DEBUG << "CompositionView::contentsMouseDoubleClickEvent - have currentItem\n";

    CompositionItemImpl* itemImpl = dynamic_cast<CompositionItemImpl*>((_CompositionItem*)m_currentItem);
        
    if (m_currentItem->isRepeating()) {
        Rosegarden::timeT time = getModel()->getRepeatTimeAt(e->pos(), m_currentItem);

        RG_DEBUG << "editRepeat at time " << time << endl;
        if (time > 0)
            emit editRepeat(itemImpl->getSegment(), time);
        else
            emit editSegment(itemImpl->getSegment());

    } else {
            
        emit editSegment(itemImpl->getSegment());
    }
}

void CompositionView::contentsMouseMoveEvent(QMouseEvent* e)
{
    slotDrawBufferNeedsRefresh();

    if (!m_tool) return;

    int follow = m_tool->handleMouseMove(e);
    setScrollDirectionConstraint(follow);
    
    if (follow != RosegardenCanvasView::NoFollow) {
        doAutoScroll();

        if (follow & RosegardenCanvasView::FollowHorizontal)
            slotScrollHorizSmallSteps(e->pos().x());

        if (follow & RosegardenCanvasView::FollowVertical)
            slotScrollVertSmallSteps(e->pos().y());
    }
}

void CompositionView::releaseCurrentItem()
{
    m_currentItem = CompositionItem();
}

void CompositionView::setPointerPos(int pos)
{
    int oldPos = m_pointerPos;
    bool smallChange = abs(oldPos - pos) < 10;

    if (smallChange)
        pointerMoveUpdate();

    m_pointerPos = pos;

    if (smallChange)
        pointerMoveUpdate();
    else
        pointerMoveUpdate(oldPos);
}

void CompositionView::setTextFloat(int x, int y, const QString &text)
{
    m_textFloatPos.setX(x);
    m_textFloatPos.setY(y);
    m_textFloatText = text;
    m_drawTextFloat = true;
    slotDrawBufferNeedsRefresh();
}

void CompositionView::pointerMoveUpdate(int oldPos)
{
    slotDrawBufferNeedsRefresh();

    if (oldPos < 0) { // "large" change - only update around the current pointer position

        int x = std::max(0, m_pointerPos - int(m_pointerPen.width()) - 2);
        updateContents(QRect(x, 0,
                             m_pointerPen.width() + 4, contentsHeight()));

    } else {

        int left = oldPos, right = m_pointerPos;
        if (oldPos > m_pointerPos) {
            left = m_pointerPos;
            right = oldPos;
        }
        int x = std::max(0, left - int(m_pointerPen.width()) - 2);
        updateContents(QRect(x, 0,
                             m_pointerPen.width() + 4 + (right - left), contentsHeight()));
        
    }
}

void CompositionView::slotSetFineGrain(bool value)
{
    m_fineGrain = value;
}

void 
CompositionView::slotTextFloatTimeout() 
{ 
    hideTextFloat();
    slotDrawBufferNeedsRefresh();
}

#include "compositionview.moc"
