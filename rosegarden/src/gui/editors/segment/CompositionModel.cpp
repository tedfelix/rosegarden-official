/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "CompositionModel.h"

#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/Segment.h"
#include "base/SnapGrid.h"
#include "CompositionColourCache.h"
#include "CompositionItemHelper.h"
#include "CompositionItemImpl.h"
#include "CompositionModelImpl.h"
#include "CompositionRect.h"
#include "gui/general/GUIPalette.h"
#include <qcolor.h>
#include <qpoint.h>
#include <qrect.h>


namespace Rosegarden
{

const CompositionModel::rectcontainer& CompositionModelImpl::getRectanglesIn(const QRect& rect,
        RectRanges* npData,
        AudioPreviewDrawData* apData)
{
    //    Profiler profiler("CompositionModelImpl::getRectanglesIn", true);

    m_res.clear();

    //     RG_DEBUG << "CompositionModelImpl::getRectanglesIn: ruler scale is "
    // 	     << (dynamic_cast<SimpleRulerScale *>(m_grid.getRulerScale()))->getUnitsPerPixel() << endl;

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

//            RG_DEBUG << "CompositionModelImpl::getRectanglesIn: segment " << s 
//                     << " selected : " << isSelected(s) << " - tmpSelected : " << isTmpSelected(s) << endl;
                       
            if (isSelected(s) || isTmpSelected(s) || sr.intersects(m_selectionRect)) {
                sr.setSelected(true);
            }

            if (pTmpSelected != tmpSelected)
                sr.setNeedsFullUpdate(true);

            bool isAudio = (s && s->getType() == Segment::Audio);

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
            if (npData && s->getType() == Segment::Internal) {
                makeNotationPreviewRects(npData, QPoint(0, sr.y()), s, rect);
                // Audio preview data
            } else if (apData && s->getType() == Segment::Audio) {
                makeAudioPreviewRects(apData, s, sr, rect);
            }

            m_res.push_back(sr);
        } else {
            //             RG_DEBUG << "CompositionModelImpl::getRectanglesIn: - segment out of rect\n";
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

CompositionModel::rectlist* CompositionModelImpl::getNotationPreviewData(const Segment* s)
{
    rectlist* npData = m_notationPreviewDataCache[const_cast<Segment*>(s)];

    if (!npData) {
        npData = makeNotationPreviewDataCache(s);
    }

    return npData;
}

CompositionModel::AudioPreviewData* CompositionModelImpl::getAudioPreviewData(const Segment* s)
{
    //    Profiler profiler("CompositionModelImpl::getAudioPreviewData", true);
    RG_DEBUG << "CompositionModelImpl::getAudioPreviewData\n";

    AudioPreviewData* apData = m_audioPreviewDataCache[const_cast<Segment*>(s)];

    if (!apData) {
        apData = makeAudioPreviewDataCache(s);
    }

    RG_DEBUG << "CompositionModelImpl::getAudioPreviewData returning\n";
    return apData;
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

bool CompositionModel::CompositionItemCompare::operator()(const CompositionItem &c1, const CompositionItem &c2) const
{
    return CompositionItemHelper::getSegment(c1) < CompositionItemHelper::getSegment(c2);
}

}
#include "CompositionModel.moc"
