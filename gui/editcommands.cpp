// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "editcommands.h"

#include <qregexp.h>

#include <kconfig.h>

#include "NotationTypes.h"
#include "Selection.h"
#include "SegmentNotationHelper.h"
#include "SegmentMatrixHelper.h"
#include "BaseProperties.h"
#include "Clipboard.h"
#include "Profiler.h"
#include "Marker.h"

#include "notationproperties.h"
#include "segmentcommands.h"

#include "rosestrings.h"
#include "rosedebug.h"
#include "rgapplication.h"

using Rosegarden::Segment;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::Event;
using Rosegarden::timeT;
using Rosegarden::Note;
using Rosegarden::Clef;
using Rosegarden::Int;
using Rosegarden::String;
using Rosegarden::Text;
using Rosegarden::Accidental;
using Rosegarden::Accidentals::NoAccidental;
using Rosegarden::Indication;
using Rosegarden::EventSelection;
using Rosegarden::SegmentSelection;
using Rosegarden::TrackId;

using namespace Rosegarden::BaseProperties;

using std::string;
using std::endl;


CutCommand::CutCommand(EventSelection &selection,
		       Rosegarden::Clipboard *clipboard) :
    KMacroCommand(getGlobalName())
{
    addCommand(new CopyCommand(selection, clipboard));
    addCommand(new EraseCommand(selection));
}

CutCommand::CutCommand(SegmentSelection &selection,
		       Rosegarden::Clipboard *clipboard) :
    KMacroCommand(getGlobalName())
{
    addCommand(new CopyCommand(selection, clipboard));

    for (SegmentSelection::iterator i = selection.begin();
	 i != selection.end(); ++i) {
	addCommand(new SegmentEraseCommand(*i));
    }
}


CutAndCloseCommand::CutAndCloseCommand(Rosegarden::EventSelection &selection,
				       Rosegarden::Clipboard *clipboard) :
    KMacroCommand(getGlobalName())
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

    assert(m_gapEnd >= m_gapStart);
    if (m_gapEnd == m_gapStart) return;

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
	if ((*i)->getAbsoluteTime() > m_gapStart) break;
	if ((*i)->isa(Rosegarden::Note::EventRestType)) continue;
	++m_staticEvents;
    }

    std::vector<Event *> events;
    timeT timeDifference = m_gapEnd - m_gapStart;

    for (Segment::iterator i = m_segment->findTime(m_gapEnd);
	 m_segment->isBeforeEndMarker(i); ++i) {
	events.push_back(new Event
			 (**i, (*i)->getAbsoluteTime() - timeDifference));
    }

    timeT oldEndTime = m_segment->getEndTime();

    // remove rests from target area, and everything thereafter
    for (Segment::iterator i = m_segment->findTime(m_gapStart);
	 m_segment->isBeforeEndMarker(i); ) {
	if ((*i)->getAbsoluteTime() >= m_gapEnd ||
	    (*i)->isa(Rosegarden::Note::EventRestType)) {
	    Segment::iterator j(i);
	    ++j;
	    m_segment->erase(i);
	    i = j;
	} else {
	    ++i;
	}
    }
    
    for (unsigned int i = 0; i < events.size(); ++i) {
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

    assert(m_gapEnd >= m_gapStart);
    if (m_gapEnd == m_gapStart) return;

    // May need to ignore some static events at m_gapStart.
    // These are assumed to have smaller subordering than whatever
    // we're not ignoring.  Actually this still isn't quite right:
    // it'll do the wrong thing where we have, say, a clef then
    // some notes then another clef and we cut-and-close all the
    // notes and then undo.  But it's better than we were doing
    // before.

    Segment::iterator starti = m_segment->findTime(m_gapStart);

    while (m_segment->isBeforeEndMarker(starti)) {
	if (m_staticEvents == 0) break;
	if ((*starti)->getAbsoluteTime() > m_gapStart) break;
	if (!(*starti)->isa(Note::EventRestType)) --m_staticEvents;
	++starti;
    }

    std::vector<Event *> events;
    timeT timeDifference = m_gapEnd - m_gapStart;

    for (Segment::iterator i = starti; m_segment->isBeforeEndMarker(i); ) {
	Segment::iterator j(i);
	++j;
	events.push_back(new Event
			 (**i, (*i)->getAbsoluteTime() + timeDifference));
	m_segment->erase(i);
	i = j;
    }

    for (unsigned int i = 0; i < events.size(); ++i) {
	m_segment->insert(events[i]);
    }

    timeT endTime = m_segment->getEndTime();
    NOTATION_DEBUG << "setting end time to " << (endTime - timeDifference) << endl;
//!!! this following is not working for bugaccidentals.rg:
    m_segment->setEndTime(endTime - timeDifference);

    m_segment->normalizeRests(m_gapStart, m_gapEnd);
}  


CopyCommand::CopyCommand(EventSelection &selection,
			 Rosegarden::Clipboard *clipboard) :
    KNamedCommand(getGlobalName()),
    m_targetClipboard(clipboard)
{
    m_sourceClipboard = new Rosegarden::Clipboard;
    m_sourceClipboard->newSegment(&selection)->setLabel
	(selection.getSegment().getLabel() + " " + qstrtostr(i18n("(excerpt)")));
}

CopyCommand::CopyCommand(SegmentSelection &selection,
			 Rosegarden::Clipboard *clipboard) :
    KNamedCommand(getGlobalName()),
    m_targetClipboard(clipboard)
{
    m_sourceClipboard = new Rosegarden::Clipboard;

    for (SegmentSelection::iterator i = selection.begin();
	 i != selection.end(); ++i) {
	m_sourceClipboard->newSegment(*i)->setLabel((*i)->getLabel() + " " +
						    qstrtostr(i18n("(copied)")));
    }
}

CopyCommand::~CopyCommand()
{
    delete m_sourceClipboard;
}

void
CopyCommand::execute()
{
//    RG_DEBUG << "CopyCommand::execute" << endl;

    Rosegarden::Clipboard temp(*m_targetClipboard);
    m_targetClipboard->copyFrom(m_sourceClipboard);
    m_sourceClipboard->copyFrom(&temp);
}

void
CopyCommand::unexecute()
{
//    RG_DEBUG << "CopyCommand::unexecute" << endl;

    Rosegarden::Clipboard temp(*m_sourceClipboard);
    m_sourceClipboard->copyFrom(m_targetClipboard);
    m_targetClipboard->copyFrom(&temp);
}


PasteSegmentsCommand::PasteSegmentsCommand(Rosegarden::Composition *composition,
					   Rosegarden::Clipboard *clipboard,
					   Rosegarden::timeT pasteTime) :
    KNamedCommand(getGlobalName()),
    m_composition(composition),
    m_clipboard(clipboard),
    m_pasteTime(pasteTime),
    m_detached(false)
{
    // nothing else
}

PasteSegmentsCommand::~PasteSegmentsCommand()
{
    if (m_detached) {
	for (unsigned int i = 0; i < m_addedSegments.size(); ++i) {
	    delete m_addedSegments[i];
	}
    }
}

void
PasteSegmentsCommand::execute()
{
    if (m_addedSegments.size() > 0) {
	// been here before
	for (unsigned int i = 0; i < m_addedSegments.size(); ++i) {
            m_addedSegments[i]->setTrack(m_composition->getSelectedTrack());
	    m_composition->addSegment(m_addedSegments[i]);
	}
	return;
    }

    if (m_clipboard->isEmpty()) return;

    // We want to paste such that the earliest Segment starts at
    // m_pasteTime and the others start at the same times relative
    // to that as they did before

    timeT earliestStartTime = 0;
    timeT latestEndTime = 0;
    int trackOffset = 0;

    for (Rosegarden::Clipboard::iterator i = m_clipboard->begin();
	 i != m_clipboard->end(); ++i) {

	if (i == m_clipboard->begin() ||
	    (*i)->getStartTime() < earliestStartTime) {
	    earliestStartTime = (*i)->getStartTime();
            trackOffset = (*i)->getTrack();
	}

        if ((*i)->getEndMarkerTime() > latestEndTime)
            latestEndTime = (*i)->getEndMarkerTime();
    }

    timeT offset = m_pasteTime - earliestStartTime;

    for (Rosegarden::Clipboard::iterator i = m_clipboard->begin();
	 i != m_clipboard->end(); ++i) {

        TrackId newTrackId = m_composition->getSelectedTrack() 
            + (*i)->getTrack()
            - trackOffset;

        // needs to check for valid id
        if (newTrackId < m_composition->getMinTrackId() ||
            newTrackId > m_composition->getMaxTrackId()) continue;

	Segment *segment = new Segment(**i);
	segment->setStartTime(segment->getStartTime() + offset);
        segment->setTrack(newTrackId);
        m_composition->addSegment(segment);
	if (m_clipboard->isPartial()) {
	    segment->normalizeRests(segment->getStartTime(),
				    segment->getEndMarkerTime() + offset);
	}
	m_addedSegments.push_back(segment);
    }

    // User preference? Update song pointer position on paste
    m_composition->setPosition(latestEndTime 
                               + m_pasteTime 
                               - earliestStartTime);
    
    m_detached = false;
}

void
PasteSegmentsCommand::unexecute()
{
    for (unsigned int i = 0; i < m_addedSegments.size(); ++i) {
	m_composition->detachSegment(m_addedSegments[i]);
    }
    m_detached = true;
}
    

PasteEventsCommand::PasteEventsCommand(Rosegarden::Segment &segment,
				       Rosegarden::Clipboard *clipboard,
				       Rosegarden::timeT pasteTime,
				       PasteType pasteType) :
    BasicCommand(getGlobalName(), segment, pasteTime,
		 getEffectiveEndTime(segment, clipboard, pasteTime)),
    m_relayoutEndTime(getEndTime()),
    m_clipboard(clipboard),
    m_pasteType(pasteType)
{
    if (pasteType != OpenAndPaste) {

	// paste clef or key -> relayout to end

	if (clipboard->isSingleSegment()) {

	    Segment *s(clipboard->getSingleSegment());
	    for (Segment::iterator i = s->begin(); i != s->end(); ++i) {
		if ((*i)->isa(Rosegarden::Clef::EventType) ||
		    (*i)->isa(Rosegarden::Key::EventType)) {
		    m_relayoutEndTime = s->getEndTime();
		    break;
		}
	    }
	}
    }
}

PasteEventsCommand::PasteEventsCommand(Rosegarden::Segment &segment,
				       Rosegarden::Clipboard *clipboard,
				       Rosegarden::timeT pasteTime,
				       Rosegarden::timeT pasteEndTime,
				       PasteType pasteType) :
    BasicCommand(getGlobalName(), segment, pasteTime, pasteEndTime),
    m_relayoutEndTime(getEndTime()),
    m_clipboard(clipboard),
    m_pasteType(pasteType)
{
}

PasteEventsCommand::PasteTypeMap
PasteEventsCommand::getPasteTypes()
{
    static PasteTypeMap types;
    static bool haveTypes = false;
    if (!haveTypes) {
	types[Restricted] =
	    i18n("Paste into an existing gap [\"restricted\"]");
	types[Simple] =
	    i18n("Erase existing events to make room [\"simple\"]");
	types[OpenAndPaste] =
	    i18n("Move existing events out of the way [\"open-n-paste\"]");
	types[NoteOverlay] =
	    i18n("Overlay notes, tying against present notes [\"note-overlay\"]");
	types[MatrixOverlay] =
	    i18n("Overlay notes, ignoring present notes [\"matrix-overlay\"]");
    }
    return types;
}

timeT
PasteEventsCommand::getEffectiveEndTime(Rosegarden::Segment &segment,
					Rosegarden::Clipboard *clipboard,
					Rosegarden::timeT pasteTime)
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
	if (i == segment.end()) return segment.getEndTime();
	else return (*i)->getAbsoluteTime();
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

    if (!m_clipboard->isSingleSegment()) return;

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
	if ((*i)->get<Int>(BEAMED_GROUP_ID, groupId)) {
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
	if (!helper.removeRests(pasteTime, duration)) return;
	break;

    case Simple:
	destination->erase(destination->findTime(pasteTime),
			   destination->findTime(pasteTime + duration));
	break;

    case OpenAndPaste:
    {
	std::vector<Event *> copies;
	for (Segment::iterator i = destination->findTime(pasteTime);
	     i != destination->end(); ++i) {
	    Event *e = new Event(**i, (*i)->getAbsoluteTime() + duration);
	    if (e->has(BEAMED_GROUP_ID)) {
		e->set<Int>(BEAMED_GROUP_ID, groupIdMap[e->get<Int>(BEAMED_GROUP_ID)]);
	    }
	    copies.push_back(e);
	}

	destination->erase(destination->findTime(pasteTime),
			   destination->end());

	for (unsigned int i = 0; i < copies.size(); ++i) {
	    destination->insert(copies[i]);
	}

	break;
    }

    case NoteOverlay:
	for (Segment::iterator i = source->begin(); i != source->end(); ++i) {
	    if ((*i)->isa(Note::EventRestType)) continue;
	    if ((*i)->isa(Note::EventType)) {
		Event *e = new Event(**i,
				     (*i)->getAbsoluteTime() - origin + pasteTime,
				     (*i)->getDuration(),
				     (*i)->getSubOrdering(),
				     (*i)->getNotationAbsoluteTime() - origin + pasteTime,
				     (*i)->getNotationDuration());
		if (e->has(BEAMED_GROUP_ID)) {
		    e->set<Int>(BEAMED_GROUP_ID, groupIdMap[e->get<Int>(BEAMED_GROUP_ID)]);
		}
		helper.insertNote(e); // e is model event: we retain ownership of it
		delete e;
	    } else {
		Event *e = new Event
		    (**i, (*i)->getAbsoluteTime() - origin + pasteTime);
		if (e->has(BEAMED_GROUP_ID)) {
		    e->set<Int>(BEAMED_GROUP_ID, groupIdMap[e->get<Int>(BEAMED_GROUP_ID)]);
		}
		destination->insert(e);
	    }
	}

	return;

    case MatrixOverlay:

	for (Segment::iterator i = source->begin(); i != source->end(); ++i) {

	    if ((*i)->isa(Note::EventRestType)) continue;

	    Event *e = new Event
		(**i, (*i)->getAbsoluteTime() - origin + pasteTime);

	    if (e->has(BEAMED_GROUP_TYPE) &&
		e->get<String>(BEAMED_GROUP_TYPE) == GROUP_TYPE_BEAMED) {
		e->unset(BEAMED_GROUP_ID);
		e->unset(BEAMED_GROUP_TYPE);
	    }

	    if (e->has(BEAMED_GROUP_ID)) {
		e->set<Int>(BEAMED_GROUP_ID, groupIdMap[e->get<Int>(BEAMED_GROUP_ID)]);
	    }

	    destination->insert(e);
	}

	destination->normalizeRests
	    (source->getStartTime(), source->getEndTime());

	return;
    }

    RG_DEBUG << "PasteEventsCommand::modifySegment() - inserting\n";

    for (Segment::iterator i = source->begin(); i != source->end(); ++i) {
	Event *e = new Event
	    (**i, (*i)->getAbsoluteTime() - origin + pasteTime);
	if (e->has(BEAMED_GROUP_ID)) {
	    e->set<Int>(BEAMED_GROUP_ID, groupIdMap[e->get<Int>(BEAMED_GROUP_ID)]);
	}
	destination->insert(e);
    }

    destination->normalizeRests
	(source->getStartTime(), source->getEndTime());
}


EraseCommand::EraseCommand(EventSelection &selection) :
    BasicSelectionCommand(getGlobalName(), selection, true),
    m_selection(&selection),
    m_relayoutEndTime(getEndTime())
{
    // nothing else
}

void
EraseCommand::modifySegment()
{
    RG_DEBUG << "EraseCommand::modifySegment" << endl;

    std::vector<Event *> toErase;
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {
	
	if ((*i)->isa(Rosegarden::Clef::EventType) ||
	    (*i)->isa(Rosegarden::Key ::EventType)) {
	    m_relayoutEndTime = getSegment().getEndTime();
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

    for (unsigned int j = 0; j < toErase.size(); ++j) {
	getSegment().eraseSingle(toErase[j]);
    }

    getSegment().normalizeRests(getStartTime(), getEndTime());
}

timeT
EraseCommand::getRelayoutEndTime()
{
    return m_relayoutEndTime;
}



EventEditCommand::EventEditCommand(Rosegarden::Segment &segment,
				   Rosegarden::Event *eventToModify,
				   const Rosegarden::Event &newEvent) :
    BasicCommand(getGlobalName(),
		 segment,
		 std::min(eventToModify->getAbsoluteTime(),
			  newEvent.getAbsoluteTime()),
		 std::max(eventToModify->getAbsoluteTime() +
			  eventToModify->getDuration(),
			  newEvent.getAbsoluteTime() +
			  newEvent.getDuration()),
		 true), // bruteForceRedo
    m_oldEvent(eventToModify),
    m_newEvent(newEvent)
{
    // nothing else to see here
}

void
EventEditCommand::modifySegment()
{
    Segment &segment(getSegment());
    segment.eraseSingle(m_oldEvent);
    segment.insert(new Event(m_newEvent));
    segment.normalizeRests(getStartTime(), getEndTime());
}

// -------------------- SelectionPropertyCommand -----------------
//
//
SelectionPropertyCommand::SelectionPropertyCommand(
        Rosegarden::EventSelection *selection,
        const Rosegarden::PropertyName &property,
        Rosegarden::PropertyPattern pattern,
        int value1,
        int value2):
    BasicSelectionCommand(getGlobalName(), *selection, true),
    m_selection(selection),
    m_property(property),
    m_pattern(pattern),
    m_value1(value1),
    m_value2(value2)
{
}

void
SelectionPropertyCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i =
        m_selection->getSegmentEvents().begin();

    int count = 0;

    Rosegarden::timeT endTime = 0;
    Rosegarden::timeT startTime = 0;
    bool haveStart = false, haveEnd = false;

    // Get start and end times
    //
    for (;i != m_selection->getSegmentEvents().end(); ++i)
    {
        if ((*i)->getAbsoluteTime() < startTime || !haveStart) {
            startTime = (*i)->getAbsoluteTime();
	    haveStart = true;
	}
        
        if ((*i)->getAbsoluteTime() > endTime || !haveEnd) {
            endTime = (*i)->getAbsoluteTime();
	    haveEnd = true;
	}
    }

    double step = double(m_value1 - m_value2) / double(endTime - startTime);
    double lowStep = double(m_value2) / double(endTime - startTime);

    for (i = m_selection->getSegmentEvents().begin();
         i != m_selection->getSegmentEvents().end(); ++i)
    {
        if (m_pattern == Rosegarden::FlatPattern)
            (*i)->set<Rosegarden::Int>(m_property, m_value1);
        else if (m_pattern == Rosegarden::AlternatingPattern)
        {
            if (count % 2 == 0)
                (*i)->set<Rosegarden::Int>(m_property, m_value1);
            else
                (*i)->set<Rosegarden::Int>(m_property, m_value2);

        } else if (m_pattern == Rosegarden::CrescendoPattern)
        {
            (*i)->set<Rosegarden::Int>(m_property,
                                       m_value2 +
                                       int(step *
					   ((*i)->getAbsoluteTime() - startTime)));
        } else if (m_pattern == Rosegarden::DecrescendoPattern)
        {
            (*i)->set<Rosegarden::Int>(m_property,
                                       m_value1 -
                                       int(step *
					   ((*i)->getAbsoluteTime() - startTime)));
        } else if (m_pattern == Rosegarden::RingingPattern)
        {
            if (count % 2 == 0)
                (*i)->set<Rosegarden::Int>
                    (m_property,
                     m_value1 - int(step * 
				    ((*i)->getAbsoluteTime() - startTime)));
            else
            {
                int value = m_value2 - int(lowStep *
					   ((*i)->getAbsoluteTime() - startTime));
                if (value < 0) value = 0;

                (*i)->set<Rosegarden::Int>(m_property, value);
            }
        }

        count++;
    }
}


// -------------------- EventQuantizeCommand --------------------
//
//
EventQuantizeCommand::EventQuantizeCommand(Rosegarden::Segment &segment,
					   Rosegarden::timeT startTime,
					   Rosegarden::timeT endTime,
					   Rosegarden::Quantizer *quantizer):
    BasicCommand(getGlobalName(quantizer), segment, startTime, endTime,
		 true), // bruteForceRedo
    m_quantizer(quantizer),
    m_selection(0)
{
    // nothing else
}

EventQuantizeCommand::EventQuantizeCommand(Rosegarden::EventSelection &selection,
					   Rosegarden::Quantizer *quantizer):
    BasicCommand(getGlobalName(quantizer),
		 selection.getSegment(),
		 selection.getStartTime(),
		 selection.getEndTime(),
		 true), // bruteForceRedo
    m_quantizer(quantizer),
    m_selection(&selection)
{
    // nothing else
}

EventQuantizeCommand::EventQuantizeCommand(Rosegarden::Segment &segment,
					   Rosegarden::timeT startTime,
					   Rosegarden::timeT endTime,
					   QString configGroup,
					   bool notation):
    BasicCommand(getGlobalName(makeQuantizer(configGroup, notation)),
		 segment, startTime, endTime,
		 true), // bruteForceRedo
    m_selection(0),
    m_configGroup(configGroup)
{
    // nothing else -- m_quantizer set by makeQuantizer
}

EventQuantizeCommand::EventQuantizeCommand(Rosegarden::EventSelection &selection,
					   QString configGroup,
					   bool notation):
    BasicCommand(getGlobalName(makeQuantizer(configGroup, notation)),
		 selection.getSegment(),
		 selection.getStartTime(),
		 selection.getEndTime(),
		 true), // bruteForceRedo
    m_selection(&selection),
    m_configGroup(configGroup)
{
    // nothing else -- m_quantizer set by makeQuantizer
}

EventQuantizeCommand::~EventQuantizeCommand()
{
    delete m_quantizer;
}

QString
EventQuantizeCommand::getGlobalName(Rosegarden::Quantizer *quantizer)
{
    if (quantizer) {
	if (dynamic_cast<Rosegarden::NotationQuantizer *>(quantizer)) {
	    return i18n("Heuristic Notation &Quantize");
	} else {
	    return i18n("Grid &Quantize");
	}
    }

    return i18n("&Quantize...");
}

void
EventQuantizeCommand::modifySegment()
{
    Rosegarden::Profiler profiler("EventQuantizeCommand::modifySegment", true);

    Segment &segment = getSegment();
    SegmentNotationHelper helper(segment);

    bool rebeam = false;
    bool makeviable = false;
    bool decounterpoint = false;

    if (m_configGroup) {
 //!!! need way to decide whether to do these even if no config group (i.e. through args to the command)
	KConfig *config = kapp->config();
	config->setGroup(m_configGroup);

	rebeam = config->readBoolEntry("quantizerebeam", true);
	makeviable = config->readBoolEntry("quantizemakeviable", false);
	decounterpoint = config->readBoolEntry("quantizedecounterpoint", false);
    }

    if (m_selection) {
        m_quantizer->quantize(m_selection);

    } else {
	m_quantizer->quantize(&segment,
			      segment.findTime(getStartTime()),
			      segment.findTime(getEndTime()));
    }

    if (m_progressTotal > 0) {
	if (rebeam || makeviable || decounterpoint) {
	    emit incrementProgress(m_progressTotal / 2);
	    rgapp->refreshGUI(50);
	} else {
	    emit incrementProgress(m_progressTotal);
	    rgapp->refreshGUI(50);
	}
    }	    

    if (m_selection) {
	EventSelection::RangeTimeList ranges(m_selection->getRangeTimes());
	for (EventSelection::RangeTimeList::iterator i = ranges.begin();
	     i != ranges.end(); ++i) {
	    if (makeviable) {
		helper.makeNotesViable(i->first, i->second, true);
	    }
	    if (decounterpoint) {
		helper.deCounterpoint(i->first, i->second);
	    }
	    if (rebeam) {
		helper.autoBeam(i->first, i->second, GROUP_TYPE_BEAMED);
		helper.autoSlur(i->first, i->second, true);
	    }
	}
    } else {
	if (makeviable) {
	    helper.makeNotesViable(getStartTime(), getEndTime(), true);
	}
	if (decounterpoint) {
	    helper.deCounterpoint(getStartTime(), getEndTime());
	}
	if (rebeam) {
	    helper.autoBeam(getStartTime(), getEndTime(), GROUP_TYPE_BEAMED);
	    helper.autoSlur(getStartTime(), getEndTime(), true);
	}
    }

    if (m_progressTotal > 0) {
	if (rebeam || makeviable || decounterpoint) {
	    emit incrementProgress(m_progressTotal / 2);
	    rgapp->refreshGUI(50);
	}
    }	    
}

Rosegarden::Quantizer *
EventQuantizeCommand::makeQuantizer(QString configGroup,
				    bool notationDefault)
{
    //!!! Excessive duplication with
    // RosegardenQuantizeParameters::getQuantizer in widgets.cpp

    KConfig *config = kapp->config();
    config->setGroup(configGroup);

    Rosegarden::timeT defaultUnit = 
	Rosegarden::Note(Rosegarden::Note::Demisemiquaver).getDuration();
    
    int type = config->readNumEntry("quantizetype", notationDefault ? 2 : 0);
    Rosegarden::timeT unit = config->readNumEntry("quantizeunit",defaultUnit);
    bool notateOnly = config->readBoolEntry("quantizenotationonly", notationDefault);
    bool durations = config->readBoolEntry("quantizedurations", false);
    int simplicity = config->readNumEntry("quantizesimplicity", 13);
    int maxTuplet = config->readNumEntry("quantizemaxtuplet", 3);
    bool counterpoint = config->readNumEntry("quantizecounterpoint", false);
    bool articulate = config->readBoolEntry("quantizearticulate", true);
    int swing = config->readNumEntry("quantizeswing", 0);
    int iterate = config->readNumEntry("quantizeiterate", 100);

    m_quantizer = 0;

    if (type == 0) {
	if (notateOnly) {
	    m_quantizer = new Rosegarden::BasicQuantizer
		(Rosegarden::Quantizer::RawEventData,
		 Rosegarden::Quantizer::NotationPrefix,
		 unit, durations, swing, iterate);
	} else {
	    m_quantizer = new Rosegarden::BasicQuantizer
		(Rosegarden::Quantizer::RawEventData,
		 Rosegarden::Quantizer::RawEventData,
		 unit, durations, swing, iterate);
	}
    } else if (type == 1) {
	if (notateOnly) {
	    m_quantizer = new Rosegarden::LegatoQuantizer
		(Rosegarden::Quantizer::RawEventData,
		 Rosegarden::Quantizer::NotationPrefix, unit);
	} else {
	    m_quantizer = new Rosegarden::LegatoQuantizer
		(Rosegarden::Quantizer::RawEventData,
		 Rosegarden::Quantizer::RawEventData, unit);
	}
    } else {
	
	Rosegarden::NotationQuantizer *nq;

	if (notateOnly) {
	    nq = new Rosegarden::NotationQuantizer();
	} else {
	    nq = new Rosegarden::NotationQuantizer
		(Rosegarden::Quantizer::RawEventData,
		 Rosegarden::Quantizer::RawEventData);
	}

	nq->setUnit(unit);
	nq->setSimplicityFactor(simplicity);
	nq->setMaxTuplet(maxTuplet);
	nq->setContrapuntal(counterpoint);
	nq->setArticulate(articulate);

	m_quantizer = nq;
    }

    return m_quantizer;
}
    


// ---------------- Unquantize -----------
EventUnquantizeCommand::EventUnquantizeCommand(Rosegarden::Segment &segment,
					       Rosegarden::timeT startTime,
					       Rosegarden::timeT endTime,
					       Rosegarden::Quantizer *quantizer) :
    BasicCommand(i18n("Unquantize Events"), segment, startTime, endTime,
		 true), // bruteForceRedo
    m_quantizer(quantizer),
    m_selection(0)
{
    // nothing else
}

EventUnquantizeCommand::EventUnquantizeCommand(
        Rosegarden::EventSelection &selection,
        Rosegarden::Quantizer *quantizer) :
    BasicCommand(i18n("Unquantize Events"),
		 selection.getSegment(),
		 selection.getStartTime(),
		 selection.getEndTime(),
		 true), // bruteForceRedo
    m_quantizer(quantizer),
    m_selection(&selection)
{
    // nothing else
}

EventUnquantizeCommand::~EventUnquantizeCommand()
{
    delete m_quantizer;
}

QString
EventUnquantizeCommand::getGlobalName(Rosegarden::Quantizer *)
{
/*!!!
    if (quantizer) {
	switch (quantizer->getType()) {
	case Rosegarden::Quantizer::PositionQuantize:
	    return i18n("Position &Quantize");
	case Rosegarden::Quantizer::UnitQuantize:
	    return i18n("Unit &Quantize");
	case Rosegarden::Quantizer::NoteQuantize:
	    return i18n("Note &Quantize");
	case Rosegarden::Quantizer::LegatoQuantize:
	    return i18n("Smoothing &Quantize");
	}
    }
*/
    return i18n("&Quantize...");
}

void
EventUnquantizeCommand::modifySegment()
{
    Segment &segment = getSegment();

    if (m_selection) {

        m_quantizer->unquantize(m_selection);

    } else {
	m_quantizer->unquantize(&segment,
				segment.findTime(getStartTime()),
				segment.findTime(getEndTime()));
    }
}


//-----------------Collapse Notes Command-------------------
//
//
void
AdjustMenuCollapseNotesCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    timeT endTime = getEndTime();

    // This is really nasty stuff.  We can't go in forward direction
    // using the j-iterator trick because collapseNoteAggressively may
    // erase the following iterator as well as the preceding one.  We
    // can't go backward naively, because collapseNoteAggressively
    // erases i from the EventSelection now that it's a
    // SegmentObserver.  We need the fancy hybrid j-iterator-backward
    // technique applied to selections instead of segments.
    
    EventSelection::eventcontainer::iterator i =
	m_selection->getSegmentEvents().end();
    EventSelection::eventcontainer::iterator j = i;
    bool thisOne = false;

    while (i != m_selection->getSegmentEvents().begin()) {
	
	--j;
	
	if (thisOne) {
	    helper.collapseNoteAggressively(*i, endTime);
	}
	
	// rather than "true" one could perform a test to see
	// whether j pointed to a candidate for collapsing:
	thisOne = true;
	
	i = j;
    }
    
    if (thisOne) {
	helper.collapseNoteAggressively(*i, endTime);
    }
}



SetLyricsCommand::SetLyricsCommand(Segment *segment, QString newLyricData) :
    KNamedCommand(getGlobalName()),
    m_segment(segment),
    m_newLyricData(newLyricData)
{
    // nothing
}

SetLyricsCommand::~SetLyricsCommand()
{
    for (std::vector<Event *>::iterator i = m_oldLyricEvents.begin();
	 i != m_oldLyricEvents.end(); ++i) {
	delete *i;
    }
}

void
SetLyricsCommand::execute()
{
    // This and LyricEditDialog::unparse() are opposites that will
    // need to be kept in sync with any changes to one another.  (They
    // should really both be in a common lyric management class.)

    // first remove old lyric events
    
    Segment::iterator i = m_segment->begin();

    while (i != m_segment->end()) {

	Segment::iterator j = i;
	++j;

	if ((*i)->isa(Text::EventType)) {
	    std::string textType;
	    if ((*i)->get<String>(Text::TextTypePropertyName, textType) &&
		textType == Text::Lyric) {
		m_oldLyricEvents.push_back(new Event(**i));
		m_segment->erase(i);
	    }
	}

	i = j;
    }

    // now parse the new string

    QStringList barStrings =
	QStringList::split("/", m_newLyricData, true); // empties ok
    
    Rosegarden::Composition *comp = m_segment->getComposition();
    int barNo = comp->getBarNumber(m_segment->getStartTime());
    
    for (QStringList::Iterator bsi = barStrings.begin();
	 bsi != barStrings.end(); ++bsi) {

	NOTATION_DEBUG << "Parsing lyrics for bar number " << barNo << ": \"" << *bsi << "\"" << endl;

	std::pair<timeT, timeT> barRange = comp->getBarRange(barNo++);
	QString syllables = *bsi;
	syllables.replace(QRegExp("\\[\\d+\\] "), " ");
	QStringList syllableList = QStringList::split(" ", syllables); // no empties
	
	i = m_segment->findTime(barRange.first);
	timeT laterThan = barRange.first - 1;

	for (QStringList::Iterator ssi = syllableList.begin();
	     ssi != syllableList.end(); ++ssi) {

	    while (m_segment->isBeforeEndMarker(i) &&
		   (*i)->getAbsoluteTime() < barRange.second &&
		   (!(*i)->isa(Note::EventType) ||
		    (*i)->getNotationAbsoluteTime() <= laterThan ||
		    ((*i)->has(TIED_BACKWARD) &&
		     (*i)->get<Rosegarden::Bool>(TIED_BACKWARD)))) ++i;

	    timeT time = m_segment->getEndMarkerTime();
	    timeT notationTime = time;
	    if (m_segment->isBeforeEndMarker(i)) {
		time = (*i)->getAbsoluteTime();
		notationTime = (*i)->getNotationAbsoluteTime();
	    }

	    QString syllable = *ssi;
	    syllable.replace(QRegExp("~"), " ");
	    syllable = syllable.simplifyWhiteSpace();
	    if (syllable == "") continue;
	    laterThan = notationTime + 1;
	    if (syllable == ".") continue;

	    NOTATION_DEBUG << "Syllable \"" << syllable << "\" at time " << time <<  endl;

	    Text text(qstrtostr(syllable), Text::Lyric);
	    m_segment->insert(text.getAsEvent(time));
	}
    }
}

void
SetLyricsCommand::unexecute()
{
    // Before we inserted the new lyric events (in execute()), we
    // removed all the existing ones.  That means we know any lyric
    // events found now must have been inserted by execute(), so we
    // can safely remove them before restoring the old ones.
    
    Segment::iterator i = m_segment->begin();

    while (i != m_segment->end()) {

	Segment::iterator j = i;
	++j;

	if ((*i)->isa(Text::EventType)) {
	    std::string textType;
	    if ((*i)->get<String>(Text::TextTypePropertyName, textType) &&
		textType == Text::Lyric) {
		m_segment->erase(i);
	    }
	}

	i = j;
    }

    // Now restore the old ones and clear out the vector.

    for (std::vector<Event *>::iterator i = m_oldLyricEvents.begin();
	 i != m_oldLyricEvents.end(); ++i) {
	m_segment->insert(*i);
    }

    m_oldLyricEvents.clear();
}

    
void
TransposeCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if ((*i)->isa(Note::EventType)) {
	    long pitch = (*i)->get<Int>(PITCH);
	    pitch += m_semitones;
	    (*i)->set<Int>(PITCH, pitch); 
	    (*i)->unset(ACCIDENTAL);
	}
    }
}


RescaleCommand::RescaleCommand(EventSelection &sel,
			       timeT newDuration,
			       bool closeGap) :
    BasicCommand(getGlobalName(), sel.getSegment(),
		 sel.getStartTime(),
		 closeGap ? sel.getSegment().getEndMarkerTime() : sel.getEndTime(),
		 true),
    m_selection(&sel),
    m_oldDuration(sel.getTotalDuration()),
    m_newDuration(newDuration),
    m_closeGap(closeGap)
{
    // nothing else
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
    if (m_oldDuration == m_newDuration) return;

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
	    toInsert.push_back(new Event(**i, (*i)->getAbsoluteTime() + diff));
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


MoveCommand::MoveCommand(Segment &s, timeT delta, bool useNotationTimings,
			 EventSelection &sel) :
    BasicCommand(getGlobalName(), s,
		 delta < 0 ? sel.getStartTime() + delta : sel.getStartTime(),
		 delta < 0 ? sel.getEndTime()+1 : sel.getEndTime()+1 + delta,
		 true),
    m_selection(&sel),
    m_delta(delta),
    m_useNotationTimings(useNotationTimings),
    m_lastInsertedEvent(0)
{
    // nothing else
}

QString
MoveCommand::getGlobalName(Rosegarden::timeT delta)
{
    if (delta == 0) {
	return "&Move Events";
    } else if (delta < 0) {
	return "&Move Events Back";
    } else {
	return "&Move Events Forward";
    }
}

void
MoveCommand::modifySegment()
{
    RG_DEBUG << "MoveCommand::modifySegment: delta is " << m_delta
	     << ", useNotationTimings " << m_useNotationTimings
	     << ", start time " << m_selection->getStartTime()
	     << ", end time " << m_selection->getEndTime() << endl;

    std::vector<Event *> toErase;
    std::vector<Event *> toInsert;

    timeT a0 = m_selection->getStartTime();
    timeT a1 = m_selection->getEndTime();
    timeT b0 = a0 + m_delta;
    timeT b1 = b0 + (a1 - a0);

    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if ((*i)->isa(Note::EventRestType)) continue;

	toErase.push_back(*i);
	timeT newTime =
	    (m_useNotationTimings ?
	     (*i)->getNotationAbsoluteTime() : (*i)->getAbsoluteTime()) + m_delta;

	Event *e;
	if (m_useNotationTimings) {
	    e = new Event(**i, newTime, (*i)->getDuration(), (*i)->getSubOrdering(),
			  newTime, (*i)->getNotationDuration());
	} else {
	    e = new Event(**i, newTime);
	}

	toInsert.push_back(e);
    }

    Segment &segment(m_selection->getSegment());

    for (unsigned int j = 0; j < toErase.size(); ++j) {
	Segment::iterator jtr(segment.findSingle(toErase[j]));
	if (jtr != segment.end()) segment.erase(jtr);
    }

    for (unsigned int j = 0; j < toInsert.size(); ++j) {

	Segment::iterator jtr = segment.end();

	// somewhat like the NoteOverlay part of PasteEventsCommand::modifySegment
/* nah -- let's do a de-counterpoint afterwards perhaps
	if (m_useNotationTimings && toInsert[j]->isa(Note::EventType)) {
	    long pitch = 0;
	    Accidental explicitAccidental = NoAccidental;
	    toInsert[j]->get<String>(ACCIDENTAL, explicitAccidental);
	    if (toInsert[j]->get<Int>(PITCH, pitch)) {
		jtr = SegmentNotationHelper(segment).insertNote
		    (toInsert[j]->getAbsoluteTime(),
		     Note::getNearestNote(toInsert[j]->getDuration()),
		     pitch, explicitAccidental);
		delete toInsert[j];
		toInsert[j] = *jtr;
	    }
	} else {
*/
	    jtr = segment.insert(toInsert[j]);
//	}

        // insert new event back into selection
        m_selection->addEvent(toInsert[j]);

	if (jtr != segment.end()) m_lastInsertedEvent = toInsert[j];
    }

    if (m_useNotationTimings) {
	SegmentNotationHelper(segment).deCounterpoint(b0, b1);
    }

    segment.normalizeRests(a0, a1);
    segment.normalizeRests(b0, b1);
}
   

MoveAcrossSegmentsCommand::MoveAcrossSegmentsCommand(Rosegarden::Segment &,
						     Rosegarden::Segment &secondSegment,
						     Rosegarden::timeT newStartTime,
						     bool notation,
						     Rosegarden::EventSelection &selection) :
    KMacroCommand(getGlobalName()),
    m_clipboard(new Rosegarden::Clipboard())
{
    addCommand(new CutCommand(selection, m_clipboard));

    timeT newEndTime = newStartTime + selection.getEndTime() - selection.getStartTime();
    Segment::iterator i = secondSegment.findTime(newEndTime);
    if (i == secondSegment.end()) newEndTime = secondSegment.getEndTime();
    else newEndTime = (*i)->getAbsoluteTime();

    addCommand(new PasteEventsCommand(secondSegment, m_clipboard,
				      newStartTime,
				      newEndTime,
				      notation ?
				      PasteEventsCommand::NoteOverlay :
				      PasteEventsCommand::MatrixOverlay));
}

MoveAcrossSegmentsCommand::~MoveAcrossSegmentsCommand()
{
    delete m_clipboard;
}

QString
MoveAcrossSegmentsCommand::getGlobalName()
{
    return i18n("&Move Events to Other Segment");
}



void
ChangeVelocityCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if ((*i)->isa(Note::EventType)) {

	    long velocity = 100;
	    (*i)->get<Int>(VELOCITY, velocity);

	    // round velocity up to the next multiple of delta
	    velocity /= m_delta;
	    velocity *= m_delta;
	    velocity += m_delta;

	    if (velocity < 0) velocity = 0;
	    if (velocity > 127) velocity = 127;
	    (*i)->set<Int>(VELOCITY, velocity); 
	}
    }
}

// ------------------- Markers -------------------
//
//


AddMarkerCommand::AddMarkerCommand(Rosegarden::Composition *comp,
                                   Rosegarden::timeT time,
                                   const std::string &name,
                                   const std::string &description):
    KNamedCommand(getGlobalName()),
    m_composition(comp),
    m_detached(true)
{
    m_marker = new Rosegarden::Marker(time, name, description);
}

AddMarkerCommand::~AddMarkerCommand()
{
    if (m_detached) delete m_marker;
}

void
AddMarkerCommand::execute()
{
    m_composition->addMarker(m_marker);
    m_detached = false;
}

void
AddMarkerCommand::unexecute()
{
    m_composition->detachMarker(m_marker);
    m_detached = true;
}


RemoveMarkerCommand::RemoveMarkerCommand(Rosegarden::Composition *comp,
                                         Rosegarden::timeT time,
                                         const std::string &name,
                                         const std::string &description):
    KNamedCommand(getGlobalName()),
    m_composition(comp),
    m_marker(0),
    m_time(time),
    m_name(name),
    m_descr(description),
    m_detached(false)
{
}

RemoveMarkerCommand::~RemoveMarkerCommand()
{
    if (m_detached) delete m_marker;
}

void
RemoveMarkerCommand::execute()
{
    Rosegarden::Composition::markercontainer markers = 
        m_composition->getMarkers();

    Rosegarden::Composition::markerconstiterator it = markers.begin();

    for (; it != markers.end(); ++it)
    {
        if ((*it)->getTime() == m_time && 
            (*it)->getName() == m_name && 
            (*it)->getDescription() == m_descr)
        {
            m_marker = (*it);
            m_composition->detachMarker(m_marker);
	    m_detached = true;
            return;
        }
    }
}

void
RemoveMarkerCommand::unexecute()
{
    if (m_marker) m_composition->addMarker(m_marker);
    m_detached = false;
}

ModifyMarkerCommand::ModifyMarkerCommand(Rosegarden::Composition *comp,
                                         Rosegarden::timeT time,
                                         Rosegarden::timeT newTime,
                                         const std::string &name,
                                         const std::string &des):
    KNamedCommand(getGlobalName()),
    m_composition(comp),
    m_time(time),
    m_newTime(newTime),
    m_name(name),
    m_description(des),
    m_oldName(""),
    m_oldDescription("")
{
}

ModifyMarkerCommand::~ModifyMarkerCommand()
{
}

void
ModifyMarkerCommand::execute()
{
    Rosegarden::Composition::markercontainer markers = 
        m_composition->getMarkers();

    Rosegarden::Composition::markerconstiterator it = markers.begin();

    for (; it != markers.end(); ++it)
    {
        if ((*it)->getTime() == m_time)
        {
            if (m_oldName.empty()) m_oldName = (*it)->getName();
            if (m_oldDescription.empty()) 
                m_oldDescription = (*it)->getDescription();

            (*it)->setName(m_name);
            (*it)->setDescription(m_description);
            (*it)->setTime(m_newTime);
            return;
        }
    }
}

void
ModifyMarkerCommand::unexecute()
{
    Rosegarden::Composition::markercontainer markers = 
        m_composition->getMarkers();

    Rosegarden::Composition::markerconstiterator it = markers.begin();

    for (; it != markers.end(); ++it)
    {
        if ((*it)->getTime() == m_newTime)
        {
            (*it)->setName(m_oldName);
            (*it)->setDescription(m_oldDescription);
            (*it)->setTime(m_time);
        }
    }
}


void
SetTriggerCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if (!m_notesOnly || (*i)->isa(Note::EventType)) {
	    (*i)->set<Int>(TRIGGER_SEGMENT_ID, m_triggerSegmentId);
	    (*i)->set<Rosegarden::Bool>(TRIGGER_SEGMENT_RETUNE, m_retune);
	    (*i)->set<Rosegarden::String>(TRIGGER_SEGMENT_ADJUST_TIMES, m_timeAdjust);
	    if (m_mark != Rosegarden::Marks::NoMark) {
		Rosegarden::Marks::addMark(**i, m_mark, true);
	    }
	}
    }

    // Update the rec references here, without bothering to do so in unexecute
    // or in ClearTriggersCommand -- because it doesn't matter if a trigger
    // has references to segments that don't actually trigger it, whereas it
    // does matter if it loses a reference to something that does

    Rosegarden::TriggerSegmentRec *rec =
	m_selection->getSegment().getComposition()->getTriggerSegmentRec
	(m_triggerSegmentId);

    if (rec) rec->updateReferences();
}

void
ClearTriggersCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	(*i)->unset(TRIGGER_SEGMENT_ID);
	(*i)->unset(TRIGGER_SEGMENT_RETUNE);
	(*i)->unset(TRIGGER_SEGMENT_ADJUST_TIMES);
    }
}


InsertTriggerNoteCommand::InsertTriggerNoteCommand(Rosegarden::Segment &segment,
						   Rosegarden::timeT time,
						   Rosegarden::Note note,
						   int pitch,
						   int velocity,
						   NoteStyleName noteStyle,
						   Rosegarden::TriggerSegmentId id,
						   bool retune,
						   std::string timeAdjust,
						   Rosegarden::Mark mark) :
    BasicCommand(i18n("Insert Trigger Note"), segment,
		 time, time + note.getDuration()),
    m_time(time),
    m_note(note),
    m_pitch(pitch),
    m_velocity(velocity),
    m_noteStyle(noteStyle),
    m_id(id),
    m_retune(retune),
    m_timeAdjust(timeAdjust),
    m_mark(mark)
{
    // nothing
}

InsertTriggerNoteCommand::~InsertTriggerNoteCommand()
{
    // nothing
}

void
InsertTriggerNoteCommand::modifySegment()
{
    // Insert via a model event, so as to apply the note style.
    // This is a subset of the work done by NoteInsertionCommand
    
    Event *e = new Event(Note::EventType, m_time, m_note.getDuration());

    e->set<Int>(PITCH, m_pitch);
    e->set<Int>(VELOCITY, m_velocity);

    if (m_noteStyle != NoteStyleFactory::DefaultStyle) {
	e->set<String>(NotationProperties::NOTE_STYLE, m_noteStyle);
    }

    e->set<Int>(TRIGGER_SEGMENT_ID, m_id);
    e->set<Rosegarden::Bool>(TRIGGER_SEGMENT_RETUNE, m_retune);
    e->set<Rosegarden::String>(TRIGGER_SEGMENT_ADJUST_TIMES, m_timeAdjust);

    if (m_mark != Rosegarden::Marks::NoMark) {
	Rosegarden::Marks::addMark(*e, m_mark, true);
    }

    Segment &s(getSegment());
    Segment::iterator i = Rosegarden::SegmentMatrixHelper(s).insertNote(e);

    Segment::iterator j = i;
    while (++j != s.end()) {
	if ((*j)->getAbsoluteTime() >
	    (*i)->getAbsoluteTime() + (*i)->getDuration()) break;
	if ((*j)->isa(Note::EventType)) {
	    if ((*j)->getAbsoluteTime() ==
		(*i)->getAbsoluteTime() + (*i)->getDuration()) {
		if ((*j)->has(TIED_BACKWARD) && (*j)->get<Rosegarden::Bool>(TIED_BACKWARD) &&
		    ((*j)->get<Int>(PITCH) == m_pitch)) {
		    (*i)->set<Rosegarden::Bool>(TIED_FORWARD, true);
		}
	    }
	}
    }

    Rosegarden::TriggerSegmentRec *rec =
	getSegment().getComposition()->getTriggerSegmentRec(m_id);

    if (rec) rec->updateReferences();
}


void
SetNoteTypeCommand::modifySegment()
{
    std::vector<Event *> toErase;
    std::vector<Event *> toInsert;
    
    EventSelection::eventcontainer::iterator i;
    timeT endTime = getEndTime();

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if ((*i)->isa(Rosegarden::Note::EventType)) {
	    toErase.push_back(*i);
	    
	    Event *e;
	    if (m_notationOnly) {
		e = new Event(**i,
			      (*i)->getAbsoluteTime(),
			      (*i)->getDuration(),
			      (*i)->getSubOrdering(),
			      (*i)->getNotationAbsoluteTime(),
			      Rosegarden::Note(m_type).getDuration());
	    } else {
		e = new Event(**i,
			      (*i)->getNotationAbsoluteTime(),
			      Rosegarden::Note(m_type).getDuration());
	    }
		
	    if (e->getNotationAbsoluteTime() + e->getNotationDuration() > endTime) {
		endTime = e->getNotationAbsoluteTime() + e->getNotationDuration();
	    }

	    toInsert.push_back(e);
	}
    }

    for (std::vector<Event *>::iterator i = toErase.begin(); i != toErase.end(); ++i) {
	m_selection->getSegment().eraseSingle(*i);
    }

    for (std::vector<Event *>::iterator i = toInsert.begin(); i != toInsert.end(); ++i) {
	m_selection->getSegment().insert(*i);
	m_selection->addEvent(*i);
    }

    m_selection->getSegment().normalizeRests(getStartTime(), endTime);
}

void
AddDotCommand::modifySegment()
{
    std::vector<Event *> toErase;
    std::vector<Event *> toInsert;
    
    EventSelection::eventcontainer::iterator i;
    timeT endTime = getEndTime();

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if ((*i)->isa(Rosegarden::Note::EventType)) {

	    Rosegarden::Note note = Rosegarden::Note::getNearestNote
		((*i)->getNotationDuration());
	    int dots = note.getDots();
	    if (++dots > 2) dots = 0;

	    toErase.push_back(*i);

	    Event *e;

	    if (m_notationOnly) {
		e = new Event(**i,
			      (*i)->getAbsoluteTime(),
			      (*i)->getDuration(),
			      (*i)->getSubOrdering(),
			      (*i)->getNotationAbsoluteTime(),
			      Rosegarden::Note(note.getNoteType(),
					       dots).getDuration());

	    } else {
		e = new Event(**i,
			      (*i)->getNotationAbsoluteTime(),
			      Rosegarden::Note(note.getNoteType(),
					       dots).getDuration());
	    }

	    if (e->getNotationAbsoluteTime() + e->getNotationDuration() > endTime) {
		endTime = e->getNotationAbsoluteTime() + e->getNotationDuration();
	    }

	    toInsert.push_back(e);
	}
    }

    for (std::vector<Event *>::iterator i = toErase.begin(); i != toErase.end(); ++i) {
	m_selection->getSegment().eraseSingle(*i);
    }

    for (std::vector<Event *>::iterator i = toInsert.begin(); i != toInsert.end(); ++i) {
	m_selection->getSegment().insert(*i);
	m_selection->addEvent(*i);
    }

    m_selection->getSegment().normalizeRests(getStartTime(), endTime);
}

