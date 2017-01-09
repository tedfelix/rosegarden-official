/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "CutAndCloseCommand.h"

#include "base/Clipboard.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "CutCommand.h"

#include <QtGlobal>
#include <QString>

#include "misc/Debug.h"


namespace Rosegarden
{

CutAndCloseCommand::CutAndCloseCommand(EventSelection &selection,
                                       Clipboard *clipboard) :
        MacroCommand(getGlobalName())
{
    addCommand(new CutCommand(selection, clipboard));
    addCommand(new CloseCommand(&selection.getSegment(),
                                selection.getEndTime(),
                                selection.getStartTime()));
}

void
CutAndCloseCommand::CloseCommand::execute()
{
    // We shift all the events from m_gapEnd to the end of the
    // segment back so that they start at m_gapStart instead of m_gapEnd.

    Q_ASSERT(m_gapEnd >= m_gapStart);
    if (m_gapEnd == m_gapStart)
        return ;

    // We also need to record how many events there are already at
    // m_gapStart so that we can leave those unchanged when we undo.
    // (This command is executed on the understanding that the area
    // between m_gapStart and m_gapEnd is empty of all but rests, but
    // in practice there may be other things such as a clef at the
    // same time as m_gapStart.  This will only work for events that
    // have smaller subordering than notes etc.)

    m_staticEvents = 0;
    for (Segment::iterator i = m_segment->findTime(m_gapStart);
            m_segment->isBeforeEndMarker(i); ++i) {
        if ((*i)->getAbsoluteTime() > m_gapStart)
            break;
        if ((*i)->isa(Note::EventRestType))
            continue;
        ++m_staticEvents;
    }

    std::vector<Event *> events;
    timeT timeDifference = m_gapEnd - m_gapStart;

    for (Segment::iterator i = m_segment->findTime(m_gapEnd);
            m_segment->isBeforeEndMarker(i); ++i) {
        events.push_back((*i)->copyMoving( -timeDifference));
    }

    timeT oldEndTime = m_segment->getEndTime();

    // remove rests from target area, and everything thereafter
    for (Segment::iterator i = m_segment->findTime(m_gapStart);
            m_segment->isBeforeEndMarker(i); ) {
        if ((*i)->getAbsoluteTime() >= m_gapEnd ||
                (*i)->isa(Note::EventRestType)) {
            Segment::iterator j(i);
            ++j;
            m_segment->erase(i);
            i = j;
        } else {
            ++i;
        }
    }

    for (size_t i = 0; i < events.size(); ++i) {
        m_segment->insert(events[i]);
    }

    m_segment->normalizeRests(m_segment->getEndTime(), oldEndTime);
}

void
CutAndCloseCommand::CloseCommand::unexecute()
{
    // We want to shift events from m_gapStart to the end of the
    // segment forward so as to start at m_gapEnd instead of
    // m_gapStart.

    Q_ASSERT(m_gapEnd >= m_gapStart);
    if (m_gapEnd == m_gapStart)
        return ;

    // May need to ignore some static events at m_gapStart.
    // These are assumed to have smaller subordering than whatever
    // we're not ignoring.  Actually this still isn't quite right:
    // it'll do the wrong thing where we have, say, a clef then
    // some notes then another clef and we cut-and-close all the
    // notes and then undo.  But it's better than we were doing
    // before.

    Segment::iterator starti = m_segment->findTime(m_gapStart);

    while (m_segment->isBeforeEndMarker(starti)) {
        if (m_staticEvents == 0)
            break;
        if ((*starti)->getAbsoluteTime() > m_gapStart)
            break;
        if (!(*starti)->isa(Note::EventRestType))
            --m_staticEvents;
        ++starti;
    }

    std::vector<Event *> events;
    timeT timeDifference = m_gapEnd - m_gapStart;

    for (Segment::iterator i = starti; m_segment->isBeforeEndMarker(i); ) {
        Segment::iterator j(i);
        ++j;
        events.push_back((*i)->copyMoving(timeDifference));
        m_segment->erase(i);
        i = j;
    }

    for (size_t i = 0; i < events.size(); ++i) {
        m_segment->insert(events[i]);
    }

    timeT endTime = m_segment->getEndTime();
    NOTATION_DEBUG << "setting end time to " << (endTime - timeDifference);
    //!!! this following is not working for bugaccidentals.rg:
    m_segment->setEndTime(endTime - timeDifference);

    m_segment->normalizeRests(m_gapStart, m_gapEnd);
}

}
