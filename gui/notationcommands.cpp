// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.2
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

#include "Composition.h"
#include "Segment.h"
#include "Event.h"
#include "NotationTypes.h"
#include "Selection.h"
#include "SegmentNotationHelper.h"
#include "SegmentMatrixHelper.h"
#include "BaseProperties.h"
#include "Clipboard.h"
#include "notationproperties.h"

#include "rosestrings.h"
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
using Rosegarden::Bool;
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


// The endTime passed in is the end of the affected section, not
// necessarily the same as time + note.getDuration()

NoteInsertionCommand::NoteInsertionCommand(Segment &segment, timeT time,
                                           timeT endTime, Note note, int pitch,
                                           Accidental accidental,
					   bool autoBeam,
					   bool matrixType,
					   NoteStyleName noteStyle) :
    BasicCommand("Insert Note", segment,
		 (autoBeam ? segment.getBarStartForTime(time) : time),
		 (autoBeam ? segment.getBarEndForTime(endTime) : endTime)),
    m_insertionTime(time),
    m_note(note),
    m_pitch(pitch),
    m_accidental(accidental),
    m_autoBeam(autoBeam),
    m_matrixType(matrixType),
    m_noteStyle(noteStyle),
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
    Segment &segment(getSegment());
    SegmentNotationHelper helper(segment);

    // If we're attempting to insert at the same time and pitch as an
    // existing note, then we remove the existing note first (so as to
    // change its duration, if the durations differ)
    Segment::iterator i, j;
    segment.getTimeSlice(m_insertionTime, i, j);
    while (i != j) {
	if ((*i)->isa(Note::EventType)) {
	    long pitch;
	    if ((*i)->get<Int>(PITCH, pitch) && pitch == m_pitch) {
		helper.deleteNote(*i);
		break;
	    }
	}
	++i;
    }

    // insert via a model event, so as to apply the note style
    
    Event *e = new Event
	(Note::EventType, m_insertionTime, m_note.getDuration());

    e->set<Int>(PITCH, m_pitch);

    if (m_accidental != Rosegarden::Accidentals::NoAccidental) {
	e->set<String>(ACCIDENTAL, m_accidental);
    }

    if (m_noteStyle != NoteStyleFactory::DefaultStyle) {
	e->set<String>(NotationProperties::NOTE_STYLE, m_noteStyle);
    }

    if (m_matrixType) {
	i = Rosegarden::SegmentMatrixHelper(segment).insertNote(e);
    } else {
	i = helper.insertNote(e);
	// e is just a model for SegmentNotationHelper::insertNote
	delete e;
    }

    if (i != segment.end()) m_lastInsertedEvent = *i;

    if (m_autoBeam) {

	// We auto-beam the bar if it contains no beamed groups
	// after the insertion point and if it contains no tupled
	// groups at all.

	timeT barStartTime = segment.getBarStartForTime(m_insertionTime);
	timeT barEndTime   = segment.getBarEndForTime(m_insertionTime);

	for (Segment::iterator j = i;
	     j != segment.end() && (*j)->getAbsoluteTime() < barEndTime;
	     ++j) {
	    if ((*j)->has(BEAMED_GROUP_ID)) return;
	}

	for (Segment::iterator j = i;
	     j != segment.end() && (*j)->getAbsoluteTime() >= barStartTime;
	     --j) {
	    if ((*j)->has(BEAMED_GROUP_TUPLET_BASE)) return;
	    if (j == segment.begin()) break;
	}

	helper.autoBeam(m_insertionTime, m_insertionTime, GROUP_TYPE_BEAMED);
    }
}


RestInsertionCommand::RestInsertionCommand(Segment &segment, timeT time,
                                           timeT endTime, Note note) :
    NoteInsertionCommand(segment, time, endTime, note, 0, NoAccidental,
			 false, false, NoteStyleFactory::DefaultStyle)
{
    setName("Insert Rest");
}

RestInsertionCommand::~RestInsertionCommand()
{
    // nothing
}

void
RestInsertionCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());

    Segment::iterator i = helper.insertRest(m_insertionTime, m_note);
    if (i != helper.segment().end()) m_lastInsertedEvent = *i;
}



ClefInsertionCommand::ClefInsertionCommand(Segment &segment, timeT time,
					   Clef clef,
					   bool shouldChangeOctave,
					   bool shouldTranspose) :
    BasicCommand(getGlobalName(&clef), segment, time,
		 ((shouldChangeOctave || shouldTranspose) ?
		  segment.getEndTime() : time + 1)),
    m_clef(clef),
    m_shouldChangeOctave(shouldChangeOctave),
    m_shouldTranspose(shouldTranspose),
    m_lastInsertedEvent(0)
{
    // nothing
}

ClefInsertionCommand::~ClefInsertionCommand()
{
    // nothing
}

QString
ClefInsertionCommand::getGlobalName(Rosegarden::Clef *clef) 
{
    if (clef) {
	QString name(strtoqstr(clef->getClefType()));
	name = name.left(1).upper() + name.right(name.length()-1);
	return QString("Change to ") + name + " Cle&f...";
    } else {
	return "Add Cle&f Change...";
    }
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

    Clef oldClef;
    Rosegarden::Key key;
    helper.getClefAndKeyAt(getStartTime(), oldClef, key);

    Segment::iterator i = helper.insertClef(getStartTime(), m_clef);
    if (i != helper.segment().end()) m_lastInsertedEvent = *i;

    if (m_clef != oldClef) {

	int semitones = 0;

	if (m_shouldChangeOctave) {
	    semitones += 12 * (m_clef.getOctave() - oldClef.getOctave());
	}
	if (m_shouldTranspose) {
	    semitones -= m_clef.getPitchOffset() - oldClef.getPitchOffset();
	}

	if (semitones != 0) {
	    while (i != helper.segment().end()) {
		if ((*i)->isa(Note::EventType)) {
		    long pitch = 0;
		    if ((*i)->get<Int>(PITCH, pitch)) {
			pitch += semitones;
			(*i)->set<Int>(PITCH, pitch);
		    }
		}
		++i;
	    }
	}
    }
}


TextInsertionCommand::TextInsertionCommand(Segment &segment, timeT time,
					   Rosegarden::Text text) :
    BasicCommand("Insert Text", segment, time, time + 1),
    m_text(text),
    m_lastInsertedEvent(0)
{
    // nothing
}

TextInsertionCommand::~TextInsertionCommand()
{
    // nothing
}

void
TextInsertionCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());

    Segment::iterator i = helper.insertText(getStartTime(), m_text);
    if (i != helper.segment().end()) m_lastInsertedEvent = *i;
}


KeyInsertionCommand::KeyInsertionCommand(Segment &segment, timeT time,
					 Rosegarden::Key key,
					 bool convert,
					 bool transpose) :
    BasicCommand(getGlobalName(&key), segment, time, segment.getEndTime()),
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
	helper.getClefAndKeyAt(getStartTime(), clef, oldKey);
    }

    Segment::iterator i = helper.insertKey(getStartTime(), m_key);

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
    KMacroCommand(getGlobalName(&key))
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
    BasicCommand(strtoqstr(makeName(event->getType())),
		 segment,
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



void
GroupMenuBeamCommand::modifySegment()
{
    int id = getSegment().getNextId();

    for (EventSelection::eventcontainer::iterator i =
	     m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if ((*i)->isa(Note::EventType)) {
	    (*i)->set<Int>(BEAMED_GROUP_ID, id);
	    (*i)->set<String>(BEAMED_GROUP_TYPE, GROUP_TYPE_BEAMED);
	}
    }
}

void
GroupMenuAutoBeamCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());

    helper.autoBeam(getStartTime(), getEndTime(),
                    GROUP_TYPE_BEAMED, m_quantizer);
}

void
GroupMenuBreakCommand::modifySegment()
{
    for (EventSelection::eventcontainer::iterator i =
	     m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	(*i)->unset(BEAMED_GROUP_ID);
	(*i)->unset(BEAMED_GROUP_TYPE);
    }
}

GroupMenuGraceCommand::GroupMenuGraceCommand(Rosegarden::EventSelection &selection) :
    BasicCommand(getGlobalName(),
		 selection.getSegment(),
		 selection.getStartTime(),
		 getEffectiveEndTime(selection),
		 true),
    m_selection(&selection)
{ 
}

timeT
GroupMenuGraceCommand::getEffectiveEndTime(Rosegarden::EventSelection &
					   selection)
{
    EventSelection::eventcontainer::iterator i =
	selection.getSegmentEvents().end();
    if (i == selection.getSegmentEvents().begin())
	return selection.getEndTime();
    --i;

    Segment::iterator si = selection.getSegment().findTime
	((*i)->getAbsoluteTime() + (*i)->getDuration());
    if (si == selection.getSegment().end()) return selection.getEndTime();
    else return (*si)->getAbsoluteTime() + 1;
}

void
GroupMenuGraceCommand::modifySegment()
{
    Segment &s(getSegment());
    timeT startTime = getStartTime();
    timeT endOfLastGraceNote = startTime;
    int id = s.getNextId();

    // first turn the selected events into grace notes

    for (EventSelection::eventcontainer::iterator i =
	     m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if ((*i)->isa(Note::EventType)) {
	    (*i)->set<Bool>(IS_GRACE_NOTE, true);
	    (*i)->set<Int>(BEAMED_GROUP_ID, id);
	    (*i)->set<String>(BEAMED_GROUP_TYPE, GROUP_TYPE_GRACE);
	}

	if ((*i)->getAbsoluteTime() + (*i)->getDuration() >
	    endOfLastGraceNote) {
	    endOfLastGraceNote = 
		(*i)->getAbsoluteTime() + (*i)->getDuration();
	}
    }

    // then indicate that the following chord has grace notes
    
    Segment::iterator i0, i1;
    s.getTimeSlice(endOfLastGraceNote, i0, i1);

    while (i0 != i1 && i0 != s.end()) {
	if (!(*i0)->isa(Note::EventType)) continue;
	(*i0)->set<Bool>(HAS_GRACE_NOTES, true);
	++i0;
    }
}

void
GroupMenuUnGraceCommand::modifySegment()
{
    //!!!
}


GroupMenuTupletCommand::GroupMenuTupletCommand(Rosegarden::Segment &segment,
					       timeT startTime,
					       timeT unit,
					       int untupled, int tupled) :
    BasicCommand(getGlobalName((untupled == 3) && (tupled == 2)),
		 segment, startTime, startTime + (unit * untupled)),
    m_unit(unit),
    m_untupled(untupled),
    m_tupled(tupled)
{
    // nothing else
}

void
GroupMenuTupletCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    helper.makeTupletGroup(getStartTime(), m_untupled, m_tupled, m_unit);
}



GroupMenuAddIndicationCommand::GroupMenuAddIndicationCommand(std::string indicationType, 
							     EventSelection &selection) :
    BasicCommand(getGlobalName(indicationType),
		 selection.getSegment(), selection.getStartTime(),
		 selection.getStartTime() + 1),
    m_indicationType(indicationType),
    m_indicationDuration(selection.getEndTime() - selection.getStartTime()),
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
    Event *e = indication.getAsEvent(getStartTime());
    helper.segment().insert(e);
    m_lastInsertedEvent = e;
}

QString
GroupMenuAddIndicationCommand::getGlobalName(std::string indicationType)
{
    if (indicationType == Rosegarden::Indication::Slur) {
	return "Add S&lur";
    }

    std::string n = "Add &";
    n += (char)toupper(indicationType[0]);
    n += indicationType.substr(1);
    return QString(strtoqstr(n));
}

TransformsMenuNormalizeRestsCommand::TransformsMenuNormalizeRestsCommand
(Rosegarden::EventSelection &selection) :
    BasicCommand(getGlobalName(),
		 selection.getSegment(),
		 selection.getStartTime(),
		 selection.getEndTime())
{
    // nothing else
}

void TransformsMenuNormalizeRestsCommand::modifySegment()
{
    getSegment().normalizeRests(getStartTime(), getEndTime());
}

TransformsMenuCollapseRestsCommand::TransformsMenuCollapseRestsCommand
(Rosegarden::EventSelection &selection) :
    BasicCommand(getGlobalName(),
		 selection.getSegment(),
		 selection.getStartTime(),
		 selection.getEndTime())
{
    // nothing else
}

void TransformsMenuCollapseRestsCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    helper.collapseRestsAggressively(getStartTime(), getEndTime());
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
    }
}


void
TransformsMenuTieNotesCommand::modifySegment()
{
    Segment &segment(getSegment());
    SegmentNotationHelper helper(segment);

    //!!! move part of this to SegmentNotationHelper?

    for (EventSelection::eventcontainer::iterator i =
	     m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	bool tiedForward;
	if ((*i)->get<Bool>(TIED_FORWARD, tiedForward) && tiedForward) {
	    continue;
	}
	
	Segment::iterator si = segment.findSingle(*i);
	Segment::iterator sj;
	while ((sj = helper.getNextAdjacentNote(si, true, false)) !=
	       segment.end()) {
	    if (!m_selection->contains(*sj)) break;
	    (*si)->set<Bool>(TIED_FORWARD, true);
	    (*sj)->set<Bool>(TIED_BACKWARD, true);
	    si = sj;
	}
    }
}



void
TransformsMenuUntieNotesCommand::modifySegment()
{
    for (EventSelection::eventcontainer::iterator i =
	     m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	(*i)->unset(TIED_FORWARD);
	(*i)->unset(TIED_BACKWARD);
    }
}


void
TransformsMenuChangeStemsCommand::modifySegment()
{
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
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if ((*i)->isa(Note::EventType)) {
	    (*i)->unset(NotationProperties::STEM_UP);
	}
    }
}

QString
TransformsMenuChangeStyleCommand::getGlobalName(NoteStyleName style)
{
    return strtoqstr(style);
}


void
TransformsMenuChangeStyleCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if ((*i)->isa(Note::EventType)) {
	    if (m_style == NoteStyleFactory::DefaultStyle) {
		(*i)->unset(NotationProperties::NOTE_STYLE);
	    } else {
		(*i)->set<Rosegarden::String>
			(NotationProperties::NOTE_STYLE, m_style);
	    }
	}
    }
}


void
TransformsMenuTransposeCommand::modifySegment()
{
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
NotesMenuAddSlashesCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if (m_number < 1) {
	    (*i)->unset(NotationProperties::SLASHES);
	} else {
	    (*i)->set<Int>(NotationProperties::SLASHES, m_number);
	}
    }
}


QString
MarksMenuAddMarkCommand::getGlobalName(Rosegarden::Mark markType)
{
    std::string m = markType;

    // Gosh, lots of collisions
    if (markType == Rosegarden::Marks::Sforzando) m = "S&forzando";
    else if (markType == Rosegarden::Marks::Staccato) m = "Sta&ccato";
    else if (markType == Rosegarden::Marks::Rinforzando) m = "R&inforzando";
    else if (markType == Rosegarden::Marks::Tenuto) m = "T&enuto";
    else if (markType == Rosegarden::Marks::Trill) m = "Tri&ll";
    else m = std::string("&") + (char)toupper(m[0]) + m.substr(1);

    m = std::string("Add ") + m;
    return QString(strtoqstr(m));
}

void
MarksMenuAddMarkCommand::modifySegment()
{
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
MarksMenuAddTextMarkCommand::modifySegment()
{
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
MarksMenuRemoveMarksCommand::modifySegment()
{
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


void
TransformsMenuFixSmoothingCommand::modifySegment()
{
    //!!! FIXME -- the Quantizer needs a fixQuantizedValues(EventSelection*)
    // method, but it hasn't got one yet so for the moment we're just fixing
    // all the quantized values within the time limits whether selected or not

    Segment *segment(&m_selection->getSegment());
    m_quantizer->fixQuantizedValues
	(segment,
	 segment->findTime(m_selection->getStartTime()),
	 segment->findTime(m_selection->getEndTime()));
}


