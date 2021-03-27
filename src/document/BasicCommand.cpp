/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_NO_DEBUG_PRINT 1
#define RG_MODULE_STRING "[BasicCommand]"

#include "BasicCommand.h"

#include "base/Segment.h"
#include "base/Composition.h"
#include "misc/Debug.h"
#include <QString>

// Getting a NULL reference.  Need to track down.  See Q_ASSERT_X()
// calls below.
#pragma GCC diagnostic ignored "-Waddress"


namespace Rosegarden
{
    
BasicCommand::BasicCommand(const QString &name, Segment &segment,
                           timeT start, timeT end, bool bruteForceRedo) :
    NamedCommand(name),
    m_startTime(calculateStartTime(start, segment)),
    m_endTime(calculateEndTime(end, segment)),
    m_segment(&segment),
    m_savedEvents(new Segment(segment.getType(), m_startTime)),
    m_doBruteForceRedo(false),
    m_redoEvents(nullptr),
    m_segmentMarking(""),
    m_comp(nullptr),
    m_modifiedEventsStart(-1),
    m_modifiedEventsEnd(-1),
    m_originalStartTime(segment.getStartTime())
{
    if (m_endTime == m_startTime) ++m_endTime;

    if (bruteForceRedo) {
        m_redoEvents = new Segment(segment.getType(), m_startTime);
    }
}

// Variant ctor to be used when events to insert are known when
// the command is cted.  Implies brute force redo.
BasicCommand::BasicCommand(const QString &name,
                           Segment &segment,
                           Segment *redoEvents) :
    NamedCommand(name),
    m_startTime(calculateStartTime(redoEvents->getStartTime(), *redoEvents)),
    m_endTime(calculateEndTime(redoEvents->getEndTime(), *redoEvents)),
    m_segment(&segment),
    m_savedEvents(new Segment(segment.getType(), m_startTime)),
    m_doBruteForceRedo(true),
    m_redoEvents(redoEvents),
    m_segmentMarking(""),
    m_comp(nullptr),
    m_modifiedEventsStart(-1),
    m_modifiedEventsEnd(-1),
    m_originalStartTime(segment.getStartTime())
{
    if (m_endTime == m_startTime) { ++m_endTime; }
}

// Variant ctor to be used when segment does not exist at creation time
// Implies brute force redo false.
BasicCommand::BasicCommand(const QString &name,
                           timeT start,
                           const QString& segmentMarking,
                           Composition* comp) :
    NamedCommand(name),
    m_startTime(start),
    m_segment(nullptr),
    m_savedEvents(nullptr),
    m_doBruteForceRedo(false),
    m_redoEvents(nullptr),
    m_segmentMarking(segmentMarking),
    m_comp(comp),
    m_modifiedEventsStart(-1),
    m_modifiedEventsEnd(-1),
    m_originalStartTime(-1)
{
}    

BasicCommand::~BasicCommand()
{
    requireSegment();
    m_savedEvents->clear();
    if (m_redoEvents) {
        m_redoEvents->clear();
        delete m_redoEvents;
    }
    delete m_savedEvents;
}

timeT
BasicCommand::calculateStartTime(timeT given, Segment &segment)
{
    // This assertion was firing.  Fixed.  Will leave this here
    // in case this happens again.
    Q_ASSERT_X(&segment != nullptr,
               "BasicCommand::calculateStartTime()",
               "Segment pointer is null.");

    timeT actual = given;
    Segment::iterator i = segment.findTime(given);

    while (i != segment.end()  &&  (*i)->getAbsoluteTime() == given) {
        timeT notation = (*i)->getNotationAbsoluteTime();
        if (notation < given) actual = notation;
        ++i;
    }

    return actual;
}

timeT
BasicCommand::calculateEndTime(timeT given, Segment &segment)
{
    // This assertion was firing.  Fixed.  Will leave this here
    // in case this happens again.
    Q_ASSERT_X(&segment != nullptr,
               "BasicCommand::calculateEndTime()",
               "Segment pointer is null.");

    timeT actual = given;
    Segment::iterator i = segment.findTime(given);

    while (i != segment.end()  &&  (*i)->getAbsoluteTime() == given) {
        timeT notation = (*i)->getNotationAbsoluteTime();
        if (notation > given) actual = notation;
        ++i;
    }

    return actual;
}

Rosegarden::Segment& BasicCommand::getSegment()
{
    requireSegment();
    return *m_segment;
}

Rosegarden::timeT BasicCommand::getRelayoutEndTime()
{
    requireSegment();
    return getEndTime();
}

void
BasicCommand::beginExecute()
{
    requireSegment();
    copyTo(m_savedEvents, true);
}

void
BasicCommand::execute()
{
    requireSegment();
    RG_DEBUG << getName() << "before execute";
    RG_DEBUG << getName() << "segment" <<
        m_segment->getStartTime() << m_segment->getEndTime();
    RG_DEBUG << *m_segment;
    RG_DEBUG << getName() << "segment end";
    beginExecute();

    if (!m_doBruteForceRedo) {
        modifySegment();
    } else {
        copyFrom(m_redoEvents);
    }
    
    // calculate the start and end of the modified region
    calculateModifiedStartEnd();

    m_segment->updateRefreshStatuses(getStartTime(), getRelayoutEndTime());

    RG_DEBUG << "execute() for " << getName() << ": updated refresh statuses "
             << getStartTime() << " -> " << getRelayoutEndTime();
    m_segment->signalChanged(getStartTime(), getRelayoutEndTime());
    
    RG_DEBUG << getName() << "after execute";
    RG_DEBUG << getName() << "segment" <<
        m_segment->getStartTime() << m_segment->getEndTime();
    RG_DEBUG << *m_segment;
    RG_DEBUG << getName() << "segment end";
}

void
BasicCommand::unexecute()
{
    requireSegment();
    RG_DEBUG << getName() << "before unexecute";
    RG_DEBUG << getName() << "segment" <<
        m_segment->getStartTime() << m_segment->getEndTime();
    RG_DEBUG << *m_segment;
    RG_DEBUG << getName() << "segment end";
    RG_DEBUG << "unexecute() begin...";

    if (m_redoEvents) {
        copyTo(m_redoEvents);
        m_doBruteForceRedo = true;
    }

    // This can take a very long time.  This is because we are adding
    // events to a Segment that has someone to notify of changes.
    // Every single call to Segment::insert() fires off notifications.
    copyFrom(m_savedEvents);

    if (m_segment->getStartTime() > m_originalStartTime) {
        // this can happen if a segment is shortened from the start
         m_segment->fillWithRests(m_originalStartTime,
                                  m_segment->getStartTime());
    }

    m_segment->updateRefreshStatuses(getStartTime(), getRelayoutEndTime());
    m_segment->signalChanged(getStartTime(), getRelayoutEndTime());

    RG_DEBUG << "unexecute() end.";
    RG_DEBUG << getName() << "after unexecute";
    RG_DEBUG << getName() << "segment" <<
        m_segment->getStartTime() << m_segment->getEndTime();
    RG_DEBUG << *m_segment;
    RG_DEBUG << getName() << "segment end";
}
    
void
BasicCommand::copyTo(Rosegarden::Segment *events, bool wholeSegment)
{
    requireSegment();
    RG_DEBUG << "copyTo() for" << getName() << ":" << m_segment <<
        "to" << events << ", range (" << m_startTime << "," << m_endTime <<
        ")";

    Segment::iterator from = m_segment->findTime(m_startTime);
    Segment::iterator to   = m_segment->findTime(m_endTime);

    if (wholeSegment) {
        from = m_segment->findTime(m_segment->getStartTime());
        to = m_segment->findTime(m_segment->getEndTime());
        events->clear();
    }

    for (Segment::iterator i = from; i != m_segment->end() && i != to; ++i) {

        RG_DEBUG << "copyTo(): Found event of type" << (*i)->getType() << "and duration" << (*i)->getDuration() << "at time" << (*i)->getAbsoluteTime();

        events->insert(new Event(**i));
    }
}
   
void
BasicCommand::copyFrom(Rosegarden::Segment *events, bool wholeSegment)
{
    requireSegment();
    RG_DEBUG << "copyFrom() for" << getName() << ":" << events <<
        "to" << m_segment << ", range (" << m_startTime << "," <<
        m_endTime << ")";

    Segment::iterator from = events->findTime(m_modifiedEventsStart);
    Segment::iterator to   = events->findTime(m_modifiedEventsEnd);

    if (wholeSegment) {
        from = events->findTime(events->getStartTime());
        to = events->findTime(events->getEndTime());
    }

    m_segment->erase(m_segment->findTime(m_modifiedEventsStart),
                     m_segment->findTime(m_modifiedEventsEnd));
    for (Segment::iterator i = from; i != to; ++i) {

        RG_DEBUG << "copyFrom(): Found event of type" << (*i)->getType() << "and duration" << (*i)->getDuration() << "at time" << (*i)->getAbsoluteTime();

        m_segment->insert(new Event(**i));
    }

    events->clear();
}

void
BasicCommand::requireSegment()
{
    if (m_segment) {
        // already got the segment
        return;
    }
    // get the segment from the id
    Q_ASSERT_X(&m_comp != nullptr,
               "BasicCommand::requireSegment()",
               "Composition pointer is null.");
    m_segment = m_comp->getSegmentByMarking(m_segmentMarking);
    RG_DEBUG << "requireSegment got segment" << m_segment;
    Q_ASSERT_X(&m_segment != nullptr,
               "BasicCommand::requireSegment()",
               "Segment pointer is null.");
    
    // adjust start time
    m_startTime = calculateStartTime(m_startTime, *m_segment);
    m_endTime = calculateEndTime(m_segment->getEndTime(), *m_segment);
    if (m_endTime == m_startTime) ++m_endTime;
    m_savedEvents = new Segment(m_segment->getType(), m_startTime);
    m_originalStartTime = m_segment->getStartTime();
}
  
void
BasicCommand::calculateModifiedStartEnd()
{
    if (m_modifiedEventsStart != -1 ||
        m_modifiedEventsEnd != -1) {
        //already done
        return;
    }
    m_modifiedEventsStart = m_segment->getStartTime();
    m_modifiedEventsEnd = m_segment->getEndTime();
    // m_segment has modified events savedEvents has the original
    // unchanged segment events
    Segment::iterator j = m_savedEvents->begin();
    if (j == m_savedEvents->end()) {
        // all done
        return;
    }
    for (Segment::iterator i = m_segment->begin();
         i != m_segment->end(); ++i) {
        Event* segEvent = (*i);
        Event* savedEvent = (*j);
        m_modifiedEventsStart = segEvent->getAbsoluteTime() - 1;
        // are they the same ?
        if (!segEvent->isCopyOf(*savedEvent)) {
            // found a changed note
            break;
        }
        ++j;
        if (j == m_savedEvents->end()) {
            // all done
            break;
        }
    }

    Segment::reverse_iterator rj = m_savedEvents->rbegin();
    for (Segment::reverse_iterator i = m_segment->rbegin();
         i != m_segment->rend(); ++i) {
        Event* segEvent = (*i);
        Event* savedEvent = (*rj);
        // are they the same ?
        if (!segEvent->isCopyOf(*savedEvent)) {
            // found a changed note
            break;
        }
        m_modifiedEventsEnd = segEvent->getAbsoluteTime() - 1;
        ++rj;
        if (rj == m_savedEvents->rend()) {
            // all done
            break;
        }
    }

    if (m_modifiedEventsEnd < m_modifiedEventsStart) 
        m_modifiedEventsEnd = m_modifiedEventsStart + 1;
        
    RG_DEBUG << "calculateModifiedStartEnd: " << m_modifiedEventsStart <<
        m_modifiedEventsEnd;
    
}

}
