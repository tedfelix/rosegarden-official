/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#include "NotationTypes.h"
#include "Selection.h"
#include "SegmentNotationHelper.h"
#include "BaseProperties.h"
#include "Clipboard.h"

#include "notationproperties.h"
#include "segmentcommands.h"

#include "rosestrings.h"
#include "rosedebug.h"
#include <iostream>

using Rosegarden::Segment;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::Event;
using Rosegarden::timeT;
using Rosegarden::Note;
using Rosegarden::Clef;
using Rosegarden::Int;
using Rosegarden::String;
using Rosegarden::Accidental;
using Rosegarden::Accidentals::NoAccidental;
using Rosegarden::Indication;
using Rosegarden::EventSelection;
using Rosegarden::SegmentSelection;

using namespace Rosegarden::BaseProperties;

using std::string;
using std::cerr;
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
    // we shift all the events from m_fromTime to the end of the
    // segment back so that they start at m_toTime instead of m_fromTime
    assert(m_fromTime >= m_toTime);
    if (m_fromTime == m_toTime) return;

    std::vector<Event *> events;
    timeT timeDifference = m_toTime - m_fromTime;

    for (Segment::iterator i = m_segment->findTime(m_fromTime);
	 i != m_segment->end(); ++i) {
	events.push_back(new Event
			 (**i, (*i)->getAbsoluteTime() + timeDifference));
    }

    timeT oldEndTime = m_segment->getEndTime();
    m_segment->erase(m_segment->findTime(m_toTime), m_segment->end());
    
    for (unsigned int i = 0; i < events.size(); ++i) {
	m_segment->insert(events[i]);
    }

    m_segment->fillWithRests(oldEndTime);
}

void
CutAndCloseCommand::CloseCommand::unexecute()
{
    // we shift all the events from m_toTime to the end of the
    // segment forward so that they start at m_fromTime instead of m_toTime
    assert(m_fromTime >= m_toTime);
    if (m_fromTime == m_toTime) return;

    std::vector<Event *> events;
    timeT timeDifference = m_fromTime - m_toTime;

    timeT segmentEndTime = m_segment->getEndTime();
    timeT copyFromEndTime = segmentEndTime - timeDifference;

    for (Segment::iterator i = m_segment->findTime(m_toTime);
	 i != m_segment->findNearestTime(copyFromEndTime); ++i) {
	events.push_back(new Event
			 (**i, (*i)->getAbsoluteTime() + timeDifference));
    }

    m_segment->erase(m_segment->findTime(m_toTime), m_segment->end());
    
    for (unsigned int i = 0; i < events.size(); ++i) {
	m_segment->insert(events[i]);
    }

    m_segment->normalizeRests(m_toTime, m_fromTime);
}  


CopyCommand::CopyCommand(EventSelection &selection,
			 Rosegarden::Clipboard *clipboard) :
    XKCommand(getGlobalName()),
    m_targetClipboard(clipboard)
{
    m_sourceClipboard = new Rosegarden::Clipboard;
    m_sourceClipboard->newSegment(&selection)->setLabel
	(selection.getSegment().getLabel() + " " + qstrtostr(i18n("(excerpt)")));
}

CopyCommand::CopyCommand(SegmentSelection &selection,
			 Rosegarden::Clipboard *clipboard) :
    XKCommand(getGlobalName()),
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
    Rosegarden::Clipboard temp(*m_targetClipboard);
    m_targetClipboard->copyFrom(m_sourceClipboard);
    m_sourceClipboard->copyFrom(&temp);
}

void
CopyCommand::unexecute()
{
    Rosegarden::Clipboard temp(*m_sourceClipboard);
    m_sourceClipboard->copyFrom(m_targetClipboard);
    m_targetClipboard->copyFrom(&temp);
}


PasteSegmentsCommand::PasteSegmentsCommand(Rosegarden::Composition *composition,
					   Rosegarden::Clipboard *clipboard,
					   Rosegarden::timeT pasteTime) :
    XKCommand(getGlobalName()),
    m_composition(composition),
    m_clipboard(clipboard),
    m_pasteTime(pasteTime)
{
    // nothing else
}

PasteSegmentsCommand::~PasteSegmentsCommand()
{
    for (unsigned int i = 0; i < m_addedSegments.size(); ++i) {
	delete m_addedSegments[i];
    }
}

void
PasteSegmentsCommand::execute()
{
    if (m_clipboard->isEmpty()) return;

    if (m_addedSegments.size() > 0) {
	// been here before
	for (unsigned int i = 0; i < m_addedSegments.size(); ++i) {
	    m_composition->addSegment(m_addedSegments[i]);
	}
	return;
    }

    // We want to paste such that the earliest Segment starts at
    // m_pasteTime and the others start at the same times relative
    // to that as they did before

    timeT earliestStartTime = 0;

    for (Rosegarden::Clipboard::iterator i = m_clipboard->begin();
	 i != m_clipboard->end(); ++i) {

	if (i == m_clipboard->begin() ||
	    (*i)->getStartTime() < earliestStartTime) {
	    earliestStartTime = (*i)->getStartTime();
	}
    }

    timeT offset = m_pasteTime - earliestStartTime;

    for (Rosegarden::Clipboard::iterator i = m_clipboard->begin();
	 i != m_clipboard->end(); ++i) {

	Segment *segment = new Segment(**i);
	segment->setStartTime(segment->getStartTime() + offset);
	m_composition->addSegment(segment);
	m_addedSegments.push_back(segment);
    }
}

void
PasteSegmentsCommand::unexecute()
{
    for (unsigned int i = 0; i < m_addedSegments.size(); ++i) {
	m_composition->detachSegment(m_addedSegments[i]);
    }
}
    


PasteEventsCommand::PasteType
PasteEventsCommand::m_defaultPaste = PasteEventsCommand::Restricted;

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

timeT
PasteEventsCommand::getEffectiveEndTime(Rosegarden::Segment &segment,
					Rosegarden::Clipboard *clipboard,
					Rosegarden::timeT pasteTime)
{
    if (!clipboard->isSingleSegment()) return pasteTime;
    
    timeT d = clipboard->getSingleSegment()->getEndTime() -
#ifdef OLD_SEGMENT_API
 	      clipboard->getSingleSegment()->getFirstEventTime();
#else
 	      clipboard->getSingleSegment()->getStartTime();
#endif

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
#ifdef OLD_SEGMENT_API
    timeT origin = source->getFirstEventTime();
    timeT duration = source->getDuration() - origin;
#else
    timeT origin = source->getStartTime();
    timeT duration = source->getEndTime() - origin;
#endif

    kdDebug(KDEBUG_AREA) << "NotationView::slotEditPaste: paste time is " << pasteTime << ", origin is " << origin << ", duration is " << duration << endl;

    SegmentNotationHelper helper(getSegment());
    return helper.removeRests(pasteTime, duration, true);
}


void
PasteEventsCommand::modifySegment()
{
    if (!m_clipboard->isSingleSegment()) return;

    Segment *source = m_clipboard->getSingleSegment();

    timeT pasteTime = getStartTime();
#ifdef OLD_SEGMENT_API
    timeT origin = source->getFirstEventTime();
#else
    timeT origin = source->getStartTime();
#endif
    timeT duration = source->getEndTime() - origin;
    
    Segment *destination(&getSegment());
    SegmentNotationHelper helper(*destination);

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
	    Event *e = new Event(**i, e->getAbsoluteTime() + duration);
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
		long pitch = 0;
		Accidental explicitAccidental = NoAccidental;
		(*i)->get<String>(ACCIDENTAL, explicitAccidental);
		if ((*i)->get<Int>(PITCH, pitch)) {
		    helper.insertNote
			((*i)->getAbsoluteTime() - origin + pasteTime,
			 Note::getNearestNote((*i)->getDuration()),
			 pitch, explicitAccidental);
		}
	    } else {
		Event *e = new Event
		    (**i, e->getAbsoluteTime() - origin + pasteTime);
		destination->insert(e);
	    }
	}
	break;

    case MatrixOverlay:

	for (Segment::iterator i = source->begin(); i != source->end(); ++i) {

	    if ((*i)->isa(Note::EventRestType)) continue;

	    Event *e = new Event
		(**i, e->getAbsoluteTime() - origin + pasteTime);
	    destination->insert(e);
	}
	break;
    }

    for (Segment::iterator i = source->begin(); i != source->end(); ++i) {
	Event *e = new Event
	    (**i, (*i)->getAbsoluteTime() - origin + pasteTime);
	destination->insert(e);
    }

    destination->normalizeRests
#ifdef OLD_SEGMENT_API
	(source->getFirstEventTime(), source->getEndTime());
#else
	(source->getStartTime(), source->getEndTime());
#endif
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



EventQuantizeCommand::EventQuantizeCommand(Rosegarden::Segment &segment,
					   Rosegarden::timeT startTime,
					   Rosegarden::timeT endTime,
					   Rosegarden::Quantizer quantizer) :
    BasicCommand(getGlobalName(&quantizer), segment, startTime, endTime,
		 true), // bruteForceRedo
    m_quantizer(quantizer),
    m_selection(0)
{
    // nothing else
}

EventQuantizeCommand::EventQuantizeCommand(Rosegarden::EventSelection &selection,
					   Rosegarden::Quantizer quantizer) :
    BasicCommand(getGlobalName(&quantizer),
		 selection.getSegment(),
		 selection.getStartTime(),
		 selection.getEndTime(),
		 true), // bruteForceRedo
    m_quantizer(quantizer),
    m_selection(&selection)
{
    // nothing else
}

QString
EventQuantizeCommand::getGlobalName(Rosegarden::Quantizer *quantizer)
{
    if (quantizer) {
	switch (quantizer->getType()) {
	case Rosegarden::Quantizer::PositionQuantize:
	    return "Position &Quantize";
	case Rosegarden::Quantizer::UnitQuantize:
	    return "Unit &Quantize";
	case Rosegarden::Quantizer::NoteQuantize:
	    return "Note &Quantize";
	case Rosegarden::Quantizer::LegatoQuantize:
	    return "Smoothing &Quantize";
	}
    }

    return "&Quantize...";
}

void
EventQuantizeCommand::modifySegment()
{
    Segment &segment = getSegment();

    if (m_selection) {

	// Attempt to handle non-contiguous selections.

	// We have to be a bit careful here, because the rest-
	// normalisation that's carried out as part of a quantize
	// process is liable to replace the event that follows
	// the quantized range.

	typedef std::vector<std::pair<Segment::iterator,
				      Segment::iterator> > RangeList;
	RangeList ranges;

	Segment::iterator i = segment.findTime(getStartTime());
	Segment::iterator j;
	Segment::iterator k = segment.findTime(getEndTime());

	while (j != k) {
	
	    for (j = i; j != k && m_selection->contains(*j); ++j);

	    if (j != i) {
		ranges.push_back(RangeList::value_type(i, j));
		
		for (i = j; i != k && !m_selection->contains(*i); ++i);
		j = i;
	    }
	}

	for (RangeList::iterator r = ranges.begin(); r != ranges.end(); ++r) {
	    m_quantizer.quantize(&segment, r->first, r->second);
	}

    } else {
	m_quantizer.quantize(&segment,
			     segment.findTime(getStartTime()),
			     segment.findTime(getEndTime()));
    }
}

