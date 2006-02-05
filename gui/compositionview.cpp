// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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
#include <qbitmap.h>

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
#include "rgapplication.h"
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
 	// t = std::max(grid.snapX(item->rect().x()), 0L); - wrong, we can have negative start times,
        // and if we do this we 'crop' segments when they are moved before the start of the composition
	t = grid.snapX(item->rect().x());

//         RG_DEBUG << "CompositionItemHelper::getStartTime(): item is repeating : " << item->isRepeating()
//                  << " - startTime = " << t
//                  << endl;
    }

    return t;
}

timeT CompositionItemHelper::getEndTime(const CompositionItem& item, const Rosegarden::SnapGrid& grid)
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
      m_pointerTimePos(0),
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
    RG_DEBUG << "CompositionModelImpl::~CompositionModelImpl()" << endl;

    if (!isCompositionDeleted()) {

        m_composition.removeObserver(this);

        const Composition::segmentcontainer& segments = m_composition.getSegments();
        Composition::segmentcontainer::iterator segEnd = segments.end();

        for(Composition::segmentcontainer::iterator i = segments.begin();
            i != segEnd; ++i) {

            (*i)->removeObserver(this);
        }
    }

    RG_DEBUG << "CompositionModelImpl::~CompositionModelImpl(): removal from Segment & Composition observers OK" << endl;
    
    if (m_audioPreviewThread) {
	while (!m_audioPreviewUpdaterMap.empty()) {
	    // Cause any running previews to be cancelled
	    delete m_audioPreviewUpdaterMap.begin()->second;
	    m_audioPreviewUpdaterMap.erase(m_audioPreviewUpdaterMap.begin());
	}
    }
}

unsigned int CompositionModelImpl::getNbRows()
{
    return m_composition.getNbTracks();
}

const CompositionModel::rectcontainer& CompositionModelImpl::getRectanglesIn(const QRect& rect,
                                                                             RectRanges* npData,
                                                                             AudioPreviewDrawData* apData)
{
//    Rosegarden::Profiler profiler("CompositionModelImpl::getRectanglesIn", true);

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
//         RG_DEBUG << "CompositionModelImpl::getRectanglesIn: seg rect = " << sr << endl;

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
                makeNotationPreviewRects(npData, QPoint(0, sr.y()), s, rect);                
            // Audio preview data
            } else if (apData && s->getType() == Rosegarden::Segment::Audio) {
                makeAudioPreviewRects(apData, s, sr, rect);
            }
            
            m_res.push_back(sr);
        } else {
//             RG_DEBUG << "CompositionModelImpl::getRectanglesIn: - segment out of rect\n";
        }
        
    }

    // changing items

    itemcontainer::iterator movEnd = m_changingItems.end();
    for(itemcontainer::iterator i = m_changingItems.begin(); i != movEnd; ++i) {
        CompositionRect sr((*i)->rect());
        if (sr.intersects(rect)) {
            Segment* s = CompositionItemHelper::getSegment(*i);
            sr.setSelected(true);
            QColor brushColor = GUIPalette::convertColour(m_composition.getSegmentColourMap().getColourByIndex(s->getColourIndex()));
            sr.setBrush(brushColor);

            sr.setPen(CompositionColourCache::getInstance()->SegmentBorder);
            
            // Notation preview data
            if (npData && s->getType() == Rosegarden::Segment::Internal) {
                makeNotationPreviewRectsMovingSegment(npData, sr.topLeft(), s, sr);
            // Audio preview data
            } else if (apData && s->getType() == Rosegarden::Segment::Audio) {
                makeAudioPreviewRects(apData, s, sr, rect);
            }

            m_res.push_back(sr);
        }
    }

    return m_res;
}

struct RectCompare {
    bool operator()(const QRect &r1, const QRect &r2) const {
        return r1.x() < r2.x();
    }
};



void CompositionModelImpl::makeNotationPreviewRects(RectRanges* npRects, QPoint basePoint,
                                                    const Segment* segment, const QRect& clipRect)
{

    rectlist* cachedNPData = getNotationPreviewData(segment);

    if (cachedNPData->empty())
        return;

    rectlist::iterator npEnd = cachedNPData->end();

    rectlist::iterator npi = std::lower_bound(cachedNPData->begin(), npEnd, clipRect, RectCompare());

    if (npi == npEnd)
        return;

    if (npi != cachedNPData->begin())
        --npi;

    RectRange interval;
    
    interval.range.first = npi;

    int segEndX = int(nearbyint(m_grid.getRulerScale()->getXForTime(segment->getEndMarkerTime())));
    int xLim = std::min(clipRect.topRight().x(), segEndX);

//     RG_DEBUG << "CompositionModelImpl::makeNotationPreviewRects : basePoint.x : "
//              << basePoint.x() << endl;

    // move iterator forward
    //
    while (npi->x() < xLim && npi != npEnd) ++npi;

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
        return;

    rectlist::iterator npEnd = cachedNPData->end(),
        npBegin = cachedNPData->begin();

    rectlist::iterator npi;
    
    if (getChangeType() == ChangeResizeFromStart)
        npi = std::lower_bound(npBegin, npEnd, currentSR, RectCompare());
    else
        npi = std::lower_bound(npBegin, npEnd, unmovedSR, RectCompare());

    if (npi == npEnd)
        return;

    if (npi != npBegin && getChangeType() != ChangeResizeFromStart) {
        --npi;
    }
    
    RectRange interval;
    
    interval.range.first = npi;

    int xLim = getChangeType() == ChangeMove ? unmovedSR.topRight().x() : currentSR.topRight().x();

//     RG_DEBUG << "CompositionModelImpl::makeNotationPreviewRectsMovingSegment : basePoint.x : "
//              << basePoint.x() << endl;

    // move iterator forward
    //
    while (npi->x() < xLim && npi != npEnd) ++npi;

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
                                                 const CompositionRect& segRect, const QRect& clipRect)
{
    Rosegarden::Profiler profiler("CompositionModelImpl::makeAudioPreviewRects", true);
    RG_DEBUG << "CompositionModelImpl::makeAudioPreviewRects - segRect = " << segRect << endl;

    PixmapArray previewImage = getAudioPreviewPixmap(segment);
    int penWidth = 0; // std::max(1U, segRect.getPen().width());
    QPoint basePoint(segRect.x() + penWidth, segRect.y() + penWidth);

    AudioPreviewDrawDataItem previewItem(previewImage, basePoint, segRect);

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

void CompositionModelImpl::setAudioPreviewThread(AudioPreviewThread *thread)
{
//    std::cerr << "\nCompositionModelImpl::setAudioPreviewThread()\n" << std::endl;

    while (!m_audioPreviewUpdaterMap.empty()) {
	// Cause any running previews to be cancelled
	delete m_audioPreviewUpdaterMap.begin()->second;
	m_audioPreviewUpdaterMap.erase(m_audioPreviewUpdaterMap.begin());
    }

    m_audioPreviewThread = thread;
}

void CompositionModelImpl::clearPreviewCache()
{
    RG_DEBUG << "CompositionModelImpl::clearPreviewCache\n";

    m_notationPreviewDataCache.clear();
    m_audioPreviewDataCache.clear();
    m_audioSegmentPreviewMap.clear();

    for (AudioPreviewUpdaterMap::iterator i = m_audioPreviewUpdaterMap.begin();
	 i != m_audioPreviewUpdaterMap.end(); ++i) {
	i->second->cancel();
    }

    const Composition::segmentcontainer& segments = m_composition.getSegments();
    Composition::segmentcontainer::iterator segEnd = segments.end();

    for(Composition::segmentcontainer::iterator i = segments.begin();
        i != segEnd; ++i) {

	if ((*i)->getType() == Rosegarden::Segment::Audio) {
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

    for (Segment::iterator i = segment->begin();
	 i != segment->end(); ++i) {

        long pitch = 0;
        if (!(*i)->isa(Rosegarden::Note::EventType) ||
            !(*i)->get<Rosegarden::Int>
            (Rosegarden::BaseProperties::PITCH, pitch)) {
            continue;
        }

	timeT eventStart = (*i)->getAbsoluteTime();
	timeT eventEnd = eventStart + (*i)->getDuration();
// 	if (eventEnd > segment->getEndMarkerTime()) {
// 	    eventEnd = segment->getEndMarkerTime();
// 	}

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

        QRect r(x, (int)y, width, 2);

//         RG_DEBUG << "CompositionModelImpl::updatePreviewCacheForNotationSegment() : npData = "
//                  << npData << ", preview rect = "
//                  << r << endl;
        npData->push_back(r);
    }

}

QColor CompositionModelImpl::computeSegmentPreviewColor(const Segment* segment) 
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
//	std::cerr << "CompositionModelImpl::updatePreviewCacheForAudioSegment() - new audio preview started" << std::endl;

        QRect segRect = computeSegmentRect(*segment);
        segRect.moveTopLeft(QPoint(0,0));
	
	if (apData) apData->setSegmentRect(segRect);

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
        RG_DEBUG << "CompositionModelImpl::updatePreviewCacheForAudioSegment() - no audio preview thread set\n";
    }
}

void CompositionModelImpl::slotAudioPreviewComplete(AudioPreviewUpdater* apu)
{
    RG_DEBUG << "CompositionModelImpl::slotAudioPreviewComplete()\n";
    
    AudioPreviewData *apData = getAudioPreviewData(apu->getSegment());
    QRect updateRect;
    
    if (apData) {
	RG_DEBUG << "CompositionModelImpl::slotAudioPreviewComplete(" << apu << "): apData contains " << apData->getValues().size() << " values already" << endl;
	unsigned int channels = 0;
	const std::vector<float> &values = apu->getComputedValues(channels);
	if (channels > 0) {
	    RG_DEBUG << "CompositionModelImpl::slotAudioPreviewComplete: set "
                     << values.size() << " samples on " << channels << " channels\n";
	    apData->setChannels(channels);
	    apData->setValues(values);
            updateRect = postProcessAudioPreview(apData, apu->getSegment());
	}
    }

    if (!updateRect.isEmpty()) 
        emit needContentUpdate(updateRect);
}

class AudioPreviewPainter {
public:
    AudioPreviewPainter(CompositionModelImpl& model,
			CompositionModel::AudioPreviewData* apData,
			const Rosegarden::Composition &composition,
			const Rosegarden::Segment* segment);

    void paintPreviewImage();
    PixmapArray getPreviewImage();
    const CompositionRect& getSegmentRect() { return m_rect; }

    static int tileWidth();

protected:
    void finalizeCurrentSlice();

    //--------------- Data members ---------------------------------
    CompositionModelImpl& m_model;
    CompositionModel::AudioPreviewData* m_apData;
    const Rosegarden::Composition &m_composition;
    const Rosegarden::Segment* m_segment;
    CompositionRect m_rect;

    QImage m_image;
    PixmapArray m_previewPixmaps;

    QPainter m_p;
    QPainter m_pb;
    QColor m_defaultCol;
    int m_penWidth;
    int m_height;
    int m_halfRectHeight;
    int m_sliceNb;

};

AudioPreviewPainter::AudioPreviewPainter(CompositionModelImpl& model,
					 CompositionModel::AudioPreviewData* apData,
					 const Rosegarden::Composition &composition,
					 const Rosegarden::Segment* segment)
    : m_model(model),
      m_apData(apData),
      m_composition(composition),
      m_segment(segment),
      m_rect(model.computeSegmentRect(*(segment))),
      m_image(1, 1, 8, 4),
      m_defaultCol(CompositionColourCache::getInstance()->SegmentAudioPreview),
      m_height(model.grid().getYSnap()/2)
{
    int pixWidth = std::min(m_rect.getBaseWidth(), tileWidth());

    m_image = QImage(pixWidth, m_rect.height(), 8, 4);
    m_image.setAlphaBuffer(true);

    m_penWidth = (std::max(1U, m_rect.getPen().width()) * 2);
    m_halfRectHeight = m_model.grid().getYSnap()/2 - m_penWidth / 2 - 2;
}

int AudioPreviewPainter::tileWidth()
{
    static int tw = -1;
    if (tw == -1) tw = QApplication::desktop()->width();
    return tw;
}

void AudioPreviewPainter::paintPreviewImage()
{
    const std::vector<float>& values = m_apData->getValues();

    if (values.size() == 0)
        return;
        
    float gain[2] = { 1.0, 1.0 };
    int instrumentChannels = 2;
    Rosegarden::TrackId trackId = m_segment->getTrack();
    Rosegarden::Track *track = m_model.getComposition().getTrackById(trackId);
    if (track) {
        Rosegarden::Instrument *instrument = m_model.getStudio().getInstrumentById(track->getInstrument());
        if (instrument) {
            float level = Rosegarden::AudioLevel::dB_to_multiplier(instrument->getLevel());
            float pan = instrument->getPan() - 100.0;
            gain[0] = level * ((pan > 0.0) ? (1.0 - (pan / 100.0)) : 1.0);
            gain[1] = level * ((pan < 0.0) ? ((pan + 100.0) / 100.0) : 1.0);
	    instrumentChannels = instrument->getAudioChannels();
        }
    }

    bool showMinima = m_apData->showsMinima();
    unsigned int channels = m_apData->getChannels();
    if (channels == 0) {
        RG_DEBUG << "AudioPreviewPainter::paintPreviewImage : problem with audio file for segment "
                 << m_segment->getLabel().c_str() << endl;
        return;
    }

    int samplePoints = values.size() / (channels * (showMinima ? 2 : 1));
    float h1, h2, l1 = 0, l2 = 0;
    double sampleScaleFactor = samplePoints / double(m_rect.getBaseWidth());
    m_sliceNb = 0;

    m_image.fill(0);

    int centre = m_image.height() / 2;

    RG_DEBUG << "AudioPreviewPainter::paintPreviewImage width = " << m_rect.getBaseWidth() << ", height = " << m_rect.height() << ", halfRectHeight = " << m_halfRectHeight << endl;

    RG_DEBUG << "AudioPreviewPainter::paintPreviewImage: channels = " << channels << ", gain left = " << gain[0] << ", right = " << gain[1] << endl;

    double audioDuration = double(m_segment->getAudioEndTime().sec) +
	double(m_segment->getAudioEndTime().nsec) / 1000000000.0;

    // We need to take each pixel value and map it onto a point within
    // the preview.  We have samplePoints preview points in a known
    // duration of audioDuration.  Thus each point spans a real time
    // of audioDuration / samplePoints.  We need to convert the
    // accumulated real time back into musical time, and map this
    // proportionately across the segment width.

    Rosegarden::RealTime startRT =
	m_model.getComposition().getElapsedRealTime(m_segment->getStartTime());
    double startTime = double(startRT.sec) + double(startRT.nsec) / 1000000000.0;

    Rosegarden::RealTime endRT =
	m_model.getComposition().getElapsedRealTime(m_segment->getEndMarkerTime());
    double endTime = double(endRT.sec) + double(endRT.nsec) / 1000000000.0;

    bool haveTempoChange = false;

    int finalTempoChangeNumber =
	m_model.getComposition().getTempoChangeNumberAt
	(m_segment->getEndMarkerTime());

    if ((finalTempoChangeNumber >= 0) &&

	(finalTempoChangeNumber > 
	 m_model.getComposition().getTempoChangeNumberAt
	 (m_segment->getStartTime()))) {

	haveTempoChange = true;
    }

    for (int i = 0; i < m_rect.getBaseWidth(); ++i) {

	// i is the x coordinate within the rectangle.  We need to
	// calculate the position within the audio preview from which
	// to draw the peak for this coordinate.  It's possible there
	// may be more than one, in which case we need to find the
	// peak of all of them.

	int position = 0;

	if (haveTempoChange) {
	    
	    // First find the time corresponding to this i.
	    timeT musicalTime =
		m_model.grid().getRulerScale()->getTimeForX(m_rect.x() + i);
	    Rosegarden::RealTime realTime =
		m_model.getComposition().getElapsedRealTime(musicalTime);
	    
	    double time = double(realTime.sec) +
		double(realTime.nsec) / 1000000000.0;
	    double offset = time - startTime;

	    if (endTime > startTime) {
		position = offset * m_rect.getBaseWidth() / (endTime - startTime);
		position = int(channels * position);
	    }
	    
	} else {

	    position = int(channels * i * sampleScaleFactor);
	}

        if (position < 0) continue;

        if (position >= values.size() - channels) {
            finalizeCurrentSlice();
            break;
        }

        if (channels == 1) {

            h1 = values[position++];
            h2 = h1;

            if (showMinima) {
                l1 = values[position++];
                l2 = l1;
            }
        } else {

            h1 = values[position++];
            if (showMinima) l1 = values[position++];

            h2 = values[position++];
            if (showMinima) l2 = values[position++];
            
        }

	if (instrumentChannels == 1 && channels == 2) {
	    h1 = h2 = (h1 + h2) / 2;
	    l1 = l2 = (l1 + l2) / 2;
	}

	h1 *= gain[0];
	h2 *= gain[1];
	
	l1 *= gain[0];
	l2 *= gain[1];

        int width = 1;
	int pixel;

        // h1 left, h2 right
        if (h1 >= 1.0) { h1 = 1.0; pixel = 2; }
	else { pixel = 1; }

        int h = Rosegarden::AudioLevel::multiplier_to_preview(h1, m_height);
        if (h <= 0) h = 1;
	if (h > m_halfRectHeight) h = m_halfRectHeight;

        int rectX = i % tileWidth();

	for (int py = 0; py < h; ++py) {
	    m_image.setPixel(rectX, centre - py, pixel);
	}
	
        if (h2 >= 1.0) { h2 = 1.0; pixel = 2; }
        else { pixel = 1; }

        h = Rosegarden::AudioLevel::multiplier_to_preview(h2, m_height);
        if (h < 0) h = 0;
	
	for (int py = 0; py < h; ++py) {
	    m_image.setPixel(rectX, centre + py, pixel);
	}

        if (((i+1) % tileWidth()) == 0 || i == (m_rect.getBaseWidth() - 1)) {
            finalizeCurrentSlice();
        }
    }

/* Auto-fade not yet implemented.

    if (m_segment->isAutoFading()) {

        Composition &comp = m_model.getComposition();

        int audioFadeInEnd = int(
                                 m_model.grid().getRulerScale()->getXForTime(comp.
                                                                     getElapsedTimeForRealTime(m_segment->getFadeInTime()) +
                                                                     m_segment->getStartTime()) -
                                 m_model.grid().getRulerScale()->getXForTime(m_segment->getStartTime()));

        m_p.setPen(Qt::blue);
        m_p.drawRect(0,  m_apData->getSegmentRect().height() - 1, audioFadeInEnd, 1);
        m_pb.drawRect(0, m_apData->getSegmentRect().height() - 1, audioFadeInEnd, 1);
    }

    m_p.end();
    m_pb.end();
*/
}

void AudioPreviewPainter::finalizeCurrentSlice()
{
//     RG_DEBUG << "AudioPreviewPainter::finalizeCurrentSlice : copying pixmap to image at " << m_sliceNb * tileWidth() << endl;

    // transparent background
    m_image.setColor(0, qRgba(255, 255, 255, 0));

    // foreground from computeSegmentPreviewColor
    QColor c = m_model.computeSegmentPreviewColor(m_segment);
    QRgb rgba = qRgba(c.red(), c.green(), c.blue(), 255);
    m_image.setColor(1, rgba);

    // red for clipping
    m_image.setColor(2, qRgba(255, 0, 0, 255));

    m_previewPixmaps.push_back(m_image.copy());

    m_image.fill(0);

    ++m_sliceNb;
}

PixmapArray AudioPreviewPainter::getPreviewImage()
{
    return m_previewPixmaps;
}

QRect CompositionModelImpl::postProcessAudioPreview(AudioPreviewData* apData, const Segment* segment)
{
    RG_DEBUG << "CompositionModelImpl::postProcessAudioPreview()\n";

    AudioPreviewPainter previewPainter(*this, apData, m_composition, segment);
    previewPainter.paintPreviewImage();

    m_audioSegmentPreviewMap[segment] = previewPainter.getPreviewImage();
    
    return previewPainter.getSegmentRect();
}

void CompositionModelImpl::slotInstrumentParametersChanged(Rosegarden::InstrumentId id)
{
    RG_DEBUG << "CompositionModelImpl::slotInstrumentParametersChanged()\n";
    const Composition::segmentcontainer& segments = m_composition.getSegments();
    Composition::segmentcontainer::iterator segEnd = segments.end();

    for (Composition::segmentcontainer::iterator i = segments.begin();
        i != segEnd; ++i) {

        Segment* s = *i;
        if (s->getType() == Rosegarden::Segment::Audio) {

            Rosegarden::TrackId trackId = s->getTrack();
            Rosegarden::Track *track = getComposition().getTrackById(trackId);
            if (track && track->getInstrument() == id) {
                makeAudioPreviewDataCache(s);
            }
        }
    }
}

void CompositionModelImpl::slotAudioFileFinalized(Rosegarden::Segment* s)
{
    RG_DEBUG << "CompositionModelImpl::slotAudioFileFinalized()\n";
    removePreviewCache(s);
}

CompositionModel::rectlist* CompositionModelImpl::getNotationPreviewData(const Rosegarden::Segment* s)
{
    rectlist* npData = m_notationPreviewDataCache[const_cast<Rosegarden::Segment*>(s)];
 
    if (!npData) {
        npData = makeNotationPreviewDataCache(s);
    }

    return npData;
}

CompositionModel::AudioPreviewData* CompositionModelImpl::getAudioPreviewData(const Rosegarden::Segment* s)
{
//    Rosegarden::Profiler profiler("CompositionModelImpl::getAudioPreviewData", true);
    RG_DEBUG << "CompositionModelImpl::getAudioPreviewData\n";

    AudioPreviewData* apData = m_audioPreviewDataCache[const_cast<Rosegarden::Segment*>(s)];

    if (!apData) {
        apData = makeAudioPreviewDataCache(s);
    }

    return apData;
}

PixmapArray CompositionModelImpl::getAudioPreviewPixmap(const Rosegarden::Segment* s)
{
    getAudioPreviewData(s);
    return m_audioSegmentPreviewMap[s];
}


void CompositionModelImpl::eventAdded(const Rosegarden::Segment *s, Rosegarden::Event *)
{
    RG_DEBUG << "CompositionModelImpl::eventAdded()\n";
    removePreviewCache(s);
    emit needContentUpdate(computeSegmentRect(*s));
}

void CompositionModelImpl::eventRemoved(const Rosegarden::Segment *s, Rosegarden::Event *)
{
    RG_DEBUG << "CompositionModelImpl::eventRemoved" << endl;
    removePreviewCache(s);
    emit needContentUpdate(computeSegmentRect(*s));
}

void CompositionModelImpl::appearanceChanged(const Rosegarden::Segment *s)
{
    RG_DEBUG << "CompositionModelImpl::appearanceChanged" << endl;
    clearInCache(s, true);
    emit needContentUpdate(computeSegmentRect(*s));
}

void CompositionModelImpl::endMarkerTimeChanged(const Rosegarden::Segment *s, bool shorten)
{
    RG_DEBUG << "CompositionModelImpl::endMarkerTimeChanged(" << shorten << ")" << endl;
    clearInCache(s, true);
    if (shorten) {
	emit needContentUpdate(); // no longer know former segment dimension
    } else {
	emit needContentUpdate(computeSegmentRect(*s));
    }
}


void CompositionModelImpl::makePreviewCache(const Segment *s) 
{
    if (s->getType() == Rosegarden::Segment::Internal) {
        makeNotationPreviewDataCache(s);
    } else {
        makeAudioPreviewDataCache(s);
    }
}

void CompositionModelImpl::removePreviewCache(const Segment *s) 
{
    if (s->getType() == Rosegarden::Segment::Internal) {
        m_notationPreviewDataCache.remove(const_cast<Segment*>(s));
    } else {
        m_audioPreviewDataCache.remove(const_cast<Segment*>(s));
        m_audioSegmentPreviewMap.erase(s);
    }

}

void CompositionModelImpl::segmentAdded(const Composition *, Segment *s) 
{
    makePreviewCache(s);
    s->addObserver(this);
    emit needContentUpdate();
}

void CompositionModelImpl::segmentRemoved(const Composition *, Segment *s)
{
    QRect r = computeSegmentRect(*s);

    m_selectedSegments.erase(s);

    clearInCache(s, true);
    s->removeObserver(this);
    m_recordingSegments.erase(s); // this could be a recording segment
    emit needContentUpdate(r);
}

void CompositionModelImpl::segmentRepeatChanged(const Composition *, Segment *s, bool)
{
    clearInCache(s);
    emit needContentUpdate();
}

CompositionModel::rectlist* CompositionModelImpl::makeNotationPreviewDataCache(const Segment *s)
{
    rectlist* npData = new rectlist();
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

    QRect updateRect = m_selectionRect;

    for(Composition::segmentcontainer::iterator i = segments.begin();
        i != segEnd; ++i) {

        Segment* s = *i;
        CompositionRect sr = computeSegmentRect(*s);
        if (sr.intersects(m_selectionRect)) {
            m_tmpSelectedSegments.insert(s);
            updateRect |= sr;
        }
    }

    updateRect = updateRect.normalize();

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
    Composition::segmentcontainer::iterator segEnd = segments.end();

    for(Composition::segmentcontainer::iterator i = segments.begin();
        i != segEnd; ++i) {

        Segment* s = *i;
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
    emit needContentUpdate();

     RG_DEBUG << "CompositionModelImpl::addRecordingItem: now have "
 	     << m_recordingSegments.size() << " recording items\n";
}

void CompositionModelImpl::removeRecordingItem(const CompositionItem &item)
{
    Segment* s = CompositionItemHelper::getSegment(item);
    
    m_recordingSegments.erase(s);
    clearInCache(s, true);

    emit needContentUpdate();

     RG_DEBUG << "CompositionModelImpl::removeRecordingItem: now have "
 	     << m_recordingSegments.size() << " recording items\n";
}

void CompositionModelImpl::clearRecordingItems()
{
    for(recordingsegmentset::iterator i = m_recordingSegments.begin();
        i != m_recordingSegments.end(); ++i)
        clearInCache(*i, true);

    m_recordingSegments.clear();

    emit needContentUpdate();
     RG_DEBUG << "CompositionModelImpl::clearRecordingItem\n";
}

bool CompositionModelImpl::isMoving(const Segment* sm) const
{
    itemcontainer::const_iterator movEnd = m_changingItems.end();

    for(itemcontainer::const_iterator i = m_changingItems.begin(); i != movEnd; ++i) {
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

void CompositionModelImpl::setPointerPos(int xPos)
{
    m_pointerTimePos = grid().getRulerScale()->getTimeForX(xPos);

    for (recordingsegmentset::iterator i = m_recordingSegments.begin();
	 i != m_recordingSegments.end(); ++i) {
	emit needContentUpdate(computeSegmentRect(**i));
    }
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
    RG_DEBUG << "CompositionModelImpl::setSelected" << endl;
    if (selected) {
        if (!isSelected(segment))
            m_selectedSegments.insert(const_cast<Segment*>(segment));
    } else {
        Rosegarden::SegmentSelection::iterator i = m_selectedSegments.find(const_cast<Segment*>(segment));
        if (i != m_selectedSegments.end())
            m_selectedSegments.erase(i);
    }
    emit needContentUpdate();
}

void CompositionModelImpl::signalSelection()
{
//     RG_DEBUG << "CompositionModelImpl::signalSelection()\n";
    emit selectedSegments(getSelectedSegments());
}

void CompositionModelImpl::signalContentChange()
{
    RG_DEBUG << "CompositionModelImpl::signalContentChange" << endl;
    emit needContentUpdate();
}

void CompositionModelImpl::clearSelected()
{
    RG_DEBUG << "CompositionModelImpl::clearSelected" << endl;
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
        m_itemGC.push_back(item);
    } else {
        item->saveRect();
        m_changingItems.insert(item);
    }
}

void CompositionModelImpl::startChangeSelection(CompositionModel::ChangeType change)
{
    Rosegarden::SegmentSelection::iterator i = m_selectedSegments.begin();
    for(; i != m_selectedSegments.end(); ++i) {
        Segment* s = *i;
        CompositionRect sr = computeSegmentRect(*s);
        startChange(CompositionItem(new CompositionItemImpl(*s, sr)), change);
    }
    
}

void CompositionModelImpl::endChange()
{
    for(itemcontainer::const_iterator i = m_changingItems.begin(); i != m_changingItems.end(); ++i) {
        delete *i;
    }

    m_changingItems.clear();

    for(itemgc::iterator i = m_itemGC.begin(); i != m_itemGC.end(); ++i) {
        delete *i;
    }
    m_itemGC.clear();
    RG_DEBUG << "CompositionModelImpl::endChange" << endl;
    emit needContentUpdate();
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
//    Rosegarden::Profiler profiler("CompositionModelImpl::computeSegmentOrigin", true);

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

void CompositionModelImpl::clearInCache(const Rosegarden::Segment* s, bool clearPreview)
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

void CompositionModelImpl::putInCache(const Rosegarden::Segment*s, const CompositionRect& cr)
{
    m_segmentRectMap[s] = cr;
    m_segmentEndTimeMap[s] = s->getEndMarkerTime();
}

const CompositionRect& CompositionModelImpl::getFromCache(const Rosegarden::Segment* s, timeT& endTime)
{
    endTime = m_segmentEndTimeMap[s];
    return m_segmentRectMap[s];
}

CompositionRect CompositionModelImpl::computeSegmentRect(const Segment& s)
{
//    Rosegarden::Profiler profiler("CompositionModelImpl::computeSegmentRect", true);

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
    Rosegarden::timeT endTime   = isRecordingSegment ? m_pointerTimePos /*s.getEndTime()*/ : s.getEndMarkerTime();


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

    if (s.isRepeating()) {
        computeRepeatMarks(cr, &s);
    } else {
	cr.setBaseWidth(cr.width());
    }

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
    : RosegardenScrollView(parent, name, f | WNoAutoErase | WStaticContents),
#else
    : RosegardenScrollView(parent, name, f | WRepaintNoErase | WResizeNoErase | WStaticContents),
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
      m_segmentsDrawBuffer(visibleWidth(), visibleHeight()),
      m_artifactsDrawBuffer(visibleWidth(), visibleHeight()),
      m_segmentsDrawBufferRefresh(0, 0, visibleWidth(), visibleHeight()),
      m_artifactsDrawBufferRefresh(0, 0, visibleWidth(), visibleHeight()),
      m_lastBufferRefreshX(0),
      m_lastBufferRefreshY(0),
      m_lastPointerRefreshX(0)
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

//    connect(this, SIGNAL(contentsMoving(int, int)),
//            this, SLOT(slotAllDrawBuffersNeedRefresh()));

//     connect(this, SIGNAL(contentsMoving(int, int)),
//             this, SLOT(slotContentsMoving(int, int)));
    connect(model, SIGNAL(selectedSegments(const Rosegarden::SegmentSelection &)),
            this, SIGNAL(selectedSegments(const Rosegarden::SegmentSelection &)));

    connect(model, SIGNAL(needContentUpdate()),
            this, SLOT(slotUpdateSegmentsDrawBuffer()));
    connect(model, SIGNAL(needContentUpdate(const QRect&)),
            this, SLOT(slotUpdateSegmentsDrawBuffer(const QRect&)));
    connect(model, SIGNAL(needArtifactsUpdate()),
            this, SLOT(slotArtifactsDrawBufferNeedsRefresh()));

    connect(doc, SIGNAL(docColoursChanged()),
            this, SLOT(slotRefreshColourCache()));

    // recording-related signals
    connect(doc, SIGNAL(newMIDIRecordingSegment(Rosegarden::Segment*)),
            this, SLOT(slotNewMIDIRecordingSegment(Rosegarden::Segment*)));
    connect(doc, SIGNAL(newAudioRecordingSegment(Rosegarden::Segment*)),
            this, SLOT(slotNewAudioRecordingSegment(Rosegarden::Segment*)));
//     connect(doc, SIGNAL(recordMIDISegmentUpdated(Rosegarden::Segment*, Rosegarden::timeT)),
//             this, SLOT(slotRecordMIDISegmentUpdated(Rosegarden::Segment*, Rosegarden::timeT)));
    connect(doc, SIGNAL(stoppedAudioRecording()),
            this, SLOT(slotStoppedRecording()));
    connect(doc, SIGNAL(stoppedMIDIRecording()),
            this, SLOT(slotStoppedRecording()));
    connect(doc, SIGNAL(audioFileFinalized(Rosegarden::Segment*)),
            getModel(), SLOT(slotAudioFileFinalized(Rosegarden::Segment*)));

    CompositionModelImpl* cmi = dynamic_cast<CompositionModelImpl*>(model);
    if (cmi) {
        cmi->setAudioPreviewThread(&doc->getAudioPreviewThread());
    }

    doc->getAudioPreviewThread().setEmptyQueueListener(this);

    m_segmentsDrawBuffer.setOptimization(QPixmap::BestOptim);
    m_artifactsDrawBuffer.setOptimization(QPixmap::BestOptim);

    
}

void CompositionView::endAudioPreviewGeneration()
{
    CompositionModelImpl* cmi = dynamic_cast<CompositionModelImpl*>(m_model);
    if (cmi) {
        cmi->setAudioPreviewThread(0);
    }
}    

void CompositionView::setBackgroundPixmap(const QPixmap &m)
{
    m_backgroundPixmap = m;
//     viewport()->setErasePixmap(m_backgroundPixmap);
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
    m_selectionRect.setRect(pos.x(), pos.y(), 0, 0);
    getModel()->setSelectionRect(m_selectionRect);
}

void CompositionView::setSelectionRectSize(int w, int h)
{
    m_selectionRect.setSize(QSize(w, h));
    getModel()->setSelectionRect(m_selectionRect);
}

void CompositionView::setDrawSelectionRect(bool d)
{
    if (m_drawSelectionRect != d) {
        m_drawSelectionRect = d;
        slotArtifactsDrawBufferNeedsRefresh();
        slotUpdateSegmentsDrawBuffer(m_selectionRect);
    }
}


void CompositionView::clearSegmentRectsCache(bool clearPreviews)
{
    dynamic_cast<CompositionModelImpl*>(getModel())->clearSegmentRectsCache(clearPreviews);
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
//     qDebug("contents moving : x=%d", x);
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
    RG_DEBUG << "CompositionView::slotSelectSegments\n";

    static QRect dummy;

    getModel()->clearSelected();

    for(SegmentSelection::iterator i = segments.begin(); i != segments.end(); ++i) {
        getModel()->setSelected(CompositionItem(new CompositionItemImpl(**i, dummy)));
    }
    slotUpdateSegmentsDrawBuffer();
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

void CompositionView::slotUpdateSegmentsDrawBuffer()
{
//     RG_DEBUG << "CompositionView::slotUpdateSegmentsDrawBuffer()\n";
    slotAllDrawBuffersNeedRefresh();
    updateContents();
}

void CompositionView::slotUpdateSegmentsDrawBuffer(const QRect& rect)
{
//     RG_DEBUG << "CompositionView::slotUpdateSegmentsDrawBuffer() rect "
//              << rect << " - valid : " << rect.isValid() << endl;

    slotAllDrawBuffersNeedRefresh(rect);

    if (rect.isValid()) {
        updateContents(rect);
    } else {
        updateContents();
    }
}

void CompositionView::slotRefreshColourCache()
{
    CompositionColourCache::getInstance()->init();
    clearSegmentRectsCache();
    slotUpdateSegmentsDrawBuffer();
}

void CompositionView::slotNewMIDIRecordingSegment(Rosegarden::Segment* s)
{
    getModel()->addRecordingItem(CompositionItemHelper::makeCompositionItem(s));
}

void CompositionView::slotNewAudioRecordingSegment(Rosegarden::Segment* s)
{
    getModel()->addRecordingItem(CompositionItemHelper::makeCompositionItem(s));
}

// void CompositionView::slotRecordMIDISegmentUpdated(Rosegarden::Segment*, Rosegarden::timeT)
// {
//     RG_DEBUG << "CompositionView::slotRecordMIDISegmentUpdated()\n";
//     slotUpdateSegmentsDrawBuffer();
// }

void CompositionView::slotStoppedRecording()
{
    getModel()->clearRecordingItems();
}

/// update size of draw buffer
void CompositionView::resizeEvent(QResizeEvent* e)
{
    QScrollView::resizeEvent(e);
    int w = std::max(m_segmentsDrawBuffer.width(), visibleWidth());
    int h = std::max(m_segmentsDrawBuffer.height(), visibleHeight());
    
    m_segmentsDrawBuffer.resize(w, h);
    m_artifactsDrawBuffer.resize(w, h);
    slotAllDrawBuffersNeedRefresh();
//     RG_DEBUG << "CompositionView::resizeEvent() : drawBuffer size = " << m_segmentsDrawBuffer.size() << endl;
}

void CompositionView::viewportPaintEvent(QPaintEvent* e)
{
    QMemArray<QRect> rects = e->region().rects();

    for (int i = 0; i < rects.size(); ++i) {
        viewportPaintRect(rects[i]);
    }
}

void CompositionView::viewportPaintRect(QRect r)
{
    QRect updateRect = r;

    r &= viewport()->rect();
    r.moveBy(contentsX(), contentsY());

//     RG_DEBUG << "CompositionView::viewportPaintRect() r = " << r
//              << " - moveBy " << contentsX() << "," << contentsY() << " - updateRect = " << updateRect
//              << " - refresh " << m_segmentsDrawBufferRefresh << " artrefresh " << m_artifactsDrawBufferRefresh << endl;


    bool scroll = false;
    bool changed = checkScrollAndRefreshDrawBuffer(r, scroll);

    if (changed || m_artifactsDrawBufferRefresh.isValid()) {

	// r was modified by checkScrollAndRefreshDrawBuffer
	QRect copyRect(r | m_artifactsDrawBufferRefresh);
	copyRect.moveBy(-contentsX(), -contentsY());

//	RG_DEBUG << "copying from segment to artifacts buffer: " << copyRect << endl;

	bitBlt(&m_artifactsDrawBuffer,
	       copyRect.x(), copyRect.y(),
	       &m_segmentsDrawBuffer,
	       copyRect.x(), copyRect.y(), copyRect.width(), copyRect.height());
	m_artifactsDrawBufferRefresh |= r;
    }

    if (m_artifactsDrawBufferRefresh.isValid()) {
	refreshArtifactsDrawBuffer(m_artifactsDrawBufferRefresh);
	m_artifactsDrawBufferRefresh = QRect();
    }

    if (scroll) {
        bitBlt(viewport(), 0, 0,
               &m_artifactsDrawBuffer, 0, 0,
               m_artifactsDrawBuffer.width(), m_artifactsDrawBuffer.height());
    } else {
        bitBlt(viewport(), updateRect.x(), updateRect.y(),
               &m_artifactsDrawBuffer, updateRect.x(), updateRect.y(),
               updateRect.width(), updateRect.height());
    }

    // DEBUG

//     QPainter pdebug(viewport());
//     static QPen framePen(Qt::red, 1);
//     pdebug.setPen(framePen);
//     pdebug.drawRect(updateRect);

}

bool CompositionView::checkScrollAndRefreshDrawBuffer(QRect &rect, bool& scroll)
{
    bool all = false;
    QRect refreshRect = m_segmentsDrawBufferRefresh;
     
    int w = visibleWidth(), h = visibleHeight();
    int cx = contentsX(), cy = contentsY();

    scroll = (cx != m_lastBufferRefreshX || cy != m_lastBufferRefreshY);

    if (scroll) {

//	RG_DEBUG << "checkScrollAndRefreshDrawBuffer: scrolling by ("
//		 << cx - m_lastBufferRefreshX << "," << cy - m_lastBufferRefreshY << ")" << endl;

	if (refreshRect.isValid()) {

	    // If we've scrolled and there was an existing refresh
	    // rect, we can't be sure whether the refresh rect
	    // predated or postdated the internal update of scroll
	    // location.  Cut our losses and refresh everything.

	    refreshRect.setRect(cx, cy, w, h);

	} else {
	
	    // No existing refresh rect: we only need to handle the
	    // scroll

	    if (cx != m_lastBufferRefreshX) {

		int dx = m_lastBufferRefreshX - cx;
	    
		if (dx > -w && dx < w) {
		
		    QPainter cp(&m_segmentsDrawBuffer);
		    cp.drawPixmap(dx, 0, m_segmentsDrawBuffer);
		    
		    if (dx < 0) {
			refreshRect |= QRect(cx + w + dx, cy, -dx, h);
		    } else {
			refreshRect |= QRect(cx, cy, dx, h);
		    }
		    
		} else {
		    
		    refreshRect.setRect(cx, cy, w, h);
		    all = true;
		}
	    }
	    
	    if (cy != m_lastBufferRefreshY && !all) {
		
		int dy = m_lastBufferRefreshY - cy;
		
		if (dy > -h && dy < h) {
		    
		    QPainter cp(&m_segmentsDrawBuffer);
		    cp.drawPixmap(0, dy, m_segmentsDrawBuffer);
		    
		    if (dy < 0) {
			refreshRect |= QRect(cx, cy + h + dy, w, -dy);
		    } else {
			refreshRect |= QRect(cx, cy, w, dy);
		    }
		    
		} else {
		    
		    refreshRect.setRect(cx, cy, w, h);
		    all = true;
		}
	    }
	}
    }

    bool needRefresh = false;

    if (refreshRect.isValid()) {
	needRefresh = true;
    }

    if (needRefresh) refreshSegmentsDrawBuffer(refreshRect);

    m_segmentsDrawBufferRefresh = QRect();
    m_lastBufferRefreshX = cx;
    m_lastBufferRefreshY = cy;

    rect |= refreshRect;
    if (scroll) rect.setRect(cx, cy, w, h);
    return needRefresh;
}

void CompositionView::refreshSegmentsDrawBuffer(const QRect& rect)
{
//    Rosegarden::Profiler profiler("CompositionView::refreshDrawBuffer", true);
//      RG_DEBUG << "CompositionView::refreshSegmentsDrawBuffer() r = "
//  	     << rect << endl;

    QPainter p(&m_segmentsDrawBuffer, viewport());
    p.translate(-contentsX(), -contentsY());

    if (!m_backgroundPixmap.isNull()) {
        QPoint pp(rect.x() % m_backgroundPixmap.height(), rect.y() % m_backgroundPixmap.width());
	p.drawTiledPixmap(rect, m_backgroundPixmap, pp);
    } else {
        p.eraseRect(rect);
    }

    drawArea(&p, rect);

    // DEBUG - show what's updated
//    QPen framePen(Qt::red, 1);
//    p.setPen(framePen);
//    p.drawRect(rect);
    
//    m_segmentsDrawBufferNeedsRefresh = false;
}

void CompositionView::refreshArtifactsDrawBuffer(const QRect& rect)
{
//      RG_DEBUG << "CompositionView::refreshArtifactsDrawBuffer() r = "
//               << rect << endl;

    QPainter p;
    p.begin(&m_artifactsDrawBuffer, viewport());
    p.translate(-contentsX(), -contentsY());
//     QRect r(contentsX(), contentsY(), m_artifactsDrawBuffer.width(), m_artifactsDrawBuffer.height());
    drawAreaArtifacts(&p, rect);
    p.end();

//    m_artifactsDrawBufferNeedsRefresh = false;
}


void CompositionView::drawArea(QPainter *p, const QRect& clipRect)
{
//     Rosegarden::Profiler profiler("CompositionView::drawArea", true);

//     RG_DEBUG << "CompositionView::drawArea() clipRect = " << clipRect << endl;

    CompositionModel::AudioPreviewDrawData* audioPreviewData = 0;
    CompositionModel::RectRanges* notationPreviewData = 0;

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

        // draw audio previews
        //
        drawAreaAudioPreviews(p, clipRect);
        
        // draw notation previews
        //
        CompositionModel::RectRanges::const_iterator npi = m_notationPreviewRects.begin();
        CompositionModel::RectRanges::const_iterator npEnd = m_notationPreviewRects.end();
        
        for(; npi != npEnd; ++npi) {
            CompositionModel::RectRange interval = *npi;
            p->save();
            p->translate(interval.basePoint.x(), interval.basePoint.y());
//             RG_DEBUG << "CompositionView::drawArea : translating to x = " << interval.basePoint.x() << endl;
            for(; interval.range.first != interval.range.second; ++interval.range.first) {

                const PreviewRect& pr = *(interval.range.first);
                QColor defaultCol = CompositionColourCache::getInstance()->SegmentInternalPreview;
                QColor col = interval.color.isValid() ? interval.color : defaultCol;
                p->setBrush(col);
                p->setPen(col);
//                 RG_DEBUG << "CompositionView::drawArea : drawing rect at x = " << pr.x() << endl;
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

//    drawAreaArtifacts(p, clipRect);
    
}

void CompositionView::drawAreaAudioPreviews(QPainter * p, const QRect& clipRect)
{
    CompositionModel::AudioPreviewDrawData::const_iterator api = m_audioPreviewRects.begin();
    CompositionModel::AudioPreviewDrawData::const_iterator apEnd = m_audioPreviewRects.end();
    QRect rectToFill, // rect to fill on canvas
        localRect; // the rect of the tile to draw on the canvas
    QPoint basePoint, // origin of segment rect
        drawBasePoint; // origin of rect to fill on canvas
    QRect r;
    for(; api != apEnd; ++api) {
        rectToFill = api->rect;
        basePoint = api->basePoint;
        rectToFill.moveTopLeft(basePoint);
        rectToFill &= clipRect;
        r = rectToFill;
        drawBasePoint = rectToFill.topLeft();
        rectToFill.moveBy(-basePoint.x(), -basePoint.y());
        int firstPixmapIdx = (r.x() - basePoint.x()) / AudioPreviewPainter::tileWidth();
        if (firstPixmapIdx >= api->pixmap.size()) {
//             RG_DEBUG << "CompositionView::drawAreaAudioPreviews : WARNING - miscomputed pixmap array : r.x = "
//                      << r.x() << " - basePoint.x = " << basePoint.x() << " - firstPixmapIdx = " << firstPixmapIdx
//                      << endl;
            continue;
        }
        int x = 0, idx = firstPixmapIdx;
//         RG_DEBUG << "CompositionView::drawAreaAudioPreviews : clipRect = " << clipRect
//                  << " - firstPixmapIdx = " << firstPixmapIdx << endl;
        while(x < clipRect.width()) {
            int pixmapRectXOffset = idx * AudioPreviewPainter::tileWidth();
            localRect.setRect(basePoint.x() + pixmapRectXOffset, basePoint.y(),
                              AudioPreviewPainter::tileWidth(), api->rect.height());
//             RG_DEBUG << "CompositionView::drawAreaAudioPreviews : initial localRect = "
//                      << localRect << endl;
            localRect &= clipRect;
//             RG_DEBUG << "CompositionView::drawAreaAudioPreviews : localRect & clipRect = "
//                      << localRect << endl;
            if (localRect.isEmpty()) {
//                 RG_DEBUG << "CompositionView::drawAreaAudioPreviews : localRect & clipRect is empty\n";
                break;
            }
            localRect.moveBy(-(basePoint.x() + pixmapRectXOffset), -basePoint.y());

            RG_DEBUG << "CompositionView::drawAreaAudioPreviews : drawing pixmap "
                     << idx << " at " << drawBasePoint << " - localRect = " << localRect << endl;

            p->drawImage(drawBasePoint, api->pixmap[idx], localRect,
			 Qt::ColorOnly | Qt::ThresholdDither | Qt::AvoidDither);

            ++idx;
            if (idx >= api->pixmap.size())
                break;
            drawBasePoint.setX(drawBasePoint.x() + localRect.width());
            x += localRect.width();
        }
    }
}

void CompositionView::drawAreaArtifacts(QPainter * p, const QRect& clipRect)
{
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
//     RG_DEBUG << "CompositionView::drawPointer: clipRect "
// 	     << clipRect.x() << "," << clipRect.y() << " " << clipRect.width()
// 	     << "x" << clipRect.height() << " pointer pos is " << m_pointerPos << endl;

    if (m_pointerPos >= clipRect.x() && m_pointerPos <= (clipRect.x() + clipRect.width())) {
        p->save();
        p->setPen(m_pointerPen);
        p->drawLine(m_pointerPos, clipRect.y(), m_pointerPos, clipRect.y() + clipRect.height());
        p->restore();
    }
    
}

void CompositionView::drawTextFloat(QPainter *p, const QRect& clipRect)
{
    QFontMetrics metrics(p->fontMetrics());

    QRect bound = p->boundingRect(0, 0, 300, metrics.height() + 6, AlignAuto, m_textFloatText);

    p->save();

    bound.setLeft(bound.left() - 2);
    bound.setRight(bound.right() + 2);
    bound.setTop(bound.top() - 2);
    bound.setBottom(bound.bottom() + 2);

    QPoint pos(m_textFloatPos);
    if (pos.y() < 0 && getModel()) {
	if (pos.y() + bound.height() < 0) {
	    pos.setY(pos.y() + getModel()->grid().getYSnap() * 3);
	} else {
	    pos.setY(pos.y() + getModel()->grid().getYSnap() * 2);
	}
    }

    bound.moveTopLeft(pos);

    if (bound.intersects(clipRect)) {
 
	p->setBrush(CompositionColourCache::getInstance()->RotaryFloatBackground);

        drawRect(bound, p, clipRect, false, 0, true);

        p->setPen(CompositionColourCache::getInstance()->RotaryFloatForeground);

        p->drawText(pos.x() + 2, pos.y() + 3 + metrics.ascent(), m_textFloatText);

    }
    
    p->restore();
}

bool CompositionView::event(QEvent* e)
{
    if (e->type() == AudioPreviewThread::AudioPreviewQueueEmpty) {
        RG_DEBUG << "CompositionView::event - AudioPreviewQueueEmpty\n";
        slotSegmentsDrawBufferNeedsRefresh();
        viewport()->update();
        return true;
    }

    return RosegardenScrollView::event(e);
}

void CompositionView::contentsMousePressEvent(QMouseEvent* e)
{
    Qt::ButtonState bs = e->state();
    RosegardenGUIApp::self()->slotUpdateKeyModifiers
	(bs & Qt::ShiftButton, bs & Qt::ControlButton);

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
//     RG_DEBUG << "CompositionView::setPointerPos(" << pos << ")\n";
    int oldPos = m_pointerPos;
    if (oldPos == pos) return;

    m_pointerPos = pos;
    getModel()->setPointerPos(pos);

    // interesting -- isAutoScrolling() never seems to return true?
//     RG_DEBUG << "CompositionView::setPointerPos(" << pos << "), isAutoScrolling " << isAutoScrolling() << ", contentsX " << contentsX() << ", m_lastPointerRefreshX " << m_lastPointerRefreshX << ", contentsHeight " << contentsHeight() << endl;
    
    if (contentsX() != m_lastPointerRefreshX) {
	m_lastPointerRefreshX = contentsX();
	// We'll need to shift the whole canvas anyway, so
	slotArtifactsDrawBufferNeedsRefresh();
	return;
    }

    int deltaW = abs(m_pointerPos - oldPos);

    if (deltaW <= m_pointerPen.width() * 2) { // use one rect instead of two separate ones

	QRect updateRect
	    (std::min(m_pointerPos, oldPos) - m_pointerPen.width(), 0,
	     deltaW + m_pointerPen.width() * 2, contentsHeight());

	slotArtifactsDrawBufferNeedsRefresh(updateRect);

    } else {

	slotArtifactsDrawBufferNeedsRefresh
	    (QRect(m_pointerPos - m_pointerPen.width(), 0,
		   m_pointerPen.width() * 2, contentsHeight()));

	slotArtifactsDrawBufferNeedsRefresh
	    (QRect(oldPos - m_pointerPen.width(), 0,
		   m_pointerPen.width() * 2, contentsHeight()));
    }
}

void CompositionView::setGuidesPos(int x, int y)
{
    m_topGuidePos = x; m_foreGuidePos = y;
    slotArtifactsDrawBufferNeedsRefresh();
}

void CompositionView::setGuidesPos(const QPoint& p)
{
    m_topGuidePos = p.x(); m_foreGuidePos = p.y();
    slotArtifactsDrawBufferNeedsRefresh();
}

void CompositionView::setDrawGuides(bool d)
{
    m_drawGuides = d;
    slotArtifactsDrawBufferNeedsRefresh();
}

void CompositionView::setTmpRect(const QRect& r)
{
    QRect pRect = m_tmpRect;
    m_tmpRect = r;
    slotUpdateSegmentsDrawBuffer(m_tmpRect | pRect);
}

void CompositionView::setTextFloat(int x, int y, const QString &text)
{
    m_textFloatPos.setX(x);
    m_textFloatPos.setY(y);
    m_textFloatText = text;
    m_drawTextFloat = true;
    slotArtifactsDrawBufferNeedsRefresh();

    // most of the time when the floating text is drawn
    // we want to update a larger part of the view
    // so don't update here
//     QRect r = fontMetrics().boundingRect(x, y, 300, 40, AlignAuto, m_textFloatText);
//     slotUpdateSegmentsDrawBuffer(r);

    
//    rgapp->slotSetStatusMessage(text);
}

void CompositionView::slotSetFineGrain(bool value)
{
    m_fineGrain = value;
}

void 
CompositionView::slotTextFloatTimeout() 
{ 
    hideTextFloat();
    slotArtifactsDrawBufferNeedsRefresh();
//    rgapp->slotSetStatusMessage(QString::null);
}

#include "compositionview.moc"
