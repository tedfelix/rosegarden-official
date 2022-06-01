/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[BasicCommand]"
#define RG_NO_DEBUG_PRINT 1

#include "BasicCommand.h"

#include "base/Segment.h"
#include "base/Composition.h"
#include "misc/Debug.h"
#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenMainWindow.h"

#include <QString>

// Getting a NULL reference.  Need to track down.  See Q_ASSERT_X()
// calls below.
#pragma GCC diagnostic ignored "-Waddress"


namespace Rosegarden
{
    
BasicCommand::BasicCommand(const QString &name, Segment &segment,
                           timeT start, timeT end, bool bruteForceRedo) :
    NamedCommand(name),
    m_segment(&segment),
    m_originalStartTime(segment.getStartTime()),
    m_startTime(calculateStartTime(start, segment)),
    m_endTime(calculateEndTime(end, segment)),
    m_modifiedEventsStart(-1),
    m_modifiedEventsEnd(-1),
    m_originalEvents(new Segment(segment.getType(), m_startTime)),
    m_doBruteForceRedo(false),
    m_redoEvents(nullptr),
    m_segmentMarking("")
{
    RG_DEBUG << "5 param ctor...";
    RG_DEBUG << "  start:" << start;
    RG_DEBUG << "  end:" << end;
    RG_DEBUG << "  m_startTime:" << m_startTime;
    RG_DEBUG << "  m_endTime:" << m_endTime;

    if (m_endTime == m_startTime)
        ++m_endTime;

    if (bruteForceRedo)
        m_redoEvents = QSharedPointer<Segment>
            (new Segment(segment.getType(), m_startTime));

}

BasicCommand::BasicCommand(const QString &name,
                           EventSelection &selection,
                           bool bruteForceRedo) :
    BasicCommand(name,
                 selection.getSegment(),
                 selection.getStartTime(),
                 selection.getEndTime(),
                 bruteForceRedo)
{
}

// Variant ctor to be used when events to insert are known when
// the command is cted.  Implies brute force redo.
BasicCommand::BasicCommand(const QString &name,
                           Segment &segment,
                           Segment *redoEvents) :
    NamedCommand(name),
    m_segment(&segment),
    m_originalStartTime(segment.getStartTime()),
    m_startTime(calculateStartTime(redoEvents->getStartTime(), *redoEvents)),
    m_endTime(calculateEndTime(redoEvents->getEndTime(), *redoEvents)),
    m_modifiedEventsStart(-1),
    m_modifiedEventsEnd(-1),
    m_originalEvents(new Segment(segment.getType(), m_startTime)),
    m_doBruteForceRedo(true),
    m_redoEvents(redoEvents->clone()), // we do not own redoEvents
    m_segmentMarking("")
{
    RG_DEBUG << "3 param ctor...";
    RG_DEBUG << "  redoEvents->getStartTime():" << redoEvents->getStartTime();
    RG_DEBUG << "  redoEvents->getEndTime():" << redoEvents->getEndTime();
    RG_DEBUG << "  m_startTime:" << m_startTime;
    RG_DEBUG << "  m_endTime:" << m_endTime;

    if (m_endTime == m_startTime)
        ++m_endTime;
}

// Variant ctor to be used when segment does not exist at creation time
// Implies brute force redo false.
BasicCommand::BasicCommand(const QString &name,
                           timeT start,
                           const QString& segmentMarking) :
    NamedCommand(name),
    m_segment(nullptr),
    m_originalStartTime(-1),
    m_startTime(start),
    m_endTime(0),
    m_modifiedEventsStart(-1),
    m_modifiedEventsEnd(-1),
    m_originalEvents(nullptr),
    m_doBruteForceRedo(false),
    m_redoEvents(nullptr),
    m_segmentMarking(segmentMarking)
{
    RG_DEBUG << "4 param ctor...";
    RG_DEBUG << "  start:" << start;
}

BasicCommand::~BasicCommand()
{
    requireSegment();
    m_originalEvents->clear();
    if (! m_redoEvents.isNull()) {
        m_redoEvents->clear();
    }
}

timeT
BasicCommand::calculateStartTime(timeT given, Segment &segment)
{
    // This assertion was firing.  Fixed.  Will leave this here
    // in case this happens again.
    Q_ASSERT_X(&segment != nullptr,
               "BasicCommand::calculateStartTime()",
               "Segment pointer is null.");

    timeT actualStartTime = given;

#if 1
    Segment::const_iterator i = segment.findTime(given);

    // For each Event at the given start time
    while (i != segment.end()  &&  (*i)->getAbsoluteTime() == given) {
        const timeT notationTime = (*i)->getNotationAbsoluteTime();
        if (notationTime < given)
            actualStartTime = notationTime;
        // Next event.
        ++i;
    }
#else
    // For each Event in the Segment
    for (const Event *event : segment) {
        // If this event is not at the given start time, bail.
        if (event->getAbsoluteTime() != given)
            break;

        const timeT notationTime = event->getNotationAbsoluteTime();
        // If the notation time is earlier, use it.
        if (notationTime < given)
            actualStartTime = notationTime;
    }
#endif

    return actualStartTime;
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
    Segment::const_iterator i = segment.findTime(given);

    while (i != segment.end()  &&  (*i)->getAbsoluteTime() == given) {
        const timeT notationTime = (*i)->getNotationAbsoluteTime();
        // If the notation time is later, use it.
        if (notationTime > given)
            actual = notationTime;

        // Next event.
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
BasicCommand::execute()
{
    requireSegment();

    RG_DEBUG << getName() << "before execute";
    RG_DEBUG << getName() << "segment" <<
                m_segment->getStartTime() << m_segment->getEndTime();
    RG_DEBUG << *m_segment;
    RG_DEBUG << getName() << "segment end";

    copyTo(m_originalEvents);

    if (m_doBruteForceRedo)
        copyFrom(m_redoEvents);
     else
        modifySegment();
    
    // calculate the start and end of the modified region
    calculateModifiedStartEnd();

    timeT updateStartTime = m_modifiedEventsStart;
    if (m_segment->getStartTime() < updateStartTime)
        updateStartTime = m_segment->getStartTime();
    m_segment->updateRefreshStatuses(updateStartTime,
                                     m_modifiedEventsEnd);

    RG_DEBUG << "execute() for " << getName() << ": updated refresh statuses "
             << updateStartTime << " -> " << m_modifiedEventsEnd;

    m_segment->signalChanged(updateStartTime, m_modifiedEventsEnd);

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

    if (m_segment->getStartTime() > m_originalStartTime) {
        // this can happen if a segment is shortened from the start
        m_segment->fillWithRests(m_originalStartTime,
                                 m_segment->getStartTime());
    }

    if (m_segment->getStartTime() < m_originalStartTime) {
        // this can happen if a segment is lengthened from the start
        for (Segment::const_iterator i = m_segment->begin();
             m_segment->isBeforeEndMarker(i); ) {

            Segment::const_iterator j = i;
            ++j;

            if ((*i)->getAbsoluteTime() >= m_originalStartTime)
                break;
            if ((*i)->getAbsoluteTime() + (*i)->getDuration() <=
                m_originalStartTime) {
                m_segment->erase(i);
            }
            i = j;
        }
    }

    // This can take a very long time.  This is because we are adding
    // events to a Segment that has someone to notify of changes.
    // Every single call to Segment::insert() fires off notifications.
    // ??? A better design is to never send notifications from Segment::insert().
    //     Instead, always rely on the client to trigger a notification when
    //     they are done.  Have to be careful, though, as some notification
    //     receivers might be expecting a notification for every single change.
    //     These cases need to be rewritten if possible, or a separate
    //     fine grained notification mechanism introduced only to be used by
    //     those who absolutely need it.
    copyFrom(m_originalEvents);

    timeT updateStartTime = m_modifiedEventsStart;
    if (m_segment->getStartTime() < updateStartTime)
        updateStartTime = m_segment->getStartTime();
    m_segment->updateRefreshStatuses(updateStartTime,
                                     m_modifiedEventsEnd);

    RG_DEBUG << "unexecute() for " << getName() << ": updated refresh statuses "
             << updateStartTime << " -> " << m_modifiedEventsEnd;

    m_segment->signalChanged(updateStartTime, m_modifiedEventsEnd);

    RG_DEBUG << "unexecute() end.";
    RG_DEBUG << getName() << "after unexecute";
    RG_DEBUG << getName() << "segment" <<
                m_segment->getStartTime() << m_segment->getEndTime();
    RG_DEBUG << *m_segment;
    RG_DEBUG << getName() << "segment end";
}
    
void
BasicCommand::copyTo(QSharedPointer<Segment> dest)
{
    requireSegment();

    RG_DEBUG << "copyTo() for" << getName() << ":" << m_segment <<
                "to" << dest;

    // Using const_iterator here since these are not being used to
    // make changes to the contents of the container.
    Segment::const_iterator from = m_segment->begin();
    Segment::const_iterator to = m_segment->end();

    dest->clear();

    // For each Event in m_segment...
    for (Segment::const_iterator i = from;
         i != m_segment->end()  &&  i != to;
         ++i) {

        RG_DEBUG << "copyTo(): Found event of type" << (*i)->getType() << "and duration" << (*i)->getDuration() << "at time" << (*i)->getAbsoluteTime();

        dest->insert(new Event(**i));
    }
}
   
void BasicCommand::copyFrom(QSharedPointer<Segment> source, bool wholeSegment)
{
    requireSegment();

    RG_DEBUG << "copyFrom() for" << getName() << ":" << source <<
                "to" << m_segment << ", range (" << m_startTime << "," <<
                m_endTime << ")";

    Segment::const_iterator from = source->findTime(m_modifiedEventsStart);
    Segment::const_iterator to = source->findTime(m_modifiedEventsEnd);

    if (wholeSegment) {
        from = source->findTime(source->getStartTime());
        to = source->findTime(source->getEndTime());
    }

    m_segment->erase(m_segment->findTime(m_modifiedEventsStart),
                     m_segment->findTime(m_modifiedEventsEnd));
    for (Segment::const_iterator i = from; i != to; ++i) {

        RG_DEBUG << "copyFrom(): Found event of type" << (*i)->getType() << "and duration" << (*i)->getDuration() << "at time" << (*i)->getAbsoluteTime();

        m_segment->insert(new Event(**i));
    }

    source->clear();
}

void
BasicCommand::requireSegment()
{
    // If we already have a Segment, bail.
    if (m_segment)
        return;

    RG_DEBUG << "requireSegment()...";

    RosegardenDocument *doc = RosegardenDocument::currentDocument;
    Composition &composition = doc->getComposition();

    // Get the marked Segment.
    m_segment = composition.getSegmentByMarking(m_segmentMarking);

    Q_ASSERT_X(m_segment != nullptr,
               "BasicCommand::requireSegment()",
               "Segment pointer is null.");
    if (!m_segment)
        return;

    RG_DEBUG << "  m_segment" << m_segment;

    // adjust start time
    m_startTime = calculateStartTime(m_startTime, *m_segment);
    m_endTime = calculateEndTime(m_segment->getEndTime(), *m_segment);

    if (m_endTime == m_startTime)
        ++m_endTime;

    m_originalEvents = QSharedPointer<Segment>
        (new Segment(m_segment->getType(), m_startTime));

    m_originalStartTime = m_segment->getStartTime();

    RG_DEBUG << "  m_segment->getStartTime():" << m_segment->getStartTime();
    RG_DEBUG << "  m_segment->getEndTime():" << m_segment->getEndTime();
    RG_DEBUG << "  m_startTime adjusted:" << m_startTime;
    RG_DEBUG << "  m_endTime:" << m_endTime;

}

void
BasicCommand::calculateModifiedStartEnd()
{
    // If we've already computed this, bail.
    if (m_modifiedEventsStart != -1  ||
        m_modifiedEventsEnd != -1)
        return;

    // use savedEvents here in case the segment was shortened
    m_modifiedEventsStart = m_originalEvents->getStartTime();
    m_modifiedEventsEnd = m_originalEvents->getEndTime();

    // m_segment has modified events m_originalEvents has the original
    // unchanged events.

    // Find the start of the modification(s).

    // If the segment start time has changed, go with the start of
    // m_originalEvents.
    if (m_segment->getStartTime() != m_originalStartTime) {
        // the segment has been shortened or lengthened from the start
        // always use the start of m_originalEvents
        m_modifiedEventsStart = m_originalEvents->getStartTime();
    } else {
        Segment::const_iterator i = m_segment->begin();
        Segment::const_iterator j = m_originalEvents->begin();
        
        while (true) {
            // If we are at the end of m_segment, we're done.
            if (i == m_segment->end())
                break;
            
            // If we are at the end of the undo buffer, we're done.
            if (j == m_originalEvents->end())
                break;
            
            const Event *segEvent = (*i);
            const Event *savedEvent = (*j);
            
            m_modifiedEventsStart = std::min(savedEvent->getAbsoluteTime(),
                                             segEvent->getAbsoluteTime()) - 1;
            // If we found a changed Event, we're done searching.
            if (!segEvent->isCopyOf(*savedEvent))
                break;
            
            // Try the next Event.
            ++i;
            ++j;
        }
    }

    // Find the end of the modification(s). This should be valid even
    // if the end time has changed

    Segment::const_reverse_iterator ir = m_segment->rbegin();
    Segment::const_reverse_iterator jr = m_originalEvents->rbegin();

    while (true) {
        // If we are at the beginning of m_segment, we're done.
        if (ir == m_segment->rend())
            break;

        // If we are at the beginning of the undo buffer, we're done.
        if (jr == m_originalEvents->rend())
            break;

        const Event *segEvent = (*ir);
        const Event *savedEvent = (*jr);

        m_modifiedEventsEnd = std::max(savedEvent->getAbsoluteTime(),
                                       segEvent->getAbsoluteTime()) + 1;
        // If we found a changed Event, we're done searching.
        if (!segEvent->isCopyOf(*savedEvent))
            break;

        // Try the previous Event.
        ++ir;
        ++jr;
    }        


    // End before start?  Go with a null range.
    if (m_modifiedEventsEnd < m_modifiedEventsStart) 
        m_modifiedEventsEnd = m_modifiedEventsStart;
        
    RG_DEBUG << "calculateModifiedStartEnd: " << m_modifiedEventsStart <<
        m_modifiedEventsEnd;
}

}
