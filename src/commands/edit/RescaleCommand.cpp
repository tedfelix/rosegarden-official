/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2026 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_NO_DEBUG_PRINT

#include "RescaleCommand.h"

#include "base/Event.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "document/BasicCommand.h"
#include "misc/Debug.h"

#include <QString>
#include <iostream>


namespace Rosegarden
{


namespace {

// Rescale t given oldDuration and newDuration.
timeT a_rescale(const timeT oldDuration,
                const timeT newDuration,
                const timeT t)
{
    // avoid overflows by using doubles
    double d = t;
    d *= newDuration;
    d /= oldDuration;
    d += 0.5;  // ??? Use lround().
    return d;
}

}


RescaleCommand::RescaleCommand(EventSelection &selection,
                               timeT newDuration,
                               bool closeGap) :
        BasicCommand(getGlobalName(), selection.getSegment(),
                     selection.getStartTime(),
                     getAffectedEndTime(selection, newDuration, closeGap),
                     true),
        m_selection(&selection),
        m_oldDuration(selection.getTotalDuration()),
        m_newDuration(newDuration),
        m_closeGap(closeGap)
{
//    std::cout << "RescaleCommand: oldDuration: " << sel.getTotalDuration() << " newDuration: " << newDuration << " close gap? " << (closeGap ? "Y" : "N") << std::endl;
    // nothing else
}

timeT
RescaleCommand::getAffectedEndTime(const EventSelection &selection,
                                   const timeT newDuration,
                                   const bool closeGap)
{
    timeT preScaleEnd = selection.getEndTime();
    if (closeGap)
        preScaleEnd = selection.getSegment().getEndMarkerTime();

    const timeT selectStart = selection.getStartTime();
    const timeT selectEnd = selection.getEndTime();
    const timeT postScaleDuration = a_rescale(
            selection.getTotalDuration(), newDuration, selectEnd - selectStart);

    const timeT postScaleEnd = selectStart + postScaleDuration;

    RG_DEBUG << "getAffectedEndTime" << preScaleEnd << postScaleEnd;

    return std::max(preScaleEnd, postScaleEnd);
}

void
RescaleCommand::modifySegment()
{
    if (m_oldDuration == m_newDuration)
        return ;

    timeT startTime = m_selection->getStartTime();
    timeT diff = m_newDuration - m_oldDuration;
    std::vector<Event *> toErase;
    std::vector<Event *> toInsert;

    Segment &segment = m_selection->getSegment();

    for (EventContainer::iterator i =
                m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        toErase.push_back(*i);

        timeT t = (*i)->getAbsoluteTime() - startTime;
        timeT d = (*i)->getDuration();
        t = a_rescale(m_oldDuration, m_newDuration, t);
        d = a_rescale(m_oldDuration, m_newDuration, d);

        toInsert.push_back(new Event(**i, startTime + t, d));
    }

    if (m_closeGap) {
        for (Segment::iterator i = segment.findTime(startTime + m_oldDuration);
                i != segment.end(); ++i) {
            // move all events including any following the end marker
            toErase.push_back(*i);
            toInsert.push_back((*i)->copyMoving(diff));
        }
    }

    for (std::vector<Event *>::iterator i = toErase.begin(); i != toErase.end(); ++i) {
        m_selection->removeEvent(*i); // remove from selection
        segment.eraseSingle(*i);
    }

    for (std::vector<Event *>::iterator i = toInsert.begin(); i != toInsert.end(); ++i) {
        segment.insert(*i);
        m_selection->addEvent(*i);  // add to selection
    }

#if 0
    // Bug #1793
    //
    // Removing this but leaving it in for future reference.  It sure would
    // be nice to normalize rests, tie notes at barlines, and auto-beam after
    // this, but I think we need to leave all that up to the user for now.
    //
    // This was removed for Bug #1793 where we noticed that using Adjust >
    // Rescale > Stretch or Squash with "Adjust times of following events..."
    // checked, the size of the Segment was modified (here) and could not be
    // undone.  This is because this modifies the size of the Segment prior to
    // BasicCommand seeing it and being able to work its magic.  Changing the
    // begin or end time of a Segment in modifySegment() is prohibited.
    //
    // We could add a "clean up notation" feature to BasicCommand that derivers
    // could request by setting a bool.  Then BasicCommand would perform a
    // normalize/tie/auto-beam/... at the right moment.

    RG_DEBUG << "modifySegment" << m_closeGap << diff << startTime <<
        segment.getEndMarkerTime();
    if (m_closeGap && diff > 0) {
        segment.setEndMarkerTime(startTime +
                                 rescale(segment.getEndMarkerTime() - startTime));
    }

    segment.normalizeRests(getStartTime(), getEndTime());
#endif
}

}
