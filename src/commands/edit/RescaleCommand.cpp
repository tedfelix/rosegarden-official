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


#include "RescaleCommand.h"

#include "base/Event.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "document/BasicCommand.h"
#include <QString>
#include <iostream>

namespace Rosegarden
{

RescaleCommand::RescaleCommand(EventSelection &sel,
                               timeT newDuration,
                               bool closeGap) :
        BasicCommand(getGlobalName(), sel.getSegment(),
                     sel.getStartTime(),
                     getAffectedEndTime(sel, newDuration, closeGap),
                     true),
        m_selection(&sel),
        m_oldDuration(sel.getTotalDuration()),
        m_newDuration(newDuration),
        m_closeGap(closeGap)
{
//    std::cout << "RescaleCommand: oldDuration: " << sel.getTotalDuration() << " newDuration: " << newDuration << " close gap? " << (closeGap ? "Y" : "N") << std::endl;
    // nothing else
}

timeT
RescaleCommand::getAffectedEndTime(EventSelection &sel,
                                   timeT newDuration,
                                   bool closeGap)
{
    timeT preScaleEnd = sel.getEndTime();
    if (closeGap)
        preScaleEnd = sel.getSegment().getEndMarkerTime();

    // dupe of rescale(), but we can't use that here as the m_
    // variables may not have been set
    double d = preScaleEnd;
    d *= newDuration;
    d /= sel.getTotalDuration();
    d += 0.5;
    timeT postScaleEnd = (timeT)d;

    return std::max(preScaleEnd, postScaleEnd);
}

timeT
RescaleCommand::rescale(timeT t)
{
    // avoid overflows by using doubles
    double d = t;
    d *= m_newDuration;
    d /= m_oldDuration;
    d += 0.5;
    return (timeT)d;
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

    for (EventSelection::eventcontainer::iterator i =
                m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        toErase.push_back(*i);

        timeT t = (*i)->getAbsoluteTime() - startTime;
        timeT d = (*i)->getDuration();
        t = rescale(t);
        d = rescale(d);

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

    if (m_closeGap && diff > 0) {
        segment.setEndMarkerTime(startTime +
                                 rescale(segment.getEndMarkerTime() - startTime));
    }

    segment.normalizeRests(getStartTime(), getEndTime());
}

}
