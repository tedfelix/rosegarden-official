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

#define RG_MODULE_STRING "[SegmentResizeFromStartCommand]"
#define RG_NO_DEBUG_PRINT 1

#include "SegmentResizeFromStartCommand.h"

#include "base/Event.h"
#include "base/Segment.h"
#include "document/BasicCommand.h"


namespace Rosegarden
{

SegmentResizeFromStartCommand::SegmentResizeFromStartCommand(Segment *s,
        timeT time) :
        BasicCommand(getGlobalName(), *s,
                     std::min(time, s->getStartTime()),
                     std::max(time, s->getStartTime())),
        m_segment(s),
        m_oldStartTime(s->getStartTime()),
        m_newStartTime(time)
{
    RG_DEBUG << "ctor Segment start end" <<
        s->getStartTime() << s->getEndTime();
    // nothing else
}

SegmentResizeFromStartCommand::~SegmentResizeFromStartCommand()
{
    // nothing
}

void
SegmentResizeFromStartCommand::modifySegment()
{
    RG_DEBUG << "modifySegment start Segment start end" <<
        m_segment->getStartTime() << m_segment->getEndTime();
    if (m_newStartTime < m_oldStartTime) {
        m_segment->fillWithRests(m_newStartTime, m_oldStartTime);
    } else {

        for (Segment::iterator i = m_segment->begin();
                m_segment->isBeforeEndMarker(i); ) {

            Segment::iterator j = i;
            ++j;

            if ((*i)->getAbsoluteTime() >= m_newStartTime)
                break;

            if ((*i)->getAbsoluteTime() + (*i)->getDuration() <= m_newStartTime) {
                m_segment->erase(i);
            } else {
                Event *e = new Event
                           (**i, m_newStartTime,
                            (*i)->getAbsoluteTime() + (*i)->getDuration() - m_newStartTime);
                m_segment->erase(i);
                m_segment->insert(e);
            }

            i = j;
        }
    }
    RG_DEBUG << "modifySegment done Segment start end" <<
        m_segment->getStartTime() << m_segment->getEndTime();
}

}
