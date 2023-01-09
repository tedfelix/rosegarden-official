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

#define RG_MODULE_STRING "[CollapseNotesCommand]"
//#define RG_NO_DEBUG_PRINT 1

#include "CollapseNotesCommand.h"

#include "base/Event.h"
#include "base/SegmentNotationHelper.h"
#include "base/Selection.h"
#include "base/BaseProperties.h"

#include <QString>


namespace Rosegarden
{

void
CollapseNotesCommand::modifySegment()
{
    Segment &segment(getSegment());
    SegmentNotationHelper helper(segment);
    timeT endTime = getEndTime();

    // Because the selection tracks the segment as a SegmentObserver,
    // anything we do to the segment will also affect the selection.

    // And because collapsing a note may delete events before or after
    // it, we can't really iterate through the selection (or segment)
    // while we do it.

    // We also can't test events to find out whether they're still in
    // the segment or not, because the event comparator will crash if
    // an event has actually been deleted.

    // So, we maintain a set of the events we have already seen
    // (checking in this set by pointer comparison only, for safety)
    // and traverse the selection requesting a collapse for each event
    // that is not already in our seen set.  Each time a collapse is
    // requested, we fly back to the start of the selection -- this is
    // partly so we are sure to see any new events that may appear
    // during collapsing, and partly so that our active iterator is
    // always valid even if an event is deleted from the selection.

    QSet<Event *> seen;

    EventContainer::iterator i =
        m_selection->getSegmentEvents().begin();

    time_t start = m_selection->getStartTime();
    time_t end = m_selection->getEndTime();

    QSet<int> beamGroups;

    while (i != m_selection->getSegmentEvents().end()) {

        Event *e = *i;

        if (!seen.contains(e)) {
            if (e->has(BaseProperties::BEAMED_GROUP_ID)) {
                int gId = e->get<Int>(BaseProperties::BEAMED_GROUP_ID);
                beamGroups.insert(gId);
            }

            seen.insert(e);

            Segment::iterator collapsed =
                helper.collapseNoteAggressively(e, endTime);
            if (collapsed != segment.end()) {
                m_selection->addEvent(*collapsed);
            }

            i = m_selection->getSegmentEvents().begin();
            continue;
        }

        ++i;
    }
    if (m_makeViable) {
        helper.makeNotesViable(m_selection->getStartTime(),
                               endTime,
                               true);
    }
    if (m_autoBeam && ! beamGroups.empty()) {
        RG_DEBUG << "before adjust" << start << end;
        // we may have destroyed some beaming with the
        // collapse. Try to repair it.
        // find start and end including beamed groups
        for (auto i = segment.begin(); i != segment.end(); ++i) {
            Event* event = *i;
            if (event->getNotationAbsoluteTime() >= start) break;
            if (event->has(BaseProperties::BEAMED_GROUP_ID)) {
                int id = event->get<Int>(BaseProperties::BEAMED_GROUP_ID);
                if (beamGroups.contains(id)) {
                    // one of the adjusted beam groups
                    start = event->getNotationAbsoluteTime();
                    break;
                }
            }
        }
        for (auto i = segment.rbegin(); i != segment.rend(); ++i) {
            Event* event = *i;
            time_t eventEnd =
                event->getNotationAbsoluteTime() + event->getNotationDuration();
            if (eventEnd <= end) break;
            if (event->has(BaseProperties::BEAMED_GROUP_ID)) {
                int id = event->get<Int>(BaseProperties::BEAMED_GROUP_ID);
                if (beamGroups.contains(id)) {
                    // one of the adjusted beam groups
                    end = eventEnd;
                    break;
                }
            }
        }
        RG_DEBUG << "before adjust" << start << end;
        helper.autoBeam(start, end, BaseProperties::GROUP_TYPE_BEAMED);
    }
}

}
