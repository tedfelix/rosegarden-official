// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#ifndef _TRIGGER_SEGMENT_H_
#define _TRIGGER_SEGMENT_H_

#include <set>
#include <string>

namespace Rosegarden
{

typedef unsigned int TriggerSegmentId;

class Segment;

class TriggerSegmentRec
{	
public:
    typedef std::set<int> SegmentRuntimeIdSet;
    ~TriggerSegmentRec();
    TriggerSegmentRec(const TriggerSegmentRec &);
    TriggerSegmentRec &operator=(const TriggerSegmentRec &);
    bool operator==(const TriggerSegmentRec &rec) { return rec.m_id == m_id; }

    TriggerSegmentId getId() const { return m_id; }

    Segment *getSegment() { return m_segment; }
    const Segment *getSegment() const { return m_segment; }

    int getBasePitch() const { return m_basePitch; }
    void setBasePitch(int basePitch) { m_basePitch = basePitch; }

    int getBaseVelocity() const { return m_baseVelocity; }
    void setBaseVelocity(int baseVelocity) { m_baseVelocity = baseVelocity; }

    std::string getDefaultTimeAdjust() const { return m_defaultTimeAdjust; }
    void setDefaultTimeAdjust(std::string a) { m_defaultTimeAdjust = a; }
    
    bool getDefaultRetune() const { return m_defaultRetune; }
    void setDefaultRetune(bool r) { m_defaultRetune = r; }

    SegmentRuntimeIdSet &getReferences() { return m_references; }
    const SegmentRuntimeIdSet &getReferences() const { return m_references; }

    void updateReferences();

protected:
    friend class Composition;
    TriggerSegmentRec(TriggerSegmentId id, Segment *segment,
		      int basePitch = -1, int baseVelocity = -1,
		      std::string defaultTimeAdjust = "", bool defaultRetune = true);

    void setReferences(const SegmentRuntimeIdSet &s) { m_references = s; }

    void calculateBases();

    // data members:

    TriggerSegmentId     m_id;
    Segment             *m_segment;
    int                  m_basePitch;
    int                  m_baseVelocity;
    std::string          m_defaultTimeAdjust;
    bool                 m_defaultRetune;
    SegmentRuntimeIdSet  m_references;
};
  
struct TriggerSegmentCmp
{
    bool operator()(const TriggerSegmentRec &r1, const TriggerSegmentRec &r2) const {
	return r1.getId() < r2.getId();
    }
    bool operator()(const TriggerSegmentRec *r1, const TriggerSegmentRec *r2) const {
	return r1->getId() < r2->getId();
    }
};


}
  
#endif
