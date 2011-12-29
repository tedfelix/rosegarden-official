/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "TriggerSegment.h"

#include "base/Segment.h"
#include "base/SegmentPerformanceHelper.h"
#include "Composition.h"
#include "base/BaseProperties.h"
#include <limits>

namespace Rosegarden
{

TriggerSegmentRec::~TriggerSegmentRec()
{
    // nothing -- we don't delete the segment here
}

TriggerSegmentRec::TriggerSegmentRec(TriggerSegmentId id,
				     Segment *segment,
				     int basePitch,
				     int baseVelocity,
				     std::string timeAdjust, 
				     bool retune) :
    m_id(id),
    m_segment(segment),
    m_basePitch(basePitch),
    m_baseVelocity(baseVelocity),
    m_defaultTimeAdjust(timeAdjust),
    m_defaultRetune(retune)
{
    if (m_defaultTimeAdjust == "") {
	m_defaultTimeAdjust = BaseProperties::TRIGGER_SEGMENT_ADJUST_SQUISH;
    }

    calculateBases();
    updateReferences();
}

TriggerSegmentRec::TriggerSegmentRec(const TriggerSegmentRec &rec) :
    m_id(rec.m_id),
    m_segment(rec.m_segment),
    m_basePitch(rec.m_basePitch),
    m_baseVelocity(rec.m_baseVelocity),
    m_defaultTimeAdjust(rec.m_defaultTimeAdjust),
    m_defaultRetune(rec.m_defaultRetune),
    m_references(rec.m_references)
{
    // nothing else
}

TriggerSegmentRec &
TriggerSegmentRec::operator=(const TriggerSegmentRec &rec)
{
    if (&rec == this) return *this;
    m_id = rec.m_id;
    m_segment = rec.m_segment;
    m_basePitch = rec.m_basePitch;
    m_baseVelocity = rec.m_baseVelocity;
    m_references = rec.m_references;
    return *this;
}

void
TriggerSegmentRec::updateReferences()
{
    if (!m_segment) return;

    Composition *c = m_segment->getComposition();
    if (!c) return;

    m_references.clear();

    for (Composition::iterator i = c->begin(); i != c->end(); ++i) {
	for (Segment::iterator j = (*i)->begin(); j != (*i)->end(); ++j) {
	    if ((*j)->has(BaseProperties::TRIGGER_SEGMENT_ID) &&
		(*j)->get<Int>(BaseProperties::TRIGGER_SEGMENT_ID) == long(m_id)) {
		m_references.insert((*i)->getRuntimeId());
		break; // from inner loop only: go on to next segment
	    }
	}
    }
}

void
TriggerSegmentRec::calculateBases()
{
    if (!m_segment) return;

    for (Segment::iterator i = m_segment->begin();
	 m_segment->isBeforeEndMarker(i); ++i) {

	if (m_basePitch >= 0 && m_baseVelocity >= 0) return;

	if (m_basePitch < 0) {
	    if ((*i)->has(BaseProperties::PITCH)) {
		m_basePitch = (*i)->get<Int>(BaseProperties::PITCH);
	    }
	}

	if (m_baseVelocity < 0) {
	    if ((*i)->has(BaseProperties::VELOCITY)) {
		m_baseVelocity = (*i)->get<Int>(BaseProperties::VELOCITY);
	    }
	}
    }

    if (m_basePitch < 0) m_basePitch = 60;
    if (m_baseVelocity < 0) m_baseVelocity = 100;
}

// Constructor for TriggerSegmentTimeAdjust.  Computes the right linear
// transform for the given trigger segment and event.
// @param trigger is an iterator corresponding to the triggering
// event.
// @param oversegment is the segment that the trigger segment is
// triggered in.
// @author Tom Breton (Tehom)
// Adapted from SegmentMapper.cpp
TriggerSegmentTimeAdjust::TriggerSegmentTimeAdjust
(TriggerSegmentRec *rec, Segment::iterator trigger, Segment *oversegment)
{
    Event * ev = *trigger;
    timeT performanceStart = ev->getAbsoluteTime();
    timeT performanceDuration =
        SegmentPerformanceHelper(*oversegment).getSoundingDuration(trigger);

    timeT trStart = rec->getSegment()->getStartTime();
    timeT trEnd = rec->getSegment()->getEndMarkerTime();
    timeT trDuration = trEnd - trStart;

    std::string timeAdjust = BaseProperties::TRIGGER_SEGMENT_ADJUST_NONE;
    ev->get<String>(BaseProperties::TRIGGER_SEGMENT_ADJUST_TIMES, timeAdjust);
    
    if ((timeAdjust == BaseProperties::TRIGGER_SEGMENT_ADJUST_SQUISH) &&
        (trDuration != 0)) {
        // If duration is zero, we use the default case instead.
        m_ratio = (double(performanceDuration) / double(trDuration));
        m_offset = timeT(double(-trStart) / m_ratio) + performanceStart;
    } else if (timeAdjust == BaseProperties::TRIGGER_SEGMENT_ADJUST_SYNC_END) {
        m_ratio = 1.0;
        m_offset = performanceStart - trStart +
            performanceDuration - trDuration;
    } else {
        // Used for TRIGGER_SEGMENT_ADJUST_NONE or
        // TRIGGER_SEGMENT_ADJUST_SYNC_START
        m_ratio = 1.0;
        m_offset = performanceStart - trStart;
    }
    m_start = performanceStart;
    // Only TRIGGER_SEGMENT_ADJUST_SYNC_START needs clipping at the end.
    m_end =
        (timeAdjust == BaseProperties::TRIGGER_SEGMENT_ADJUST_SYNC_START) ? 
        performanceStart + performanceDuration :
        std::numeric_limits<int>::max();
}

}

