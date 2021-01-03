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


#include "TupletCommand.h"

#include "base/Event.h"
#include "base/Segment.h"
#include "base/SegmentNotationHelper.h"
#include "document/BasicCommand.h"
#include "base/BaseProperties.h"
#include <QString>


namespace Rosegarden
{

using namespace BaseProperties;

TupletCommand::TupletCommand(Segment &segment,
                             timeT startTime,
                             timeT unit,
                             int untupled, int tupled,
                             bool hasTimingAlready) :
        BasicCommand(getGlobalName((untupled == 3) && (tupled == 2)),
                     segment, startTime, startTime + (unit * untupled)),
        m_unit(unit),
        m_untupled(untupled),
        m_tupled(tupled),
        m_hasTimingAlready(hasTimingAlready)
{
    // nothing else
}

void
TupletCommand::modifySegment()
{
    if (m_hasTimingAlready) {

        int groupId = getSegment().getNextId();

        for (Segment::iterator i = getSegment().findTime(getStartTime());
                getSegment().isBeforeEndMarker(i); ++i) {

            if ((*i)->getNotationAbsoluteTime() >=
                    getStartTime() + (m_unit * m_tupled))
                break;

            Event *e = *i;

            e->set
            <Int>(BEAMED_GROUP_ID, groupId);
            e->set
            <String>(BEAMED_GROUP_TYPE, GROUP_TYPE_TUPLED);

            e->set
            <Int>(BEAMED_GROUP_TUPLET_BASE, m_unit);
            e->set
            <Int>(BEAMED_GROUP_TUPLED_COUNT, m_tupled);
            e->set
            <Int>(BEAMED_GROUP_UNTUPLED_COUNT, m_untupled);
        }

    } else {
        SegmentNotationHelper helper(getSegment());
        helper.makeTupletGroup(getStartTime(), m_untupled, m_tupled, m_unit);
    }
}

}
