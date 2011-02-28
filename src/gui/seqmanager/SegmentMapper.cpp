/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "SegmentMapper.h"
#include "misc/Debug.h"

#include "misc/Strings.h"
#include "base/BaseProperties.h"
#include "base/Composition.h"
#include "base/Event.h"
#include "base/Exception.h"
#include "base/NotationTypes.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "base/SegmentPerformanceHelper.h"
#include "base/TriggerSegment.h"
#include "document/RosegardenDocument.h"
#include "sound/MappedEvent.h"
#include "sound/MappedSegment.h"
#include <QFile>
#include <QString>


namespace Rosegarden
{

SegmentMapper::SegmentMapper(RosegardenDocument *doc,
                             Segment *segment,
                             MappedSegment *mapped) :
    m_doc(doc),
    m_segment(segment),
    m_mapped(mapped) // I own this
{
    SEQMAN_DEBUG << "SegmentMapper : " << this << " m_mapped = " << m_mapped << endl;
}

void SegmentMapper::init()
{
    int size = calculateSize();

    if (size > 0) {
        m_mapped->resizeBuffer(size);
        if (m_segment != 0) {
            SEQMAN_DEBUG << "SegmentMapper::init : size = " << size
                         << " for segment " << m_segment->getLabel() << endl;
        }
        dump();
    } else {
        SEQMAN_DEBUG << "SegmentMapper::init : mmap size = 0 - skipping mmapping for now\n";
    }
}

SegmentMapper::~SegmentMapper()
{
    SEQMAN_DEBUG << "~SegmentMapper : " << this << " m_mapped = " << m_mapped << endl;
    delete m_mapped;
}

bool SegmentMapper::refresh()
{
    bool res = false;

    int newFill = calculateSize();
    int oldSize = m_mapped->getBufferSize();
    int oldFill = m_mapped->getBufferFill();

    SEQMAN_DEBUG << "SegmentMapper::refresh() - " << this
                 << " - old size = " << oldSize
                 << " - old fill = " << oldFill
                 << " - new fill = " << newFill
                 << endl;
    
    if (newFill > oldSize) {
        res = true;
        m_mapped->resizeBuffer(newFill);
    }

    dump();

    return res;
}

void SegmentMapper::dump()
{
    Composition &comp = m_doc->getComposition();

    RealTime eventTime;
    RealTime duration;
    Track* track = comp.getTrackById(m_segment->getTrack());

    timeT segmentStartTime = m_segment->getStartTime();
    timeT segmentEndTime = m_segment->getEndMarkerTime();
    timeT segmentDuration = segmentEndTime - segmentStartTime;
    timeT repeatEndTime = segmentEndTime;

    int repeatCount = getSegmentRepeatCount();

    if (repeatCount > 0)
        repeatEndTime = m_segment->getRepeatEndTime();

    int index = 0;
    int size = 0;

    for (int repeatNo = 0; repeatNo <= repeatCount; ++repeatNo) {

        Segment *triggered = 0;
        Segment::iterator *i = 0;

        for (Segment::iterator j = m_segment->begin();
             m_segment->isBeforeEndMarker(j) || (i && *i != triggered->end()); ) {

            bool usingi = false;
            Segment::iterator *k = &j;

            if (i && *i != triggered->end() &&
                (!m_segment->isBeforeEndMarker(j) ||
                 (**i)->getAbsoluteTime() < (*j)->getAbsoluteTime())) {
                k = i;
                usingi = true;
            }

            if (!usingi) { // don't permit nested triggered segments

                long triggerId = -1;
                (**k)->get<Int>(BaseProperties::TRIGGER_SEGMENT_ID, triggerId);

                if (triggerId >= 0) {

                    TriggerSegmentRec *rec =
                        comp.getTriggerSegmentRec(triggerId);

                    if (rec && rec->getSegment()) {
                        timeT performanceDuration =
                            SegmentPerformanceHelper(*m_segment).
                            getSoundingDuration(j);
                        if (performanceDuration > 0) {
                            mergeTriggerSegment(&triggered, *j,
                                                performanceDuration, rec);
                            if (size == 0) size = calculateSize();
                            size = addSize(size, rec->getSegment());
                            if (size > m_mapped->getBufferSize()) {
                                m_mapped->resizeBuffer(size);
                            }
                        }
                    }

                    if (triggered) {
                        if (i) delete i;
                        i = new Segment::iterator
                            (triggered->findTime((*j)->getAbsoluteTime()));
                    }

                    // Use the next triggered event (presumably the
                    // first of the current triggered segment) instead
                    // of the one that triggered it

                    ++j; // whatever happens, we don't want to write this one

                    if (i && *i != triggered->end() &&
                        (!m_segment->isBeforeEndMarker(j) ||
                         ((**i)->getAbsoluteTime() < (*j)->getAbsoluteTime()))) {
                        k = i;
                        usingi = true;
                    } else {
                        // no joy at all
                        continue;
                    }
                }
            }

            // Ignore rests
            //
            if (!(**k)->isa(Note::EventRestType)) {

                SegmentPerformanceHelper helper
                (usingi ? *triggered : *m_segment);

                timeT playTime =
                    helper.getSoundingAbsoluteTime(*k) + repeatNo * segmentDuration;
                if (playTime >= repeatEndTime) break;

                timeT playDuration = helper.getSoundingDuration(*k);

                // Ignore notes without duration -- they're probably in a tied
                // series but not as first note
                //
                if (playDuration > 0 || !(**k)->isa(Note::EventType)) {

                    if (playTime + playDuration > repeatEndTime)
                        playDuration = repeatEndTime - playTime;

                    playTime = playTime + m_segment->getDelay();
                    eventTime = comp.getElapsedRealTime(playTime);

                    // slightly quicker than calling helper.getRealSoundingDuration()
                    duration =
                        comp.getElapsedRealTime(playTime + playDuration) - eventTime;

                    eventTime = eventTime + m_segment->getRealTimeDelay();

                    try {
                        // Create mapped event and put it in buffer.
                        // The instrument will be extracted from the
                        // ControlBlock by the sequencer, so we set it
                        // to zero here.
                        MappedEvent e(0,
                                      ***k,  // three stars! what an accolade
                                      eventTime,
                                      duration);

                        e.setTrackId(track->getId());

                        if (m_segment->getTranspose() != 0 &&
                            (**k)->isa(Note::EventType)) {
                            e.setPitch(e.getPitch() + m_segment->getTranspose());
                        }

                        m_mapped->getBuffer()[index] = e;
                        ++index;

                    } catch (...) {
                        SEQMAN_DEBUG << "SegmentMapper::dump - caught exception while trying to create MappedEvent\n";
                    }
                }
            }

            ++*k; // increment either i or j, whichever one we just used
        }

        delete i;
        delete triggered;
    }

    m_mapped->setBufferFill(index);
}

void
SegmentMapper::mergeTriggerSegment(Segment **target,
                                   Event *trigger,
                                   timeT evDuration,
                                   TriggerSegmentRec *rec)
{
    if (!rec || !rec->getSegment() || rec->getSegment()->empty()) return;
    if (!*target) *target = new Segment;

    timeT evTime = trigger->getAbsoluteTime();
    timeT trStart = rec->getSegment()->getStartTime();
    timeT trEnd = rec->getSegment()->getEndMarkerTime();
    timeT trDuration = trEnd - trStart;
    if (trDuration == 0) return;

    bool retune = false;
    std::string timeAdjust = BaseProperties::TRIGGER_SEGMENT_ADJUST_NONE;

    trigger->get<Bool>(BaseProperties::TRIGGER_SEGMENT_RETUNE, retune);
    trigger->get<String>(BaseProperties::TRIGGER_SEGMENT_ADJUST_TIMES, timeAdjust);

    long evPitch = rec->getBasePitch();
    (void)trigger->get<Int>(BaseProperties::PITCH, evPitch);
    int pitchDiff = evPitch - rec->getBasePitch();

    long evVelocity = rec->getBaseVelocity();
    (void)trigger->get<Int>(BaseProperties::VELOCITY, evVelocity);
    int velocityDiff = evVelocity - rec->getBaseVelocity();

    timeT offset = 0;
    if (timeAdjust == BaseProperties::TRIGGER_SEGMENT_ADJUST_SYNC_END) {
        offset = evDuration - trDuration;
    }

    for (Segment::iterator i = rec->getSegment()->begin();
         rec->getSegment()->isBeforeEndMarker(i); ++i) {

        timeT t = (*i)->getAbsoluteTime() - trStart;
        timeT d = (*i)->getDuration();

        if (evDuration != trDuration &&
            timeAdjust == BaseProperties::TRIGGER_SEGMENT_ADJUST_SQUISH) {
            t = timeT(double(t * evDuration) / double(trDuration));
            d = timeT(double(d * evDuration) / double(trDuration));
        }

        t += evTime + offset;

        if (t < evTime) {
            if (t + d <= evTime)
                continue;
            else {
                d -= (evTime - t);
                t = evTime;
            }
        }

        if (timeAdjust == BaseProperties::TRIGGER_SEGMENT_ADJUST_SYNC_START) {
            if (t + d > evTime + evDuration) {
                if (t >= evTime + evDuration)
                    continue;
                else {
                    d = evTime + evDuration - t;
                }
            }
        }

        Event *newEvent = new Event(**i, t, d);

        if (retune && newEvent->has(BaseProperties::PITCH)) {
            int pitch =
                newEvent->get<Int>(BaseProperties::PITCH) + pitchDiff;
            if (pitch > 127)
                pitch = 127;
            if (pitch < 0)
                pitch = 0;
            newEvent->set<Int>(BaseProperties::PITCH, pitch);
        }

        if (newEvent->has(BaseProperties::VELOCITY)) {
            int velocity =
                newEvent->get<Int>(BaseProperties::VELOCITY) + velocityDiff;
            if (velocity > 127)
                velocity = 127;
            if (velocity < 0)
                velocity = 0;
            newEvent->set<Int>(BaseProperties::VELOCITY, velocity);
        }

        (*target)->insert(newEvent);
    }
}

int
SegmentMapper::getSegmentRepeatCount()
{
    int repeatCount = 0;

    timeT segmentStartTime = m_segment->getStartTime();
    timeT segmentEndTime = m_segment->getEndMarkerTime();
    timeT segmentDuration = segmentEndTime - segmentStartTime;
    timeT repeatEndTime = segmentEndTime;

    if (m_segment->isRepeating() && segmentDuration > 0) {
        repeatEndTime = m_segment->getRepeatEndTime();
        repeatCount = 1 + (repeatEndTime - segmentEndTime) / segmentDuration;
    }

    return repeatCount;
}

int
SegmentMapper::addSize(int size, Segment *s)
{
    int repeatCount = getSegmentRepeatCount();
    return size + (repeatCount + 1) * int(s->size());
}

int
SegmentMapper::calculateSize()
{
    if (!m_segment) return 0;
    int repeatCount = getSegmentRepeatCount();
    return (repeatCount + 1) * int(m_segment->size());
}

}
