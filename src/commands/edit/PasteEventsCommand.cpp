/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[PasteEventsCommand]"

#include "PasteEventsCommand.h"

#include "misc/Debug.h"
#include "base/Clipboard.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/SegmentNotationHelper.h"
#include "document/BasicCommand.h"
#include <QString>
#include "base/BaseProperties.h"


namespace Rosegarden
{

using namespace BaseProperties;

PasteEventsCommand::PasteEventsCommand(Segment &segment,
                                       Clipboard *clipboard,
                                       timeT pasteTime,
                                       PasteType pasteType) :
    BasicCommand(getGlobalName(), segment, pasteTime,
                 getEffectiveEndTime(segment, clipboard, pasteTime)),
    m_relayoutEndTime(getEndTime()),
    m_clipboard(new Clipboard(*clipboard)),
    m_pasteType(pasteType),
    m_pastedEvents(segment)
{
    if (pasteType != OpenAndPaste) {

        // paste clef or key -> relayout to end

        if (clipboard->isSingleSegment()) {

            Segment *s(clipboard->getSingleSegment());
            for (Segment::iterator i = s->begin(); i != s->end(); ++i) {
                if ((*i)->isa(Clef::EventType) ||
                    (*i)->isa(Key::EventType)) {
                    m_relayoutEndTime = s->getEndTime();
                    break;
                }
            }
        }
    }
}

PasteEventsCommand::PasteEventsCommand(Segment &segment,
                                       Clipboard *clipboard,
                                       timeT pasteTime,
                                       timeT pasteEndTime,
                                       PasteType pasteType) :
    BasicCommand(getGlobalName(), segment, pasteTime, pasteEndTime),
    m_relayoutEndTime(getEndTime()),
    m_clipboard(new Clipboard(*clipboard)),
    m_pasteType(pasteType),
    m_pastedEvents(segment)
{}

PasteEventsCommand::~PasteEventsCommand()
{
    delete m_clipboard;
}

PasteEventsCommand::PasteTypeMap

PasteEventsCommand::getPasteTypes()
{
    static PasteTypeMap types;
    static bool haveTypes = false;
    if (!haveTypes) {
        types[Restricted] =
            tr("Paste into an existing gap [\"restricted\"]");
        types[Simple] =
            tr("Erase existing events to make room [\"simple\"]");
        types[OpenAndPaste] =
            tr("Move existing events out of the way [\"open-n-paste\"]");
        types[NoteOverlay] =
            tr("Overlay notes, tying against present notes [\"note-overlay\"]");
        types[MatrixOverlay] =
            tr("Overlay notes, ignoring present notes [\"matrix-overlay\"]");
    }
    return types;
}

timeT
PasteEventsCommand::getEffectiveEndTime(Segment &segment,
                                        Clipboard *clipboard,
                                        timeT pasteTime)
{
    if (!clipboard->isSingleSegment()) {
        RG_DEBUG << "PasteEventsCommand::getEffectiveEndTime: not single segment" << endl;
        return pasteTime;
    }

    RG_DEBUG << "PasteEventsCommand::getEffectiveEndTime: clipboard "
    << clipboard->getSingleSegment()->getStartTime()
    << " -> "
    << clipboard->getSingleSegment()->getEndTime() << endl;

    timeT d = clipboard->getSingleSegment()->getEndTime() -
              clipboard->getSingleSegment()->getStartTime();

    if (m_pasteType == OpenAndPaste) {
        return segment.getEndTime() + d;
    } else {
        Segment::iterator i = segment.findTime(pasteTime + d);
        if (i == segment.end())
            return segment.getEndTime();
        else
            return (*i)->getAbsoluteTime();
    }
}

timeT
PasteEventsCommand::getRelayoutEndTime()
{
    return m_relayoutEndTime;
}

bool
PasteEventsCommand::isPossible()
{
    if (m_clipboard->isEmpty() || !m_clipboard->isSingleSegment()) {
        return false;
    }

    if (m_pasteType != Restricted) {
        return true;
    }

    Segment *source = m_clipboard->getSingleSegment();

    timeT pasteTime = getStartTime();
    timeT origin = source->getStartTime();
    timeT duration = source->getEndTime() - origin;

    RG_DEBUG << "PasteEventsCommand::isPossible: paste time is " << pasteTime << ", origin is " << origin << ", duration is " << duration << endl;

    SegmentNotationHelper helper(getSegment());
    return helper.removeRests(pasteTime, duration, true);
}

void
PasteEventsCommand::modifySegment()
{
    RG_DEBUG << "PasteEventsCommand::modifySegment" << endl;

    if (!m_clipboard->isSingleSegment())
        return ;

    Segment *source = m_clipboard->getSingleSegment();

    timeT pasteTime = getStartTime();
    timeT origin = source->getStartTime();
    timeT duration = source->getEndTime() - origin;

    Segment *destination(&getSegment());
    SegmentNotationHelper helper(*destination);

    RG_DEBUG << "PasteEventsCommand::modifySegment() : paste type = "
    << m_pasteType << " - pasteTime = "
    << pasteTime << " - origin = " << origin << endl;

    // First check for group IDs, which we want to make unique in the
    // copies in the destination segment

    std::map<long, long> groupIdMap;
    for (Segment::iterator i = source->begin(); i != source->end(); ++i) {
        long groupId = -1;
        if ((*i)->get
                <Int>(BEAMED_GROUP_ID, groupId)) {
            if (groupIdMap.find(groupId) == groupIdMap.end()) {
                groupIdMap[groupId] = destination->getNextId();
            }
        }
    }

    switch (m_pasteType) {

        // Do some preliminary work to make space or whatever;
        // we do the actual paste after this switch statement
        // (except where individual cases do the work and return)

    case Restricted:
        if (!helper.removeRests(pasteTime, duration))
            return ;
        break;

    case Simple:
        destination->erase(destination->findTime(pasteTime),
                           destination->findTime(pasteTime + duration));
        break;

    case OpenAndPaste: {
            std::vector<Event *> copies;
            for (Segment::iterator i = destination->findTime(pasteTime);
                    i != destination->end(); ++i) {
                Event *e = (*i)->copyMoving(duration);
                if (e->has(BEAMED_GROUP_ID)) {
                    e->set
                    <Int>(BEAMED_GROUP_ID, groupIdMap[e->get
                                                      <Int>(BEAMED_GROUP_ID)]);
                }
                copies.push_back(e);
            }

            destination->erase(destination->findTime(pasteTime),
                               destination->end());

            for (size_t i = 0; i < copies.size(); ++i) {
                destination->insert(copies[i]);
                m_pastedEvents.addEvent(copies[i]);
            }

            break;
        }

    case NoteOverlay:
        for (Segment::iterator i = source->begin(); i != source->end(); ++i) {
            if ((*i)->isa(Note::EventRestType))
                continue;
            Event *e = (*i)->copyMoving(pasteTime - origin);
            if (e->has(BEAMED_GROUP_ID)) {
                e->set<Int>(BEAMED_GROUP_ID,
                            groupIdMap[e->get<Int>(BEAMED_GROUP_ID)]);
            }
            if ((*i)->isa(Note::EventType)) {
                // e is model event: we retain ownership of it
                Segment::iterator i = helper.insertNote(e);
                delete e;
                if (i != destination->end()) m_pastedEvents.addEvent(*i);
            } else {
                destination->insert(e);
                m_pastedEvents.addEvent(e);
            }
        }

        return ;

    case MatrixOverlay:

        for (Segment::iterator i = source->begin(); i != source->end(); ++i) {

            if ((*i)->isa(Note::EventRestType))
                continue;

            Event *e = (*i)->copyMoving(pasteTime - origin);

            if (e->has(BEAMED_GROUP_TYPE) &&
                    e->get
                    <String>(BEAMED_GROUP_TYPE) == GROUP_TYPE_BEAMED) {
                e->unset(BEAMED_GROUP_ID);
                e->unset(BEAMED_GROUP_TYPE);
            }

            if (e->has(BEAMED_GROUP_ID)) {
                e->set
                <Int>(BEAMED_GROUP_ID, groupIdMap[e->get
                                                  <Int>(BEAMED_GROUP_ID)]);
            }

            destination->insert(e);
            m_pastedEvents.addEvent(e);
        }

//        destination->normalizeRests(pasteTime, pasteTime + duration);
 
        // We ran the normalizeRests() calls like this as far back as I can go
        // in history, and since normalizeRests() has been implicated in a
        // string of recent paste bugs, we're going with the historical version: 
        destination->normalizeRests(source->getStartTime(), source->getEndTime());

        return ;
    }

    RG_DEBUG << "PasteEventsCommand::modifySegment() - inserting\n";

    for (Segment::iterator i = source->begin(); i != source->end(); ++i) {
        Event *e = (*i)->copyMoving(pasteTime - origin);
        if (e->has(BEAMED_GROUP_ID)) {
            e->set
            <Int>(BEAMED_GROUP_ID, groupIdMap[e->get
                                              <Int>(BEAMED_GROUP_ID)]);
        }
        destination->insert(e);
        m_pastedEvents.addEvent(e);
    }

//    destination->normalizeRests(pasteTime, pasteTime + duration);


    // We ran the normalizeRests() calls like this as far back as I can go
    // in history, and since normalizeRests() has been implicated in a
    // string of recent paste bugs, we're going with the historical version: 
    destination->normalizeRests(source->getStartTime(), source->getEndTime());
}

EventSelection
PasteEventsCommand::getPastedEvents()
{
    return m_pastedEvents;
}

}
