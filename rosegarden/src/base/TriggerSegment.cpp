// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "TriggerSegment.h"

#include "Segment.h"
#include "Composition.h"
#include "BaseProperties.h"

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

}

