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

#include "notationcommands.h"
#include "notationview.h"
#include "rosegardenguidoc.h"

#include "Selection.h"
#include "SegmentNotationHelper.h"
#include "BaseProperties.h"
#include "Clipboard.h"
#include "notationproperties.h"

#include "rosedebug.h"
#include <iostream>
#include <cctype>

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
using Rosegarden::NotationDisplayPitch;
using Rosegarden::EventSelection;

using namespace Rosegarden::BaseProperties;

using std::string;
using std::cerr;
using std::endl;


NoteInsertionCommand::NoteInsertionCommand(Segment &segment, timeT time,
                                           timeT endTime, Note note, int pitch,
                                           Accidental accidental) :
    BasicCommand("Insert Note", segment, time, endTime),
    m_note(note),
    m_pitch(pitch),
    m_accidental(accidental),
    m_lastInsertedEvent(0)
{
    // nothing
}

NoteInsertionCommand::~NoteInsertionCommand()
{
    // nothing
}

void
NoteInsertionCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());

    Segment::iterator i =
        helper.insertNote(getBeginTime(), m_note, m_pitch, m_accidental);
    if (i != helper.segment().end()) m_lastInsertedEvent = *i;
}


RestInsertionCommand::RestInsertionCommand(Segment &segment, timeT time,
                                           timeT endTime, Note note) :
    NoteInsertionCommand(segment, time, endTime, note, 0, NoAccidental)
{
    KCommand::setName("Insert Rest");
}

RestInsertionCommand::~RestInsertionCommand()
{
    // nothing
}

void
RestInsertionCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());

    Segment::iterator i =
        helper.insertRest(getBeginTime(), m_note);
    if (i != helper.segment().end()) m_lastInsertedEvent = *i;
}


ClefInsertionCommand::ClefInsertionCommand(Segment &segment, timeT time,
					   Clef clef) :
    BasicCommand("Insert Clef", segment, time, time + 1),
    m_clef(clef),
    m_lastInsertedEvent(0)
{
    // nothing
}

ClefInsertionCommand::~ClefInsertionCommand()
{
    // nothing
}

timeT
ClefInsertionCommand::getRelayoutEndTime()
{
    // Inserting a clef can change the y-coord of every subsequent note
    return getSegment().getEndTime();
}

void
ClefInsertionCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());

    Segment::iterator i = helper.insertClef(getBeginTime(), m_clef);
    if (i != helper.segment().end()) m_lastInsertedEvent = *i;
}


KeyInsertionCommand::KeyInsertionCommand(Segment &segment, timeT time,
					 Rosegarden::Key key,
					 bool convert,
					 bool transpose) :
    BasicCommand(name(&key), segment, time,
		 ((convert || transpose) ? segment.getEndTime() : time + 1)),
    m_key(key),
    m_lastInsertedEvent(0),
    m_convert(convert),
    m_transpose(transpose)
{
    // nothing
}

KeyInsertionCommand::~KeyInsertionCommand()
{
    // nothing
}

void
KeyInsertionCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    Rosegarden::Clef clef;
    Rosegarden::Key oldKey;

    if (m_convert || m_transpose) {
	helper.getClefAndKeyAt(getBeginTime(), clef, oldKey);
    }

    Segment::iterator i = helper.insertKey(getBeginTime(), m_key);

    if (i != helper.segment().end()) {

	m_lastInsertedEvent = *i;
	if (!m_convert && !m_transpose) return;

	while (++i != helper.segment().end()) {

	    //!!! what if we get two keys at the same time...?
	    if ((*i)->isa(Rosegarden::Key::EventType)) break;

	    if ((*i)->isa(Rosegarden::Clef::EventType)) {
		clef = Rosegarden::Clef(**i);
		continue;
	    }

	    if ((*i)->isa(Rosegarden::Note::EventType) &&
		(*i)->has(PITCH)) {

		long pitch = (*i)->get<Int>(PITCH);
		
		if (m_convert) {
		    (*i)->set<Int>(PITCH, m_key.convertFrom(pitch, oldKey));
		} else {
		    (*i)->set<Int>(PITCH, m_key.transposeFrom(pitch, oldKey));
		}

		(*i)->unset(ACCIDENTAL);
	    }
	}
    }
}

MultiKeyInsertionCommand::MultiKeyInsertionCommand(Rosegarden::Composition &c,
						   timeT time,
						   Rosegarden::Key key,
						   bool convert,
						   bool transpose) :
    CompoundCommand(name(&key))
{
    for (Rosegarden::Composition::iterator i = c.begin(); i != c.end(); ++i) {
       addCommand(new KeyInsertionCommand(**i, time, key, convert, transpose));
    }
}

MultiKeyInsertionCommand::~MultiKeyInsertionCommand()
{
    // nothing
}

EraseEventCommand::EraseEventCommand(Segment &segment,
				     Event *event,
				     bool collapseRest) :
    BasicCommand(makeName(event->getType()).c_str(), segment,
		 event->getAbsoluteTime(),
		 event->getAbsoluteTime() + event->getDuration(),
		 true),
    m_collapseRest(collapseRest),
    m_event(event),
    m_relayoutEndTime(getEndTime())
{
    // nothing
}

EraseEventCommand::~EraseEventCommand()
{
    // nothing
}

string
EraseEventCommand::makeName(string e)
{
    string n = "Erase ";
    n += (char)toupper(e[0]);
    n += e.substr(1);
    return n;
}

timeT
EraseEventCommand::getRelayoutEndTime()
{
    return m_relayoutEndTime;
}

void
EraseEventCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());

    if (m_event->isa(Rosegarden::Clef::EventType) ||
	m_event->isa(Rosegarden::Key ::EventType)) {
	m_relayoutEndTime = helper.segment().getEndTime();
    }

    helper.deleteEvent(m_event, m_collapseRest);
}



CutNotationCommand::CutNotationCommand(EventSelection &selection,
					 Rosegarden::Clipboard *clipboard) :
    CompoundCommand(name())
{
    addCommand(new CopyNotationCommand(selection, clipboard));
    addCommand(new EraseNotationCommand(selection));
}

CopyNotationCommand::CopyNotationCommand(EventSelection &selection,
					   Rosegarden::Clipboard *clipboard) :
    KCommand(name()),
    m_targetClipboard(clipboard)
{
    m_sourceClipboard = new Rosegarden::Clipboard;
    Segment *s = m_sourceClipboard->newSegment(&selection);
}

CopyNotationCommand::~CopyNotationCommand()
{
    delete m_sourceClipboard;
}

void
CopyNotationCommand::execute()
{
    Rosegarden::Clipboard temp(*m_targetClipboard);
    m_targetClipboard->copyFrom(m_sourceClipboard);
    m_sourceClipboard->copyFrom(&temp);
}

void
CopyNotationCommand::unexecute()
{
    Rosegarden::Clipboard temp(*m_sourceClipboard);
    m_sourceClipboard->copyFrom(m_targetClipboard);
    m_targetClipboard->copyFrom(&temp);
}

PasteNotationCommand::PasteType
PasteNotationCommand::m_defaultPaste = PasteNotationCommand::PasteIntoGap;

PasteNotationCommand::PasteNotationCommand(Rosegarden::Segment &segment,
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
PasteNotationCommand::getEffectiveEndTime(Rosegarden::Segment &segment,
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
PasteNotationCommand::getRelayoutEndTime()
{
    return m_relayoutEndTime;
}

bool
PasteNotationCommand::isPossible() 
{
    if (m_clipboard->isEmpty() || !m_clipboard->isSingleSegment()) {
	return false;
    }
    
    if (m_pasteType != PasteIntoGap) {
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
PasteNotationCommand::modifySegment()
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

    case PasteIntoGap:
	if (!helper.removeRests(pasteTime, duration)) return;
	break;

    case PasteDestructive:
	destination->erase(destination->findTime(pasteTime),
			   destination->findTime(pasteTime + duration));
	break;

    case PasteOverlay:
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
		Event *e = new Event(**i);
		e->setAbsoluteTime(e->getAbsoluteTime() - origin + pasteTime);
		destination->insert(e);
	    }
	}
	break;

    case PasteOverlayRaw:

	for (Segment::iterator i = source->begin(); i != source->end(); ++i) {

	    if ((*i)->isa(Note::EventRestType)) continue;

	    Event *e = new Event(**i);
	    e->setAbsoluteTime(e->getAbsoluteTime() - origin + pasteTime);
	    destination->insert(e);
	}
	break;

    case OpenAndPaste:

	std::vector<Event *> copies;
	for (Segment::iterator i = destination->findTime(pasteTime);
	     i != destination->end(); ++i) {
	    Event *e = new Event(**i);
	    e->setAbsoluteTime(e->getAbsoluteTime() + duration);
	    copies.push_back(e);
	}

	destination->erase(destination->findTime(pasteTime),
			   destination->end());

	for (unsigned int i = 0; i < copies.size(); ++i) {
	    destination->insert(copies[i]);
	}
	break;
    }

    for (Segment::iterator i = source->begin(); i != source->end(); ++i) {
	Event *e = new Event(**i);
	e->setAbsoluteTime(e->getAbsoluteTime() - origin + pasteTime);
	destination->insert(e);

	if (e->isa(Note::EventType)) {
	    helper.normalizeRests(e->getAbsoluteTime(),
				  e->getAbsoluteTime() + e->getDuration());
	}
    }
}


EraseNotationCommand::EraseNotationCommand(EventSelection &selection) :
    BasicSelectionCommand(name(), selection, true),
    m_selection(&selection),
    m_relayoutEndTime(getEndTime())
{
    // nothing else
}

void
EraseNotationCommand::modifySegment()
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
EraseNotationCommand::getRelayoutEndTime()
{
    return m_relayoutEndTime;
}


void GroupMenuBeamCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());

    helper.makeBeamedGroup(getBeginTime(), getEndTime(),
                           GROUP_TYPE_BEAMED);
}

void GroupMenuAutoBeamCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());

    helper.autoBeam(getBeginTime(), getEndTime(),
                    GROUP_TYPE_BEAMED);
}

void GroupMenuBreakCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());

    helper.unbeam(getBeginTime(), getEndTime());
}


GroupMenuAddIndicationCommand::GroupMenuAddIndicationCommand(std::string indicationType, 
							     EventSelection &selection) :
    BasicCommand(name(indicationType),
		 selection.getSegment(), selection.getBeginTime(),
		 selection.getBeginTime() + 1),
    m_indicationType(indicationType),
    m_indicationDuration(selection.getEndTime() - selection.getBeginTime()),
    m_lastInsertedEvent(0)
{
    // nothing else
}

GroupMenuAddIndicationCommand::~GroupMenuAddIndicationCommand()
{
    // empty
}

void
GroupMenuAddIndicationCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());

    Indication indication(m_indicationType, m_indicationDuration);
    Event *e = indication.getAsEvent(getBeginTime());
    helper.segment().insert(e);
    m_lastInsertedEvent = e;
}

QString
GroupMenuAddIndicationCommand::name(std::string indicationType)
{
    std::string n = "Add &";
    n += (char)toupper(indicationType[0]);
    n += indicationType.substr(1);
    return QString(n.c_str());
}


void TransformsMenuNormalizeRestsCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    helper.normalizeRests(getBeginTime(), getEndTime());
}

void TransformsMenuCollapseRestsCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    helper.collapseRestsAggressively(getBeginTime(), getEndTime());
}

void
TransformsMenuCollapseNotesCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());

    EventSelection::eventcontainer::iterator i;
    timeT endTime = getEndTime();
    
    // We go in reverse order, because collapseNoteAggressively
    // may delete the event following the one it's passed, but
    // never deletes anything before it

    i = m_selection->getSegmentEvents().end();

    while (i-- != m_selection->getSegmentEvents().begin()) {

	helper.collapseNoteAggressively((*i), endTime);
	helper.makeNoteViable(helper.segment().findSingle(*i));
    }
}


void
TransformsMenuChangeStemsCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if ((*i)->isa(Note::EventType)) {
	    (*i)->set<Rosegarden::Bool>(NotationProperties::STEM_UP, m_up);
	}
    }
}


void
TransformsMenuRestoreStemsCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if ((*i)->isa(Note::EventType)) {
	    (*i)->unset(NotationProperties::STEM_UP);
	}
    }
}

//!!! bleah -- merge these three classes 

void
TransformsMenuTransposeCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if ((*i)->isa(Note::EventType)) {
	    long pitch = (*i)->get<Int>(PITCH);
	    pitch += m_semitones;
	    if (pitch < 0) pitch = 0;
	    if (pitch > 127) pitch = 127;
	    (*i)->set<Int>(PITCH, pitch); 
	    (*i)->unset(ACCIDENTAL);
	}
    }
}

void
TransformsMenuTransposeOneStepCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    EventSelection::eventcontainer::iterator i;

    int offset = m_up ? 1 : -1;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if ((*i)->isa(Note::EventType)) {
	    long pitch = (*i)->get<Int>(PITCH);
	    pitch += offset;
	    if (pitch < 0) pitch = 0;
	    if (pitch > 127) pitch = 127;
	    (*i)->set<Int>(PITCH, pitch); 
	    (*i)->unset(ACCIDENTAL);
	}
    }
}

void
TransformsMenuTransposeOctaveCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    EventSelection::eventcontainer::iterator i;

    int offset = m_up ? 12 : -12;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if ((*i)->isa(Note::EventType)) {
	    long pitch = (*i)->get<Int>(PITCH);
	    pitch += offset;
	    if (pitch < 0) pitch = 0;
	    if (pitch > 127) pitch = 127;
	    (*i)->set<Int>(PITCH, pitch); 
	    (*i)->unset(ACCIDENTAL);
	}
    }
}


QString
TransformsMenuAddMarkCommand::name(Rosegarden::Mark markType)
{
    std::string m = markType;

    // Gosh, lots of collisions
    if (markType == Rosegarden::Marks::Sforzando) m = "S&forzando";
    else if (markType == Rosegarden::Marks::Rinforzando) m = "R&inforzando";
    else if (markType == Rosegarden::Marks::Tenuto) m = "T&enuto";
    else if (markType == Rosegarden::Marks::Trill) m = "Tri&ll";
    else m = std::string("&") + (char)toupper(m[0]) + m.substr(1);

    m = std::string("Add ") + m;
    return QString(m.c_str());
}

void
TransformsMenuAddMarkCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {
	
	long n = 0;
	(*i)->get<Int>(MARK_COUNT, n);
	(*i)->set<Int>(MARK_COUNT, n + 1);
	(*i)->set<String>(getMarkPropertyName(n),
			  m_mark);
    }
}

void
TransformsMenuAddTextMarkCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {
	
	long n = 0;
	(*i)->get<Int>(MARK_COUNT, n);
	(*i)->set<Int>(MARK_COUNT, n + 1);
	(*i)->set<String>(getMarkPropertyName(n),
			  Rosegarden::Marks::getTextMark(m_text));
    }
}

void
TransformsMenuRemoveMarksCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {
	
	long n = 0;
	(*i)->get<Int>(MARK_COUNT, n);
	(*i)->unset(MARK_COUNT);
	
	for (int j = 0; j < n; ++j) {
	    (*i)->unset(getMarkPropertyName(j));
	}
    }
}

