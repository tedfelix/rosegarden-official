/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
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
#include "misc/Debug.h"
#include <QString>


namespace Rosegarden
{
    
BasicCommand::BasicCommand(const QString &name, Segment &segment,
                           timeT start, timeT end, bool bruteForceRedo) :
    NamedCommand(name),
    m_startTime(calculateStartTime(start, segment)),
    m_endTime(calculateEndTime(end, segment)),
    m_segment(segment),
    m_savedEvents(segment.getType(), m_startTime),
    m_doBruteForceRedo(false),
    m_redoEvents(nullptr)
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
    m_segment(segment),
    m_savedEvents(segment.getType(), m_startTime),
    m_doBruteForceRedo(true),
    m_redoEvents(redoEvents)
{
    if (m_endTime == m_startTime) { ++m_endTime; }
}


BasicCommand::~BasicCommand()
{
    m_savedEvents.clear();
    if (m_redoEvents) m_redoEvents->clear();
    delete m_redoEvents;
}

timeT
BasicCommand::calculateStartTime(timeT given, Segment &segment)
{
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
    return m_segment;
}

Rosegarden::timeT BasicCommand::getRelayoutEndTime()
{
    return getEndTime();
}

void
BasicCommand::beginExecute()
{
    copyTo(&m_savedEvents);
}

void
BasicCommand::execute()
{
    beginExecute();

    if (!m_doBruteForceRedo) {
        modifySegment();
    } else {
        copyFrom(m_redoEvents);
    }

    m_segment.updateRefreshStatuses(getStartTime(), getRelayoutEndTime());

    RG_DEBUG << "execute() for " << getName() << ": updated refresh statuses "
             << getStartTime() << " -> " << getRelayoutEndTime();
    m_segment.signalChanged(getStartTime(), getRelayoutEndTime());
}

void
BasicCommand::unexecute()
{
    RG_DEBUG << "unexecute() begin...";

    if (m_redoEvents) {
        copyTo(m_redoEvents);
        m_doBruteForceRedo = true;
    }

    // This can take a very long time.  This is because we are adding
    // events to a Segment that has someone to notify of changes.
    // Every single call to Segment::insert() fires off notifications.
    copyFrom(&m_savedEvents);

    m_segment.updateRefreshStatuses(getStartTime(), getRelayoutEndTime());
    m_segment.signalChanged(getStartTime(), getRelayoutEndTime());

    RG_DEBUG << "unexecute() end.";
}
    
void
BasicCommand::copyTo(Rosegarden::Segment *events)
{
    RG_DEBUG << "copyTo() for" << getName() << ":" << &m_segment <<
        "to" << events << ", range (" << m_startTime << "," << m_endTime <<
        ")";

    Segment::iterator from = m_segment.findTime(m_startTime);
    Segment::iterator to   = m_segment.findTime(m_endTime);

    for (Segment::iterator i = from; i != m_segment.end() && i != to; ++i) {

       //RG_DEBUG << "copyTo(): Found event of type" << (*i)->getType() << "and duration" << (*i)->getDuration() << "at time" << (*i)->getAbsoluteTime();

       events->insert(new Event(**i));
    }
}
   
void
BasicCommand::copyFrom(Rosegarden::Segment *events)
{
    RG_DEBUG << "copyFrom() for" << getName() << ":" << events <<
        "to" << &m_segment << ", range (" << m_startTime << "," <<
        m_endTime << ")";

    m_segment.erase(m_segment.findTime(m_startTime),
                    m_segment.findTime(m_endTime));

    for (Segment::iterator i = events->begin(); i != events->end(); ++i) {

        //RG_DEBUG << "copyFrom(): Found event of type" << (*i)->getType() << "and duration" << (*i)->getDuration() << "at time" << (*i)->getAbsoluteTime();

        m_segment.insert(new Event(**i));
    }

    events->clear();
}
    
}
