/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[EraseCommand]"

#include "EraseCommand.h"

#include "misc/Debug.h"
#include "base/NotationTypes.h"
#include "base/Selection.h"
#include "base/BaseProperties.h"
#include "base/SegmentNotationHelper.h"
#include "gui/editors/notation/NotationProperties.h"


namespace Rosegarden
{


EraseCommand::EraseCommand(
        EventSelection *selection1,
        EventSelection *selection2) :
    BasicCommand(tr("&Erase"),
                 selection1 ? *selection1 : *selection2,  // assume both selections are in the same Segment
                 true),  // bruteForceRedo
    m_selection1(selection1),
    m_selection2(selection2),
    m_relayoutEndTime(getEndTime())
{
    // empty selections can cause trouble in normalize rests
    if (m_selection1  &&  m_selection1->getSegmentEvents().empty())
        m_selection1 = nullptr;
    if (m_selection2  &&  m_selection2->getSegmentEvents().empty())
        m_selection2 = nullptr;
}

EraseCommand::~EraseCommand()
{
    // ??? MEMORY LEAK (confirmed)  delete here isn't a reliable solution.
    //delete m_selection;
}

void
EraseCommand::modifySegment()
{
    const bool needRelayOut = eraseInSegment(m_selection1)  ||
                              eraseInSegment(m_selection2);

    if (needRelayOut)
        m_relayoutEndTime = getSegment().getEndTime();
}

bool
EraseCommand::eraseInSegment(EventSelection *selection)
{
    //RG_DEBUG << "eraseInSegment()";

    if (!selection)
        return false;

    Segment &segment = selection->getSegment();

    bool erasedLongEffectEvent = false;

    std::vector<Event *> toErase;

    for (EventContainer::iterator i = selection->getSegmentEvents().begin();
         i != selection->getSegmentEvents().end();
         ++i) {

        if ((*i)->isa(Clef::EventType) ||
                (*i)->isa(Key ::EventType)) {
            erasedLongEffectEvent = true;
        } else if ((*i)->isa(Indication::EventType)) {

            try {
                int graceToAdjust = 0;
                int minGraceSubOrdering = 0;
                int maxDeltaGraceSubOrdering = 0;
                int indicationSubOrdering = (*i)->getSubOrdering();
                int minSubOrdering = 0;

                // Adjust suborderings of any existing grace notes if necessary.

                Segment::iterator h, j;
                segment.getTimeSlice((*i)->getAbsoluteTime(), h, j);
                for (Segment::iterator k = h; k != j; ++k) {
                    if ((*k)->has(BaseProperties::IS_GRACE_NOTE)) {
                        if ((*k)->getSubOrdering() < indicationSubOrdering) {
                            ++graceToAdjust;
                            if ((*k)->getSubOrdering() < minGraceSubOrdering) {
                                minGraceSubOrdering = (*k)->getSubOrdering();
                                maxDeltaGraceSubOrdering =
                                    indicationSubOrdering - minGraceSubOrdering;
                            }
                        }
                    } else if ((*i) != (*k) &&
                               (*k)->getSubOrdering() < minSubOrdering) {
                        minSubOrdering = (*k)->getSubOrdering();
                    }
                }

                if (graceToAdjust > 0 &&
                    minGraceSubOrdering < indicationSubOrdering &&
                    minSubOrdering > indicationSubOrdering &&
                    maxDeltaGraceSubOrdering >= graceToAdjust) {
                    int incr = minSubOrdering - indicationSubOrdering;
                    std::vector<Event *> toInsert, toErase;
                    for (Segment::iterator k = h; k != j; ++k) {
                        if ((*k)->has(BaseProperties::IS_GRACE_NOTE) &&
                            (*k)->getSubOrdering() < indicationSubOrdering) {
                            // Subordering of the grace note is incremented to
                            // avoid (a rare) relevant decrement of that value.
                            toErase.push_back(*k);
                            toInsert.push_back
                                (new Event(**k,
                                           (*k)->getAbsoluteTime(),
                                           (*k)->getDuration(),
                                           (*k)->getSubOrdering() + incr,
                                           (*k)->getNotationAbsoluteTime(),
                                           (*k)->getNotationDuration()));
                        }
                    }
                    for (std::vector<Event *>::iterator k = toErase.begin();
                         k != toErase.end(); ++k) segment.eraseSingle(*k);
                    for (std::vector<Event *>::iterator k = toInsert.begin();
                         k != toInsert.end(); ++k) segment.insert(*k);
                }

                Indication indication(**i);
                if (indication.isOttavaType()) {

                    for (Segment::iterator j = segment.findTime ((*i)->getAbsoluteTime());
                         j != segment.findTime
                             ((*i)->getAbsoluteTime() + indication.getIndicationDuration());
                         ++j) {
                        (*j)->unset(NotationProperties::OTTAVA_SHIFT);
                    }
                }
            } catch (...) {}
        }

        // We used to do this by calling SegmentNotationHelper::deleteEvent
        // on each event in the selection, but it's probably easier to
        // cope with general selections by deleting everything in the
        // selection and then normalizing the rests.  The deleteEvent
        // mechanism is still the more sensitive way to do it for single
        // events, and it's what's used by EraseEventCommand and thus
        // the notation eraser tool.

        toErase.push_back(*i);
    }

    for (size_t j = 0; j < toErase.size(); ++j) {
        segment.eraseSingle(toErase[j]);
    }

    segment.normalizeRests(selection->getStartTime(), selection->getEndTime());
    SegmentNotationHelper(segment).updateIndications
        (selection->getStartTime(), selection->getEndTime());

    return erasedLongEffectEvent;
}

timeT
// cppcheck-suppress unusedFunction
EraseCommand::getRelayoutEndTime()
{
    return m_relayoutEndTime;
}


}
