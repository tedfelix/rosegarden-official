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

using namespace Rosegarden::BaseProperties;

using std::string;
using std::cerr;
using std::endl;



CutCommand::CutCommand(EventSelection &selection,
					 Rosegarden::Clipboard *clipboard) :
    CompoundCommand(name())
{
    addCommand(new CopyCommand(selection, clipboard));
    addCommand(new EraseCommand(selection));
}

CopyCommand::CopyCommand(EventSelection &selection,
					   Rosegarden::Clipboard *clipboard) :
    KCommand(name()),
    m_targetClipboard(clipboard)
{
    m_sourceClipboard = new Rosegarden::Clipboard;
    (void)m_sourceClipboard->newSegment(&selection);
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

PasteCommand::PasteType
PasteCommand::m_defaultPaste = PasteCommand::Restricted;

PasteCommand::PasteCommand(Rosegarden::Segment &segment,
					   Rosegarden::Clipboard *clipboard,
					   Rosegarden::timeT pasteTime,
					   PasteType pasteType) :
    BasicCommand(name(), segment, pasteTime,
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
PasteCommand::getEffectiveEndTime(Rosegarden::Segment &segment,
				  Rosegarden::Clipboard *clipboard,
				  Rosegarden::timeT pasteTime)
{
    if (!clipboard->isSingleSegment()) return pasteTime;
    
    timeT d = clipboard->getSingleSegment()->getEndTime() -
 	      clipboard->getSingleSegment()->getFirstEventTime();

    if (m_pasteType == OpenAndPaste) {
	return segment.getEndTime() + d;
    } else {
	Segment::iterator i = segment.findTime(pasteTime + d);
	if (i == segment.end()) return segment.getEndTime();
	else return (*i)->getAbsoluteTime();
    }
}

timeT
PasteCommand::getRelayoutEndTime()
{
    return m_relayoutEndTime;
}

bool
PasteCommand::isPossible() 
{
    if (m_clipboard->isEmpty() || !m_clipboard->isSingleSegment()) {
	return false;
    }
    
    if (m_pasteType != Restricted) {
	return true;
    }

    Segment *source = m_clipboard->getSingleSegment();

    timeT pasteTime = getBeginTime();
    timeT origin = source->getFirstEventTime();
    timeT duration = source->getDuration() - origin;

    SegmentNotationHelper helper(getSegment());
    return helper.removeRests(pasteTime, duration, true);
}


void
PasteCommand::modifySegment()
{
    if (!m_clipboard->isSingleSegment()) return;

    Segment *source = m_clipboard->getSingleSegment();

    timeT pasteTime = getBeginTime();
    timeT origin = source->getFirstEventTime();
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
	    (**i, e->getAbsoluteTime() - origin + pasteTime);
	destination->insert(e);

	if (e->isa(Note::EventType)) {
	    destination->normalizeRests
		(e->getAbsoluteTime(),
		 e->getAbsoluteTime() + e->getDuration());
	}
    }
}


EraseCommand::EraseCommand(EventSelection &selection) :
    BasicSelectionCommand(name(), selection, true),
    m_selection(&selection),
    m_relayoutEndTime(getEndTime())
{
    // nothing else
}

void
EraseCommand::modifySegment()
{
    std::vector<Event *> eventsToErase;
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {
	
	if ((*i)->isa(Rosegarden::Clef::EventType) ||
	    (*i)->isa(Rosegarden::Key ::EventType)) {
	    m_relayoutEndTime = getSegment().getEndTime();
	}

	// Erasing rests is dangerous and unnecessary -- when we erase
	// a note or rest, any surrounding rests may be erased and
	// collapsed as well, which means we may blow up when attempting
	// subsequently to erase one of those explicitly.  If we never
	// explicitly erase a rest, we should be fine.

	//!!! unless we _really_ want to do it, and should find some
	// way to make it work at the SegmentNotationHelper level -- 
	// perhaps we should anyway (eraseEvents method?) -- we
	// desperately need a method to make all the rests in a
	// region valid and "canonical" 

	//!!! We now have it (normalizeRests does this), so use it

	if (!(*i)->isa(Rosegarden::Note::EventRestType)) {
	    eventsToErase.push_back(*i);
	}
    }

    SegmentNotationHelper helper(getSegment());
    for (unsigned int j = 0; j < eventsToErase.size(); ++j) {
	helper.deleteEvent(eventsToErase[j], false);
    }
}

timeT
EraseCommand::getRelayoutEndTime()
{
    return m_relayoutEndTime;
}
