/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_TRIGGER_SEGMENT_H
#define RG_TRIGGER_SEGMENT_H

#include <base/Segment.h>
#include <set>
#include <string>

namespace Rosegarden
{

typedef unsigned int TriggerSegmentId;

class ControllerContextParams;
class Event;
class Segment;

class TriggerSegmentRec
{       
public:
    typedef std::set<int> SegmentRuntimeIdSet;
    ~TriggerSegmentRec();
    TriggerSegmentRec(const TriggerSegmentRec &);
    TriggerSegmentRec &operator=(const TriggerSegmentRec &);
    bool operator==(const TriggerSegmentRec &rec) const
            { return rec.m_id == m_id; }

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

    // Return a new linked segment that corresponds in timing and
    // pitch to this triggered segment as invoked by trigger.  
    // Returns nullptr if it can't make a meaningful linked segment.
    Segment *makeLinkedSegment(Event *trigger, Segment *containing);
    Segment* makeExpansion(Event *trigger,
                           Segment *containing,
                           Instrument *instrument) const;
    bool ExpandInto(Segment *target,
                    Segment::iterator iTrigger,
                    Segment *containing,
                    ControllerContextParams *controllerContextParams) const;
    int getTranspose(const Event *trigger) const;
    int getVelocityDiff(const Event *trigger) const;
    
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
