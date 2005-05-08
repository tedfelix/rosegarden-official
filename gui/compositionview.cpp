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

#include "BaseProperties.h"
#include "SnapGrid.h"
#include "Composition.h"
#include "RulerScale.h"

#include "compositionview.h"
#include "compositionitemhelper.h"
#include "colours.h"
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
	t = std::max(grid.snapX(item->rect().x()) - 1, 0L);
    }

    return t;
}

timeT CompositionItemHelper::getEndTime(const CompositionItem& item, const Rosegarden::SnapGrid& grid)
{
    timeT t = 0;

    if (item) {
        QRect itemRect = item->rect();
        
        RG_DEBUG << "CompositionItemHelper::getEndTime() : rect width = "
                 << itemRect.width() << endl;

        t = std::max(grid.snapX(itemRect.x() + itemRect.width()) - 1, 0L);
    }

    return t;
}

void CompositionItemHelper::setStartTime(CompositionItem& item, timeT time,
                                         const Rosegarden::SnapGrid& grid)
{
    if (item) {
        int x = int(nearbyint(grid.getRulerScale()->getXForTime(time)));
        item->setX(x);
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
CompositionModelImpl::CompositionModelImpl(Rosegarden::Composition& compo,
                                           Rosegarden::RulerScale *rulerScale,
                                           int vStep)
    : m_composition(compo),
      m_grid(rulerScale, vStep),
      m_recordingSegment(0)
{
    m_notationPreviewDataCache.setAutoDelete(true);
    m_audioPreviewDataCache.setAutoDelete(true);
    refreshAllPreviews();
    m_composition.addObserver(this);

    const Rosegarden::Composition::segmentcontainer& segments = m_composition.getSegments();
    Rosegarden::Composition::segmentcontainer::iterator segEnd = segments.end();

    for(Rosegarden::Composition::segmentcontainer::iterator i = segments.begin();
        i != segEnd; ++i) {

        (*i)->addObserver(this);
    }

}

CompositionModelImpl::~CompositionModelImpl()
{
    m_composition.removeObserver(this);

    const Rosegarden::Composition::segmentcontainer& segments = m_composition.getSegments();
    Rosegarden::Composition::segmentcontainer::iterator segEnd = segments.end();

    for(Rosegarden::Composition::segmentcontainer::iterator i = segments.begin();
        i != segEnd; ++i) {

        (*i)->removeObserver(this);
    }
}

unsigned int CompositionModelImpl::getNbRows()
{
    return m_composition.getNbTracks();
}

const CompositionModel::rectcontainer& CompositionModelImpl::getRectanglesIn(const QRect& rect,
                                                                             NotationPreviewData* npData,
                                                                             AudioPreviewData* apData)
{
    m_res.clear();

//     RG_DEBUG << "CompositionModelImpl::getRectanglesIn: ruler scale is "
// 	     << (dynamic_cast<Rosegarden::SimpleRulerScale *>(m_grid.getRulerScale()))->getUnitsPerPixel() << endl;

    const Rosegarden::Composition::segmentcontainer& segments = m_composition.getSegments();
    Rosegarden::Composition::segmentcontainer::iterator segEnd = segments.end();

    for(Rosegarden::Composition::segmentcontainer::iterator i = segments.begin();
        i != segEnd; ++i) {

        Segment* s = *i;
        CompositionRect sr = computeSegmentRect(*s, m_composition, m_grid);
        if (sr.intersects(rect) && !isMoving(s)) {
            bool tmpSelected = isTmpSelected(s),
                pTmpSelected = wasTmpSelected(s);

            if (isSelected(s) || isTmpSelected(s) || sr.intersects(m_selectionRect)) {
                sr.setSelected(true);
            }
            
            if (pTmpSelected != tmpSelected)
                sr.setNeedsFullUpdate(true);

            if (s->isRepeating())
                computeRepeatMarks(sr, s);

            if (s != m_recordingSegment) {
                QColor brushColor = GUIPalette::convertColour(m_composition.getSegmentColourMap().getColourByIndex(s->getColourIndex()));
                sr.setBrush(brushColor);
           	sr.setPen(GUIPalette::getColour(GUIPalette::SegmentBorder));
            } else {
           	sr.setPen(GUIPalette::getColour(GUIPalette::RecordingSegmentBorder));
                sr.setBrush(GUIPalette::getColour(GUIPalette::RecordingInternalSegmentBlock));
            }

            // Notation preview data
            if (npData && s->getType() == Rosegarden::Segment::Internal) {
                NotationPreviewData* cachedNPData = getNotationPreviewData(s);

//                 RG_DEBUG << "CompositionModelImpl::getRectanglesIn() : npData = "
//                          << npData << " - rect = " << rect << endl;

                NotationPreviewData::iterator npi = cachedNPData->lower_bound(rect),
                    npEnd = cachedNPData->end();
                if (npi != cachedNPData->begin())
                    --npi;

                int xLim = rect.topRight().x();
                for(; npi->x() <= xLim && npi != npEnd; ++npi) {
//                     RG_DEBUG << "CompositionModelImpl::getRectanglesIn : xLim = " << xLim
//                              << " - npi = " << (*npi) << endl;
                    QRect tr = *npi;

                    // put preview rectangle inside the corresponding segment's CompositionRect
		    //               tr.moveBy(sr.x(), sr.y());
		    // already has correct x-coord virtue of snap grid
		    tr.moveBy(0, sr.y());

//                     RG_DEBUG << "CompositionModelImpl::getRectanglesIn : inserting preview rect "
//                              << tr << endl;
                    npData->insert(tr);
                }
                

            // Audio preview data
            } else if (apData && s->getType() == Rosegarden::Segment::Audio) {
                // TODO
            }
            
            
            m_res.push_back(sr);
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

            sr.setPen(GUIPalette::getColour(GUIPalette::SegmentBorder));
            
            m_res.push_back(sr);
        }
    }

    return m_res;
}

void CompositionModelImpl::computeRepeatMarks(CompositionRect& sr, const Segment* s)
{
    if (s->isRepeating()) { // this works only at the creation of the CompositionRect - need a way to refresh those marks when segment is moved along X

        timeT startTime = s->getStartTime();
        timeT endTime = s->getEndMarkerTime();
        timeT repeatInterval = endTime - startTime;
        timeT repeatStart = endTime;
        timeT repeatEnd = s->getRepeatEndTime();
        sr.setWidth(int(nearbyint(m_grid.getRulerScale()->getWidthForDuration(repeatStart,
                                                                              repeatEnd - repeatStart))));

        CompositionRect::repeatmarks repeatMarks;
                
        for(timeT repeatMark = repeatStart; repeatMark < repeatEnd; repeatMark += repeatInterval) {
            repeatMarks.push_back(int(nearbyint(m_grid.getRulerScale()->getXForTime(repeatMark))));
        }
        sr.setRepeatMarks(repeatMarks);
        sr.setBaseWidth(repeatMarks[0] - sr.x());
    }
}

void CompositionModelImpl::refreshAllPreviews()
{
    clearPreviewCache();

    const Rosegarden::Composition::segmentcontainer& segments = m_composition.getSegments();
    Rosegarden::Composition::segmentcontainer::iterator segEnd = segments.end();

    for(Rosegarden::Composition::segmentcontainer::iterator i = segments.begin();
        i != segEnd; ++i) {

        makePreviewCache(*i);
    }
    
}

void CompositionModelImpl::clearPreviewCache()
{
    m_notationPreviewDataCache.clear();
    m_audioPreviewDataCache.clear();
}

void CompositionModelImpl::updatePreviewCacheForNotationSegment(const Segment* segment, NotationPreviewData* npData)
{
    npData->clear();
    
    int segStartX = int(nearbyint(m_grid.getRulerScale()->getXForTime(segment->getStartTime())));
    int segEndX = int(nearbyint(m_grid.getRulerScale()->getXForTime(segment->getEndMarkerTime())));
						       
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

        double y0 = 0;
        double y1 = m_grid.getYSnap();
        double y = y1 + ((y0 - y1) * (pitch-16)) / 96;
        if (y < y0) y = y0;
        if (y > y1-1) y = y1-1;

        QRect r(x, (int)y, width, 2);
//         RG_DEBUG << "CompositionModelImpl::updatePreviewCacheForNotationSegment() : npData = "
//                  << npData << ", preview rect = "
//                  << r << endl;
        npData->insert(r);
    }

}

void CompositionModelImpl::updatePreviewCacheForAudioSegment(const Segment* segment, AudioPreviewData* apData)
{
    // TODO
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
    std::set<const Rosegarden::Segment*>::iterator i = m_dirtySegments.begin();
    std::set<const Rosegarden::Segment*>::iterator dirtySegmentsEnd = m_dirtySegments.end();
    
    for (;i != dirtySegmentsEnd; ++i) {
        const Segment* s = *i;

        if (s->getType() == Rosegarden::Segment::Audio) {
            AudioPreviewData* apData = getAudioPreviewData(s);
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
    removePreviewCache(s);
    s->removeObserver(this);
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
    AudioPreviewData* apData = new AudioPreviewData();
    updatePreviewCacheForAudioSegment(s, apData);
    m_audioPreviewDataCache.insert(const_cast<Segment*>(s), apData);

    return apData;
}

void CompositionModelImpl::setSelectionRect(const QRect& r)
{
    m_selectionRect = r.normalize();

    m_previousTmpSelectedSegments = m_tmpSelectedSegments;
    m_tmpSelectedSegments.clear();
    
    const Rosegarden::Composition::segmentcontainer& segments = m_composition.getSegments();
    Rosegarden::Composition::segmentcontainer::iterator segEnd = segments.end();

    for(Rosegarden::Composition::segmentcontainer::iterator i = segments.begin();
        i != segEnd; ++i) {

        Segment* s = *i;
        CompositionRect sr = computeSegmentRect(*s, m_composition, m_grid);
        if (sr.intersects(m_selectionRect)) {
            m_tmpSelectedSegments.insert(s);
        }
    }
}

void CompositionModelImpl::finalizeSelectionRect()
{
    const Rosegarden::Composition::segmentcontainer& segments = m_composition.getSegments();
    Rosegarden::Composition::segmentcontainer::iterator segEnd = segments.end();

    for(Rosegarden::Composition::segmentcontainer::iterator i = segments.begin();
        i != segEnd; ++i) {

        Segment* s = *i;
        CompositionRect sr = computeSegmentRect(*s, m_composition, m_grid);
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
        CompositionRect sr = computeSegmentRect(*s, m_composition, m_grid);
        selectionRect |= sr;
    }

    return selectionRect;
}

void CompositionModelImpl::setRecordingItem(const CompositionItem& item)
{
    m_recordingSegment = CompositionItemHelper::getSegment(item);
}

void CompositionModelImpl::clearRecordingItem()
{
    m_recordingSegment = 0;
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

CompositionModel::itemcontainer CompositionModelImpl::getItemsAt(const QPoint& point)
{
    itemcontainer res;

    const Rosegarden::Composition::segmentcontainer& segments = m_composition.getSegments();

    for(Rosegarden::Composition::segmentcontainer::iterator i = segments.begin();
        i != segments.end(); ++i) {

        Segment* s = *i;
        CompositionRect sr = computeSegmentRect(*s, m_composition, m_grid);
        if (sr.contains(point)) {
//             RG_DEBUG << "CompositionModelImpl::getItemsAt() adding " << sr << endl;
            computeRepeatMarks(sr, s);
            res.push_back(CompositionItem(new CompositionItemImpl(*s, sr)));
        } else {
//             RG_DEBUG << "CompositionModelImpl::getItemsAt() skiping " << sr << endl;
        }
        
    }
    std::sort(res.begin(), res.end());
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
    item->saveRect();
    m_movingItems.push_back(item);
}

void CompositionModelImpl::startMoveSelection()
{
    Rosegarden::SegmentSelection::iterator i = m_selectedSegments.begin();
    for(; i != m_selectedSegments.end(); ++i) {
        Segment* s = *i;
        CompositionRect sr = computeSegmentRect(*s, m_composition, m_grid);
        startMove(CompositionItem(new CompositionItemImpl(*s, sr)));
    }
    
}

void CompositionModelImpl::endMove()
{
    for(itemcontainer::const_iterator i = m_movingItems.begin(); i != m_movingItems.end(); ++i) {
        delete *i;
    }

    m_movingItems.clear();
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

    return /*s->getEndMarkerTime() + */(count * (s->getEndMarkerTime() - s->getStartTime()));
}


CompositionRect CompositionModelImpl::computeSegmentRect(const Segment& s,
                                                         const Rosegarden::Composition& comp,
                                                         const Rosegarden::SnapGrid& grid)
{
    int trackPosition = comp.getTrackById(s.getTrack())->getPosition();
    Rosegarden::timeT startTime = s.getStartTime();
    Rosegarden::timeT endTime   = s.getEndMarkerTime();

    int x = int(nearbyint(grid.getRulerScale()->getXForTime(startTime)));
    int y = grid.getYBinCoordinate(trackPosition);
    int h = grid.getYSnap();
    int w;
    if (s.isRepeating()) {
        timeT repeatStart = endTime;
        timeT repeatEnd   = s.getRepeatEndTime();
        w = int(nearbyint(grid.getRulerScale()->getWidthForDuration(repeatStart,
                                                      repeatEnd - repeatStart)));
//         RG_DEBUG << "CompositionModelImpl::computeSegmentRect : s is repeating - repeatStart = "
//                  << repeatStart << " - repeatEnd : " << repeatEnd
//                  << " w = " << w << endl;
    } else {
        w = int(nearbyint(grid.getRulerScale()->getWidthForDuration(startTime, endTime - startTime)));
//         RG_DEBUG << "CompositionModelImpl::computeSegmentRect : s is NOT repeating"
//                  << " w = " << w << endl;
    }

    CompositionRect cr(x, y, w, h);
    cr.setLabel(strtoqstr(s.getLabel()));

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

void CompositionItemImpl::refreshRepeatMarks(int newX, int newWidth)
{
    int deltaX = 0, deltaW = 0;
    
    if (newX >= 0) {
        deltaX = newX - m_rect.x();
    }
    if (newWidth >= 0) {
        deltaW = newWidth - m_rect.width();
    }
    
    
    CompositionRect::repeatmarks repeatMarks = m_rect.getRepeatMarks();
    for(int i = 0; i < repeatMarks.size(); ++i) {
        repeatMarks[i] += deltaX;
    }

    m_rect.setRepeatMarks(repeatMarks);
    
}


//
// CompositionView
//
CompositionView::CompositionView(RosegardenGUIDoc* doc,
                                 CompositionModel* model,
                                 QWidget * parent, const char * name, WFlags f)
    : RosegardenScrollView(parent, name, f | WResizeNoErase|WStaticContents),
      m_model(model),
      m_currentItem(0),
      m_tool(0),
      m_toolBox(0),
      m_showPreviews(false),
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
      m_2ndLevelUpdate(false)
{
    m_toolBox = new SegmentToolBox(this, doc);

    setDragAutoScroll(true);
//     viewport()->setBackgroundMode(PaletteBase);
    viewport()->setPaletteBackgroundColor(GUIPalette::getColour(GUIPalette::SegmentCanvas));

    updateSize();
    
    QScrollBar* hsb = horizontalScrollBar();

    connect(hsb, SIGNAL(nextLine()),
            this, SLOT(scrollRight()));
    connect(hsb, SIGNAL(prevLine()),
            this, SLOT(scrollLeft()));
//     connect(this, SIGNAL(contentsMoving(int, int)),
//             this, SLOT(slotContentsMoving(int, int)));
    connect(model, SIGNAL(selectedSegments(const Rosegarden::SegmentSelection &)),
            this, SIGNAL(selectedSegments(const Rosegarden::SegmentSelection &)));
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
    
    Rosegarden::Composition &comp = dynamic_cast<CompositionModelImpl*>(getModel())->getComposition();
    
    Rosegarden::RulerScale *ruler = grid().getRulerScale();
    int width = int(nearbyint(ruler->getTotalWidth()));

    if (!shrinkWidth && width < contentsWidth())
        width = contentsWidth();

    resizeContents(width, height);
}

void CompositionView::scrollRight()
{
    if (m_stepSize == 0) initStepSize();

    if (horizontalScrollBar()->value() == horizontalScrollBar()->maxValue()) {

        resizeContents(contentsWidth() + m_stepSize, contentsHeight());
        setContentsPos(contentsX() + m_stepSize, contentsY());
        getModel()->setLength(contentsWidth());
    }
    
}

void CompositionView::scrollLeft()
{
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

void CompositionView::refreshAllPreviews()
{
    dynamic_cast<CompositionModelImpl*>(getModel())->refreshAllPreviews();
    dynamic_cast<CompositionModelImpl*>(getModel())->clearDirtyPreviews();
    
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
    static QRect dummy;

    getModel()->clearSelected();

    for(SegmentSelection::iterator i = segments.begin(); i != segments.end(); ++i) {
        getModel()->setSelected(CompositionItem(new CompositionItemImpl(**i, dummy)));
    }
    updateContents();
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
        CompositionItem res = items.front();

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
    refreshDirtyPreviews();
    viewport()->update();
}


void CompositionView::drawContents(QPainter *p, int clipx, int clipy, int clipw, int cliph)
{
    //!!!   QScrollView::drawContents(p, clipx, clipy, clipw, cliph);

    QRect clipRect(clipx, clipy, clipw, cliph);

//     RG_DEBUG << "CompositionView::drawContents() clipRect = " << clipRect << endl;

    CompositionModel::AudioPreviewData*    audioPreviewData = 0;
    CompositionModel::NotationPreviewData* notationPreviewData = 0;

    //
    // Fetch previews
    //
    if (m_showPreviews) {
        notationPreviewData = &m_notationPreviewData;
        m_notationPreviewData.clear();
        audioPreviewData = &m_audioPreviewData;
        m_audioPreviewData.clear();
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
        if (m_drawSelectionRect && i->needsFullUpdate() && !m_2ndLevelUpdate) {
            m_2ndLevelUpdate = true;
            updateContents(*i);
        } else {
            m_2ndLevelUpdate = false;
        }
        
//         RG_DEBUG << "CompositionView::drawContents : draw comp rect " << *i << endl;
        drawCompRect(*i, p, clipRect);
    }
    
    p->restore();
    
    if (rects.size() > 1) {
//         RG_DEBUG << "CompositionView::drawContents : drawing intersections\n";
        drawIntersections(rects, p, clipRect);
    }

    //
    // Previews
    //
    if (m_showPreviews) {
        p->save();
        refreshDirtyPreviews();
        p->setRasterOp(Qt::XorROP);
        
        CompositionModel::AudioPreviewData::const_iterator api = m_audioPreviewData.begin();
        CompositionModel::AudioPreviewData::const_iterator apEnd = m_audioPreviewData.end();
        
        for(; api != apEnd; ++api) {
            // TODO
        }

        CompositionModel::NotationPreviewData::const_iterator npi = m_notationPreviewData.begin();
        CompositionModel::NotationPreviewData::const_iterator npEnd = m_notationPreviewData.end();
        
        for(; npi != npEnd; ++npi) {
//             RG_DEBUG << "CompositionView::drawContents : draw preview rect " << *npi << endl;
            p->drawRect(*npi);
        }
        
        p->restore();
    }

    //
    // Draw segment labels (they must be drawn over the preview rects)
    //
    
    for(i = rects.begin(); i != end; ++i) {
        drawCompRectLabel(*i, p, clipRect);
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
        p->setPen(GUIPalette::getColour(GUIPalette::SegmentBorder));
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

        // draw 'start' rectangle with original brush
        //
        QRect startRect = r;
        startRect.setWidth(repeatMarks[0] - r.x());
        p->setBrush(r.getBrush());
        drawRect(startRect, p, clipRect, r.isSelected(), intersectLvl, fill);
        

        // now draw the 'repeat' marks
        //
        p->setPen(GUIPalette::getColour(GUIPalette::RepeatSegmentBorder));
        int penWidth = std::max(r.getPen().width(), 1u);

        for (unsigned int i = 0; i < repeatMarks.size(); ++i) {
            int pos = repeatMarks[i];
            if (pos > clipRect.right())
                break;
            
            if (pos >= clipRect.left()) {
//                 RG_DEBUG << "CompositionView::drawCompRect() : drawing repeat mark at "
//                          << pos << "," << r.y() + penWidth << endl;
                p->drawLine(pos, r.y() + penWidth, pos,
                            r.y() + r.height() - penWidth - 1);
            }
            
        }

    }

    p->restore();
}

void CompositionView::drawCompRectLabel(const CompositionRect& r, QPainter *p, const QRect& clipRect)
{
    // draw segment label
    //
    if (!r.getLabel().isEmpty()) {
        p->save();
        p->setPen(Rosegarden::GUIPalette::getColour(Rosegarden::GUIPalette::SegmentLabel));
        QRect textRect(r);
        textRect.setX(textRect.x() + 3);
        QRect textBoundingRect = p->boundingRect(textRect, Qt::AlignLeft|Qt::AlignVCenter, r.getLabel());
        p->fillRect(textBoundingRect & r, white);
        p->drawText(textRect, Qt::AlignLeft|Qt::AlignVCenter, r.getLabel());
        p->restore();
    }
    
}


void CompositionView::drawRect(const QRect& r, QPainter *p, const QRect& clipRect,
                               bool isSelected, int intersectLvl, bool fill)
{
//     RG_DEBUG << "CompositionView::drawRect : intersectLvl = " << intersectLvl
//              << " - brush col = " << p->brush().color() << endl;
    
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
            p->drawLine(rect.topLeft(), rect.topRight());
        }

        int rectBottomY = rect.y() + rect.height();
        if (rectBottomY >= clipRect.y() &&
            rectBottomY <= (clipRect.y() + clipRect.height()))
            p->drawLine(rect.bottomLeft(), rect.bottomRight());

        int rectLeftX = rect.x();
        if (rectLeftX >= clipRect.x() &&
            rectLeftX <= (clipRect.x() + clipRect.width()))
            p->drawLine(rect.topLeft(), rect.bottomLeft());

        int rectRightX = rect.x() + rect.width();
        if (rectRightX >= clipRect.x() &&
            rectRightX <= (clipRect.x() + clipRect.width()))
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

    p->setPen(GUIPalette::getColour(GUIPalette::RotaryFloatForeground));
    p->drawText(m_textFloatPos.x() + 2, m_textFloatPos.y() + 14, m_textFloatText);

    p->restore();
}

void CompositionView::contentsMousePressEvent(QMouseEvent* e)
{
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

    return;
}

void CompositionView::contentsMouseReleaseEvent(QMouseEvent* e)
{
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

        emit editRepeat(itemImpl->getSegment(), time);

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
}

void CompositionView::pointerMoveUpdate(int oldPos)
{
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
}

#include "compositionview.moc"
