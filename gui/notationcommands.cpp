// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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
#include "Sets.h"
#include "Quantizer.h"
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
using Rosegarden::Text;
using Rosegarden::Accidental;
using namespace Rosegarden::Accidentals;
using Rosegarden::Mark;
using Rosegarden::Indication;
using Rosegarden::EventSelection;

using namespace Rosegarden::BaseProperties;

using std::string;
using std::endl;


// The endTime passed in is the end of the affected section, not
// necessarily the same as time + note.getDuration()

NoteInsertionCommand::NoteInsertionCommand(Segment &segment, timeT time,
                                           timeT endTime, Note note, int pitch,
                                           Accidental accidental,
					   bool autoBeam,
					   bool matrixType,
					   NoteStyleName noteStyle) :
    BasicCommand(i18n("Insert Note"), segment,
		 getModificationStartTime(segment, time),
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

Rosegarden::timeT
NoteInsertionCommand::getModificationStartTime(Segment &segment,
					       Rosegarden::timeT time)
{
    // We may be splitting a rest to insert this note, so we'll have
    // to record the change from the start of that rest rather than
    // just the start of the note

    Rosegarden::timeT barTime = segment.getBarStartForTime(time);
    Segment::iterator i = segment.findNearestTime(time);

    if (i != segment.end() &&
	(*i)->getAbsoluteTime() < time &&
	(*i)->getAbsoluteTime() + (*i)->getDuration() > time &&
	(*i)->isa(Note::EventRestType)) {
	return std::min(barTime, (*i)->getAbsoluteTime());
    }

    return barTime;
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
    e->set<Int>(VELOCITY, 100);

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
ClefInsertionCommand::getGlobalName(Rosegarden::Clef *) 
{
/* doesn't handle octave offset -- leave it for now
    if (clef) {
	QString name(strtoqstr(clef->getClefType()));
	name = name.left(1).upper() + name.right(name.length()-1);
	return i18n("Change to %1 Cle&f...").arg(name);
    } else {
*/
	return i18n("Add Cle&f Change...");
/*
    }
*/
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
    Clef oldClef(getSegment().getClefAtTime(getStartTime()));

    Segment::iterator i = getSegment().findTime(getStartTime());
    while (getSegment().isBeforeEndMarker(i)) {
	if ((*i)->getAbsoluteTime() > getStartTime()) {
	    break;
	}
	if ((*i)->isa(Clef::EventType)) {
	    getSegment().erase(i);
	    break;
	}
	++i;
    }

    i = helper.insertClef(getStartTime(), m_clef);
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
					   Text text) :
    BasicCommand(i18n("Insert Text"), segment, time, time + 1),
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


TextChangeCommand::TextChangeCommand(Segment &segment,
				     Event *event,
				     Text text) :
    BasicCommand(i18n("Edit Text"), segment,
		 event->getAbsoluteTime(), event->getAbsoluteTime() + 1,
		 true), // bruteForceRedo
    m_event(event),
    m_text(text)
{
    // nothing
}

TextChangeCommand::~TextChangeCommand()
{
}

void
TextChangeCommand::modifySegment()
{
    m_event->set<String>(Text::TextTypePropertyName, m_text.getTextType());
    m_event->set<String>(Text::TextPropertyName, m_text.getText());
}


KeyInsertionCommand::KeyInsertionCommand(Segment &segment, timeT time,
					 Rosegarden::Key key,
					 bool convert,
					 bool transpose,
					 bool transposeKey) :
    BasicCommand(getGlobalName(&key), segment, time, segment.getEndTime()),
    m_key(key),
    m_lastInsertedEvent(0),
    m_convert(convert),
    m_transpose(transpose),
    m_transposeKey(transposeKey)

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
    Rosegarden::Key oldKey;

    if (m_convert || m_transpose) {
	oldKey = getSegment().getKeyAtTime(getStartTime());
    }

    Segment::iterator i = getSegment().findTime(getStartTime());
    while (getSegment().isBeforeEndMarker(i)) {
	if ((*i)->getAbsoluteTime() > getStartTime()) {
	    break;
	}
	if ((*i)->isa(Rosegarden::Key::EventType)) {
	    getSegment().erase(i);
	    break;
	}
	++i;
    }

    // transpose if desired, according to new dialog option
    if (m_transposeKey) {
	// calculate the offset key for tranposing segments, fixes #1520716
	//
	int segTranspose = getSegment().getTranspose();	

	bool sharp = false;

	// we don't really care about major/minor for this, so pass it through
	bool minor = m_key.isMinor();

	// strip off any extra octave(s)
	segTranspose %= 12;
	
	int newAcc = 0;

	// use a ghastly switch statement from hell to create a case by case
	// lookup table, because I can't figure out any other way to get
	// there, and something ugly that works is better than the most
	// compact and beautiful code in the world, if that prettier code has
	// even one bug
	switch (segTranspose) {
	    case 0:
		break;
	    case 1:
		sharp = true;
		newAcc = 5;
		break;
	    case 2:
		sharp = false;
		newAcc = 2;
		break;
	    case 3:
		sharp = true;
		newAcc = 3;
		break;
	    case 4:
		sharp = false;
		newAcc = 4;
		break;
	    case 5:
		sharp = true;
		newAcc = 1;
		break;
	    case 6:
		sharp = false;
		newAcc =6;
		break;
	    case 7:
		sharp = false;
		newAcc = 1;
		break;
	    case 8:
		sharp = true;
		newAcc = 4;
		break;
	    case 9:
		sharp = false;
		newAcc = 3;
		break;
	    case 10:
		sharp = true;
		newAcc = 2;
		break;
	    case 11:
		sharp = false;
		newAcc = 5;
		break;
	    case -1:
		sharp = false;
		newAcc = 5;
		break;
	    case -2:
		sharp = true;
		newAcc = 2;
		break;
	    case -3:
		sharp = false;
		newAcc = 3;
		break;
	    case -4:
		sharp = true;
		newAcc = 4;
		break;
	    case -5:
		sharp = false;
		newAcc = 1;
		break;
	    case -6:
		sharp = false;
		newAcc = 6;
		break;
	    case -7:
		sharp = true;
		newAcc = 1;
		break;
	    case -8:
		sharp = false;
		newAcc = 4;
		break;
	    case -9:
		sharp = true;
		newAcc = 3;
		break;
	    case -10:
		sharp = false;
		newAcc = 2;
		break;
	    case -11:
		sharp = true;
		newAcc = 5;
		break;
	    default:
		RG_DEBUG << "I defaulted in this switch statement, this is probably a BUG." << endl;
	}

	// failsafe should never be needed, but creating a key with bad
	// parameters causes an immediate crash, so we insure against that
	// anyway
	if (newAcc > 7 || newAcc < 0) {
	    RG_DEBUG << "KeyInsertionCommand: tried to create an illegal key!  Using C major failsafe!" << endl;
	    minor = false;
	    sharp = true;
	    newAcc = 0;
	}

	// create a new key with the newly calculated variables
	Rosegarden::Key k(newAcc, sharp, minor);
	RG_DEBUG << "KeyInsertCommand: created key with " << newAcc 
		 << (sharp ? " sharp(s)" : " flat(s)") << endl;

	m_key = k;
    } // if (m_transposeKey)

    i = helper.insertKey(getStartTime(), m_key);

    if (i != helper.segment().end()) {

	m_lastInsertedEvent = *i;
	if (!m_convert && !m_transpose) return;

	while (++i != helper.segment().end()) {

	    //!!! what if we get two keys at the same time...?
	    if ((*i)->isa(Rosegarden::Key::EventType)) break;

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
						   bool transpose,
						   bool transposeKey) :
    KMacroCommand(getGlobalName(&key))
{
    for (Rosegarden::Composition::iterator i = c.begin(); i != c.end(); ++i) {
	Rosegarden::Segment *segment = *i;

	// no harm in using getEndTime instead of getEndMarkerTime here:
	if (segment->getStartTime() <= time && segment->getEndTime() > time) {
	    addCommand(new KeyInsertionCommand(*segment, time, key, convert, transpose, transposeKey));
	} else if (segment->getStartTime() > time) {
	    addCommand(new KeyInsertionCommand(*segment, segment->getStartTime(),
					       key, convert, transpose, transposeKey));
	}
    }
}

MultiKeyInsertionCommand::~MultiKeyInsertionCommand()
{
    // nothing
}


SustainInsertionCommand::SustainInsertionCommand(Segment &segment, timeT time,
						 bool down,
						 int controllerNumber) :
    BasicCommand(getGlobalName(down), segment, time, time),
    m_down(down),
    m_controllerNumber(controllerNumber),
    m_lastInsertedEvent(0)
{
    // nothing
}

SustainInsertionCommand::~SustainInsertionCommand()
{
    // nothing
}

void
SustainInsertionCommand::modifySegment()
{
    Event *e = new Event(Rosegarden::Controller::EventType, getStartTime(), 0,
			 Rosegarden::Controller::EventSubOrdering);
    e->set<Int>(Rosegarden::Controller::NUMBER, m_controllerNumber);
    e->set<Int>(Rosegarden::Controller::VALUE, m_down ? 127 : 0);
    m_lastInsertedEvent = *getSegment().insert(e);
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

    } else if (m_event->isa(Rosegarden::Indication::EventType)) {

	try {
	    Indication indication(*m_event);
	    if (indication.isOttavaType()) {
		
		for (Segment::iterator i = getSegment().findTime
			 (m_event->getAbsoluteTime());
		     i != getSegment().findTime
			 (m_event->getAbsoluteTime() + indication.getIndicationDuration());
		     ++i) {
		    (*i)->unset(NotationProperties::OTTAVA_SHIFT);
		}
	    }
	} catch (...) {
	}
    }

    helper.deleteEvent(m_event, m_collapseRest);
}



void
NotesMenuBeamCommand::modifySegment()
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
NotesMenuAutoBeamCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    helper.autoBeam(getStartTime(), getEndTime(), GROUP_TYPE_BEAMED);
}

void
NotesMenuBreakCommand::modifySegment()
{
    for (EventSelection::eventcontainer::iterator i =
	     m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	(*i)->unset(NotationProperties::BEAMED);
	(*i)->unset(BEAMED_GROUP_ID);
	(*i)->unset(BEAMED_GROUP_TYPE);
	(*i)->clearNonPersistentProperties();
    }
}

AdjustMenuGraceCommand::AdjustMenuGraceCommand(Rosegarden::EventSelection &selection) :
    BasicCommand(getGlobalName(),
		 selection.getSegment(),
		 selection.getStartTime(),
		 getEffectiveEndTime(selection),
		 true),
    m_selection(&selection)
{ 
}

timeT
AdjustMenuGraceCommand::getEffectiveEndTime(Rosegarden::EventSelection &
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
AdjustMenuGraceCommand::modifySegment()
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
	if (!(*i0)->isa(Note::EventType)) {
	    ++i0;
	    continue;
	}
	(*i0)->set<Bool>(HAS_GRACE_NOTES, true);
	++i0;
    }
}

void
AdjustMenuUnGraceCommand::modifySegment()
{
    //!!!
}


AdjustMenuTupletCommand::AdjustMenuTupletCommand(Rosegarden::Segment &segment,
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
AdjustMenuTupletCommand::modifySegment()
{
    if (m_hasTimingAlready) {

	int groupId = getSegment().getNextId();

	for (Segment::iterator i = getSegment().findTime(getStartTime());
	     getSegment().isBeforeEndMarker(i); ++i) {

	    if ((*i)->getNotationAbsoluteTime() >=
	        getStartTime() + (m_unit * m_tupled)) break;

	    Event *e = *i;

	    e->set<Int>(BEAMED_GROUP_ID, groupId);
	    e->set<String>(BEAMED_GROUP_TYPE, GROUP_TYPE_TUPLED);
	    
	    e->set<Int>(BEAMED_GROUP_TUPLET_BASE, m_unit);
	    e->set<Int>(BEAMED_GROUP_TUPLED_COUNT, m_tupled);
	    e->set<Int>(BEAMED_GROUP_UNTUPLED_COUNT, m_untupled);
	}

    } else {
	SegmentNotationHelper helper(getSegment());
	helper.makeTupletGroup(getStartTime(), m_untupled, m_tupled, m_unit);
    }
}


void
AdjustMenuUnTupletCommand::modifySegment()
{
    for (EventSelection::eventcontainer::iterator i =
	     m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	(*i)->unset(BEAMED_GROUP_ID);
	(*i)->unset(BEAMED_GROUP_TYPE);
	(*i)->unset(BEAMED_GROUP_TUPLET_BASE);
	(*i)->unset(BEAMED_GROUP_TUPLED_COUNT);
	(*i)->unset(BEAMED_GROUP_UNTUPLED_COUNT);
    }
}



NotesMenuAddIndicationCommand::NotesMenuAddIndicationCommand(std::string indicationType, 
							     EventSelection &selection) :
    BasicCommand(getGlobalName(indicationType),
		 selection.getSegment(),
		 selection.getStartTime(),
		 selection.getEndTime()),
    m_indicationType(indicationType),
    m_indicationDuration(selection.getEndTime() - selection.getStartTime()),
    m_lastInsertedEvent(0)
{
    // nothing else
}

NotesMenuAddIndicationCommand::~NotesMenuAddIndicationCommand()
{
    // empty
}

bool
NotesMenuAddIndicationCommand::canExecute()
{
    Segment &s(getSegment());
    
    for (Segment::iterator i = s.begin(); s.isBeforeEndMarker(i); ++i) {

	if ((*i)->getAbsoluteTime() >= getStartTime() + m_indicationDuration) {
	    return true;
	}

	if ((*i)->isa(Indication::EventType)) {

	    try {
		Indication indication(**i);

		if ((*i)->getAbsoluteTime() + indication.getIndicationDuration() <=
		    getStartTime()) continue;
		
		std::string type = indication.getIndicationType();

		if (type == m_indicationType) {
		    // for all indications (including slur), we reject an
		    // exact overlap
		    if ((*i)->getAbsoluteTime() == getStartTime() &&
			indication.getIndicationDuration() == m_indicationDuration) {
			return false;
		    }
		} else if (m_indicationType == Indication::Slur) {
		    continue;
		}

		// for non-slur indications we reject a partial
		// overlap such as this one, if it's an overlap with
		// an indication of the same "sort"

		if (m_indicationType == Indication::Crescendo ||
		    m_indicationType == Indication::Decrescendo) {
		    if (type == Indication::Crescendo ||
			type == Indication::Decrescendo) return false;
		}

		if (m_indicationType == Indication::QuindicesimaUp ||
		    m_indicationType == Indication::OttavaUp ||
		    m_indicationType == Indication::OttavaDown ||
		    m_indicationType == Indication::QuindicesimaDown) {
		    if (indication.isOttavaType()) return false;
		}
	    } catch (...) {
	    }		    
	}
    }    

    return true;
}

void
NotesMenuAddIndicationCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());

    Indication indication(m_indicationType, m_indicationDuration);
    Event *e = indication.getAsEvent(getStartTime());
    helper.segment().insert(e);
    m_lastInsertedEvent = e;

    if (indication.isOttavaType()) {
	for (Segment::iterator i = getSegment().findTime(getStartTime());
	     i != getSegment().findTime(getStartTime() + m_indicationDuration);
	     ++i) {
	    if ((*i)->isa(Note::EventType)) {
		(*i)->setMaybe<Int>(NotationProperties::OTTAVA_SHIFT,
				    indication.getOttavaShift());
	    }
	}
    }
}

QString
NotesMenuAddIndicationCommand::getGlobalName(std::string indicationType)
{
    if (indicationType == Rosegarden::Indication::Slur) {
	return i18n("Add S&lur");
    } else if (indicationType == Rosegarden::Indication::PhrasingSlur) {
	return i18n("Add &Phrasing Slur");
    } else if (indicationType == Rosegarden::Indication::QuindicesimaUp) {
	return i18n("Add Double-Octave Up");
    } else if (indicationType == Rosegarden::Indication::OttavaUp) {
	return i18n("Add Octave &Up");
    } else if (indicationType == Rosegarden::Indication::OttavaDown) {
	return i18n("Add Octave &Down");
    } else if (indicationType == Rosegarden::Indication::QuindicesimaDown) {
	return i18n("Add Double Octave Down");

    // We used to generate these ones from the internal names plus
    // caps, but that makes them untranslateable:
    } else if (indicationType == Rosegarden::Indication::Crescendo) {
	return i18n("Add &Crescendo");
    } else if (indicationType == Rosegarden::Indication::Decrescendo) {
	return i18n("Add &Decrescendo");
    } else if (indicationType == Rosegarden::Indication::Glissando) {
	return i18n("Add &Glissando");
    }

    QString n = i18n("Add &%1%2").arg((char)toupper(indicationType[0])).arg(strtoqstr(indicationType.substr(1)));
    return n;
}


void
AdjustMenuMakeChordCommand::modifySegment()
{
    // find all the notes in the selection, and bring them back to align
    // with the start of the selection, giving them the same duration as
    // the longest note among them

    std::vector<Event *> toErase;
    std::vector<Event *> toInsert;
    Segment &segment(m_selection->getSegment());
    
    for (EventSelection::eventcontainer::iterator i =
	     m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {
	
	if ((*i)->isa(Note::EventType)) {
	    toErase.push_back(*i);
	    toInsert.push_back(new Event(**i, m_selection->getStartTime()));
	}
    }

    for (unsigned int j = 0; j < toErase.size(); ++j) {
	Segment::iterator jtr(segment.findSingle(toErase[j]));
	if (jtr != segment.end()) segment.erase(jtr);
    }

    for (unsigned int j = 0; j < toInsert.size(); ++j) {
	segment.insert(toInsert[j]);
    }

    segment.normalizeRests(getStartTime(), getEndTime());

    //!!! should select all notes in chord now
}	 


AdjustMenuNormalizeRestsCommand::AdjustMenuNormalizeRestsCommand
(Rosegarden::EventSelection &selection) :
    BasicCommand(getGlobalName(),
		 selection.getSegment(),
		 selection.getStartTime(),
		 selection.getEndTime())
{
    // nothing else
}

void AdjustMenuNormalizeRestsCommand::modifySegment()
{
    getSegment().normalizeRests(getStartTime(), getEndTime());
}

AdjustMenuCollapseRestsCommand::AdjustMenuCollapseRestsCommand
(Rosegarden::EventSelection &selection) :
    BasicCommand(getGlobalName(),
		 selection.getSegment(),
		 selection.getStartTime(),
		 selection.getEndTime())
{
    // nothing else
}

void AdjustMenuCollapseRestsCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    helper.collapseRestsAggressively(getStartTime(), getEndTime());
}

void
NotesMenuTieNotesCommand::modifySegment()
{
    Segment &segment(getSegment());
    SegmentNotationHelper helper(segment);

    //!!! move part of this to SegmentNotationHelper?

    for (EventSelection::eventcontainer::iterator i =
	     m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

//	bool tiedForward;
//	if ((*i)->get<Bool>(TIED_FORWARD, tiedForward) && tiedForward) {
//	    continue;
//	}
	
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
AdjustMenuUntieNotesCommand::modifySegment()
{
    for (EventSelection::eventcontainer::iterator i =
	     m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	(*i)->unset(TIED_FORWARD);
	(*i)->unset(TIED_BACKWARD);
    }
}


void
AdjustMenuMakeNotesViableCommand::modifySegment()
{
    Segment &segment(getSegment());
    SegmentNotationHelper helper(segment);

    if (m_selection) {
	EventSelection::RangeTimeList ranges(m_selection->getRangeTimes());
	for (EventSelection::RangeTimeList::iterator i = ranges.begin();
	     i != ranges.end(); ++i) {
	    helper.makeNotesViable(i->first, i->second, true);
	    segment.normalizeRests(i->first, i->second);
	}
    } else {
	helper.makeNotesViable(getStartTime(), getEndTime(), true);
	segment.normalizeRests(getStartTime(), getEndTime());
    }
}

void
AdjustMenuMakeRegionViableCommand::modifySegment()
{
    Segment &segment(getSegment());
    SegmentNotationHelper helper(segment);

    helper.makeNotesViable(getStartTime(), getEndTime(), true);
    segment.normalizeRests(getStartTime(), getEndTime());
}

void
AdjustMenuDeCounterpointCommand::modifySegment()
{
    Segment &segment(getSegment());
    SegmentNotationHelper helper(segment);

    if (m_selection) {
	EventSelection::RangeTimeList ranges(m_selection->getRangeTimes());
	for (EventSelection::RangeTimeList::iterator i = ranges.begin();
	     i != ranges.end(); ++i) {
	    helper.deCounterpoint(i->first, i->second);
	    segment.normalizeRests(i->first, i->second);
	}
    } else {
	helper.deCounterpoint(getStartTime(), getEndTime());
	segment.normalizeRests(getStartTime(), getEndTime());
    }
}


void
AdjustMenuChangeStemsCommand::modifySegment()
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
AdjustMenuRestoreStemsCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if ((*i)->isa(Note::EventType)) {
	    (*i)->unset(NotationProperties::STEM_UP);
	}
    }
}


void
AdjustMenuChangeSlurPositionCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if ((*i)->isa(Indication::EventType)) {
	    std::string indicationType;
	    if ((*i)->get<String>(Indication::IndicationTypePropertyName, indicationType)
		&& (indicationType == Indication::Slur ||
		    indicationType == Indication::PhrasingSlur)) {
		(*i)->set<Rosegarden::Bool>(NotationProperties::SLUR_ABOVE, m_above);
	    }
	}
    }
}


void
AdjustMenuRestoreSlursCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if ((*i)->isa(Indication::EventType)) {
	    std::string indicationType;
	    if ((*i)->get<String>(Indication::IndicationTypePropertyName, indicationType)
		&& (indicationType == Indication::Slur ||
		    indicationType == Indication::PhrasingSlur)) {
		(*i)->unset(NotationProperties::SLUR_ABOVE);
	    }
	}
    }
}


QString
AdjustMenuChangeStyleCommand::getGlobalName(NoteStyleName style)
{
    return strtoqstr(style);
}


void
AdjustMenuChangeStyleCommand::modifySegment()
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
NotesMenuAddMarkCommand::getGlobalName(Rosegarden::Mark markType)
{
    QString m = strtoqstr(markType);

    // Gosh, lots of collisions
    if (markType == Rosegarden::Marks::Sforzando) m = i18n("S&forzando");
    else if (markType == Rosegarden::Marks::Staccato) m = i18n("Sta&ccato");
    else if (markType == Rosegarden::Marks::Rinforzando) m = i18n("R&inforzando");
    else if (markType == Rosegarden::Marks::Tenuto) m = i18n("T&enuto");
    else if (markType == Rosegarden::Marks::Trill) m = i18n("Tri&ll");
    else if (markType == Rosegarden::Marks::LongTrill) m = i18n("Trill &with Line");
    else if (markType == Rosegarden::Marks::TrillLine) m = i18n("Trill Line");
    else if (markType == Rosegarden::Marks::Turn) m = i18n("&Turn");
    else if (markType == Rosegarden::Marks::Accent) m = i18n("&Accent");
    else if (markType == Rosegarden::Marks::Staccatissimo) m = i18n("&Staccatissimo");
    else if (markType == Rosegarden::Marks::Marcato) m = i18n("&Marcato");
    else if (markType == Rosegarden::Marks::Pause) m = i18n("&Pause");
    else if (markType == Rosegarden::Marks::UpBow) m = i18n("&Up-Bow");
    else if (markType == Rosegarden::Marks::DownBow) m = i18n("&Down-Bow");
    else if (markType == Rosegarden::Marks::Mordent)
	m = i18n("Mo&rdent");
    else if (markType == Rosegarden::Marks::MordentInverted)
	m = i18n("Inverted Mordent");
    else if (markType == Rosegarden::Marks::MordentLong)
	m = i18n("Long Mordent");
    else if (markType == Rosegarden::Marks::MordentLongInverted)
	m = i18n("Lon&g Inverted Mordent");
    else m = i18n("&%1%2").arg(m[0].upper()).arg(m.right(m.length()-1));
    // FIXME: That last i18n has very little chance of working, unless
    // by some miracle the exact same string was translated elsewhere already
    // but we'll leave it as a warning

    m = i18n("Add %1").arg(m);
    return m;
}

void
NotesMenuAddMarkCommand::modifySegment()
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
NotesMenuAddTextMarkCommand::modifySegment()
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

QString
NotesMenuAddFingeringMarkCommand::getGlobalName(QString fingering)
{
    if (fingering == "") return i18n("Add Other &Fingering...");
    else if (fingering == "0") return i18n("Add Fingering &0 (Thumb)");
    else return i18n("Add Fingering &%1").arg(fingering);
}

void
NotesMenuAddFingeringMarkCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;
    Segment &segment(m_selection->getSegment());

    std::set<Event *> done;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {
	
	if (done.find(*i) != done.end()) continue;
	if (!(*i)->isa(Note::EventType)) continue;

	// We should do this on a chord-by-chord basis, considering
	// only those notes in a chord that are also in the selection.
	// Apply this fingering to the first note in the chord that
	// does not already have a fingering.  If they all already do,
	// then clear them all and start again.

	Rosegarden::Chord chord(segment, segment.findSingle(*i),
				segment.getComposition()->getNotationQuantizer());

	int attempt = 0;

	while (attempt < 2) {

	    int count = 0;

	    for (Rosegarden::Chord::iterator ci = chord.begin();
		 ci != chord.end(); ++ci) {

		if (!m_selection->contains(**ci)) continue;

		if (attempt < 2 &&
		    Rosegarden::Marks::getFingeringMark(***ci) ==
		    Rosegarden::Marks::NoMark) {
		    Rosegarden::Marks::addMark
			(***ci, Rosegarden::Marks::getFingeringMark(m_text), true);
		    attempt = 2;
		}
		
		done.insert(**ci);
		++count;
	    }

	    if (attempt < 2) {
		if (count == 0) break;
		for (Rosegarden::Chord::iterator ci = chord.begin();
		     ci != chord.end(); ++ci) {
		    if (m_selection->contains(**ci)) {
			Rosegarden::Marks::removeMark
			    (***ci,
			     Rosegarden::Marks::getFingeringMark(***ci));
		    }
		}
		++attempt;
	    }
	}
    }
}

void
NotesMenuRemoveMarksCommand::modifySegment()
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
NotesMenuRemoveFingeringMarksCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	std::vector<Rosegarden::Mark> marks = Rosegarden::Marks::getMarks(**i);
	for (std::vector<Rosegarden::Mark>::iterator j = marks.begin();
	     j != marks.end(); ++j) {
	    if (Rosegarden::Marks::isFingeringMark(*j)) {
		Rosegarden::Marks::removeMark(**i, *j);
	    }
	}
    }
}

void
AdjustMenuFixNotationQuantizeCommand::modifySegment()
{
    std::vector<Event *> toErase;
    std::vector<Event *> toInsert;
    Segment &segment(m_selection->getSegment());

    EventSelection::eventcontainer::iterator i;

    //!!! the Quantizer needs a fixQuantizedValues(EventSelection*)
    //method, but it hasn't got one yet so for the moment we're doing
    //this by hand.

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {
	
	timeT ut = (*i)->getAbsoluteTime();
	timeT ud = (*i)->getDuration();
	timeT qt = (*i)->getNotationAbsoluteTime();
	timeT qd = (*i)->getNotationDuration();
	
	if ((ut != qt) || (ud != qd)) {
	    toErase.push_back(*i);
	    toInsert.push_back(new Event(**i, qt, qd));
	}
    }

    for (unsigned int j = 0; j < toErase.size(); ++j) {
	Segment::iterator jtr(segment.findSingle(toErase[j]));
	if (jtr != segment.end()) segment.erase(jtr);
    }
	       
    for (unsigned int j = 0; j < toInsert.size(); ++j) {
	segment.insert(toInsert[j]);
    }
    
/*!!!
    Segment *segment(&m_selection->getSegment());
    m_quantizer->fixQuantizedValues
	(segment,
	 segment->findTime(m_selection->getStartTime()),
	 segment->findTime(m_selection->getEndTime()));
*/

    //!!! normalizeRests?
}

void
AdjustMenuRemoveNotationQuantizeCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    std::vector<Event *> toInsert;
    std::vector<Event *> toErase;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	toInsert.push_back(new Event(**i,
				     (*i)->getAbsoluteTime(),
				     (*i)->getDuration(),
				     (*i)->getSubOrdering(),
				     (*i)->getAbsoluteTime(),
				     (*i)->getDuration()));

	toErase.push_back(*i);
    }

    for (std::vector<Event *>::iterator i = toErase.begin(); i != toErase.end();
	 ++i) {
	m_selection->getSegment().eraseSingle(*i);
    }

    for (std::vector<Event *>::iterator i = toInsert.begin(); i != toInsert.end();
	 ++i) {
	m_selection->getSegment().insert(*i);
    }
}


const int AdjustMenuInterpretCommand::NoInterpretation      = 0;
const int AdjustMenuInterpretCommand::GuessDirections	= (1<<0);
const int AdjustMenuInterpretCommand::ApplyTextDynamics	= (1<<1);
const int AdjustMenuInterpretCommand::ApplyHairpins		= (1<<2);
const int AdjustMenuInterpretCommand::StressBeats		= (1<<3);
const int AdjustMenuInterpretCommand::Articulate		= (1<<4);
const int AdjustMenuInterpretCommand::AllInterpretations	= (1<<5) - 1;

AdjustMenuInterpretCommand::~AdjustMenuInterpretCommand()
{
    for (IndicationMap::iterator i = m_indications.begin();
	 i != m_indications.end(); ++i) {
	delete i->second;
    }
}

void
AdjustMenuInterpretCommand::modifySegment()
{
    // Of all the interpretations, Articulate is the only one that
    // changes event times or durations.  This means we must apply it
    // last, as the selection cannot be used after it's been applied,
    // because the events in the selection will have been recreated
    // with their new timings.
    
    // The default velocity for new notes is 100, and the range is
    // 0-127 (in practice this seems to be roughly logarithmic rather
    // than linear, though perhaps that's an illusion).

    // We should only apply interpretation to those events actually
    // selected, but when applying things like hairpins and text
    // dynamics we need to take into account all dynamics that may
    // cover our events even if they're not selected or are not within
    // the time range of the selection at all.  So first we'd better
    // find all the likely indications, starting at (for the sake of
    // argument) three bars before the start of the selection:

    Segment &segment(getSegment());
    
    timeT t = m_selection->getStartTime();
    for (int i = 0; i < 3; ++i) t = segment.getBarStartForTime(t);

    Segment::iterator itr = segment.findTime(t);

    while (itr != segment.end()) {
	timeT eventTime = (*itr)->getAbsoluteTime();
	if (eventTime > m_selection->getEndTime()) break;
	if ((*itr)->isa(Indication::EventType)) {
	    m_indications[eventTime] = new Rosegarden::Indication(**itr);
	}
	++itr;
    }

    //!!! need the option of ignoring current velocities or adjusting
    //them: at the moment ApplyTextDynamics ignores them and the
    //others adjust them

    if (m_interpretations & GuessDirections) guessDirections();
    if (m_interpretations & ApplyTextDynamics) applyTextDynamics();
    if (m_interpretations & ApplyHairpins) applyHairpins();
    if (m_interpretations & StressBeats) stressBeats();
    if (m_interpretations & Articulate) articulate();

    //!!! Finally, in future we should extend this to allow
    // indications on one segment (e.g. top line of piano staff) to
    // affect another (e.g. bottom line).  All together now: "Even
    // Rosegarden 2.1 could do that!"
}

void
AdjustMenuInterpretCommand::guessDirections()
{
    //...
}

void
AdjustMenuInterpretCommand::applyTextDynamics()
{
    // laborious

    Segment &segment(getSegment());
    int velocity = 100;

    timeT startTime = m_selection->getStartTime();
    timeT   endTime = m_selection->getEndTime();

    for (Segment::iterator i = segment.begin();
	 segment.isBeforeEndMarker(i); ++i) {

	timeT t = (*i)->getAbsoluteTime();

	if (t > endTime) break;

	if (Text::isTextOfType(*i, Text::Dynamic)) {

	    std::string text;
	    if ((*i)->get<String>(Text::TextPropertyName, text)) {
		velocity = getVelocityForDynamic(text);
	    }
	}

	if (t >= startTime &&
	    (*i)->isa(Note::EventType) && m_selection->contains(*i)) {
	    (*i)->set<Int>(VELOCITY, velocity);
	}
    }
}

int
AdjustMenuInterpretCommand::getVelocityForDynamic(std::string text)
{
    int velocity = 100;
		    
    // should do case-insensitive matching with whitespace
    // removed.  can surely be cleverer about this too!
    
    if      (text == "ppppp") velocity = 10;
    else if (text == "pppp")  velocity = 20;
    else if (text == "ppp")   velocity = 30;
    else if (text == "pp")    velocity = 40;
    else if (text == "p")     velocity = 60;
    else if (text == "mp")    velocity = 80;
    else if (text == "mf")    velocity = 90;
    else if (text == "f")     velocity = 105;
    else if (text == "ff")    velocity = 110;
    else if (text == "fff")   velocity = 115;
    else if (text == "ffff")  velocity = 120;
    else if (text == "fffff") velocity = 125;

    NOTATION_DEBUG << "AdjustMenuInterpretCommand::getVelocityForDynamic: unrecognised dynamic " << text << endl;

    return velocity;
}

void
AdjustMenuInterpretCommand::applyHairpins()
{
    Segment &segment(getSegment());
    int velocityToApply = -1;

    for (EventSelection::eventcontainer::iterator ecitr =
	     m_selection->getSegmentEvents().begin();
	 ecitr != m_selection->getSegmentEvents().end(); ++ecitr) {

	Event *e = *ecitr;
	if (Text::isTextOfType(e, Text::Dynamic)) {
	    velocityToApply = -1;
	}
	if (!e->isa(Note::EventType)) continue;
	bool crescendo = true;

	IndicationMap::iterator inditr =
	    findEnclosingIndication(e, Indication::Crescendo);

	// we can't be in both crescendo and decrescendo -- at least,
	// not meaningfully
	
	if (inditr == m_indications.end()) {
	    inditr = findEnclosingIndication(e, Indication::Decrescendo);
	    if (inditr == m_indications.end()) {
		if (velocityToApply > 0) {
		    e->set<Int>(VELOCITY, velocityToApply);
		}		    
		continue;
	    }
	    crescendo = false;
	}
	 
	// The starting velocity for the indication is easy -- it's
	// just the velocity of the last note at or before the
	// indication begins that has a velocity

	timeT hairpinStartTime = inditr->first;
	// ensure we scan all of the events at this time:
	Segment::iterator itr(segment.findTime(hairpinStartTime + 1));
	while (itr == segment.end() ||
	       (*itr)->getAbsoluteTime() > hairpinStartTime ||
	       !(*itr)->isa(Note::EventType) ||
	       !(*itr)->has(VELOCITY)) {
	    if (itr == segment.begin()) {
		itr = segment.end();
		break;
	    }
	    --itr;
	}

	long startingVelocity = 100;
	if (itr != segment.end()) {
	    (*itr)->get<Int>(VELOCITY, startingVelocity);
	}
	
	// The ending velocity is harder.  If there's a dynamic change
	// directly after the hairpin, then we want to use that
	// dynamic's velocity unless it opposes the hairpin's
	// direction.  If there isn't, or it does oppose the hairpin,
	// we should probably make the degree of change caused by the
	// hairpin depend on its total duration.

	long endingVelocity = startingVelocity;
	timeT hairpinEndTime = inditr->first +
                               inditr->second->getIndicationDuration();
	itr = segment.findTime(hairpinEndTime);
	while (itr != segment.end()) {
	    if (Text::isTextOfType(*itr, Text::Dynamic)) {
		std::string text;
		if ((*itr)->get<String>(Text::TextPropertyName, text)) {
		    endingVelocity = getVelocityForDynamic(text);
		    break;
		}
	    }
	    if ((*itr)->getAbsoluteTime() >
		(hairpinEndTime + Note(Note::Crotchet).getDuration())) break;
	    ++itr;
	}

	if (( crescendo && (endingVelocity < startingVelocity)) ||
	    (!crescendo && (endingVelocity > startingVelocity))) {
	    // we've got it wrong; prefer following the hairpin to
	    // following whatever direction we got the dynamic from
	    endingVelocity = startingVelocity;
	    // and then fall into the next conditional to set up the
	    // velocities
	}

	if (endingVelocity == startingVelocity) {
	    // calculate an ending velocity based on starting velocity
	    // and hairpin duration (okay, we'll leave that bit for later)
	    endingVelocity = startingVelocity * (crescendo ? 120 : 80) / 100;
	}
	
	double proportion = 
	    (double(e->getAbsoluteTime() - hairpinStartTime) /
	     double(hairpinEndTime - hairpinStartTime));
	long velocity =
	    int((endingVelocity - startingVelocity) * proportion +
		startingVelocity);

	NOTATION_DEBUG << "AdjustMenuInterpretCommand::applyHairpins: velocity of note at " << e->getAbsoluteTime() << " is " << velocity << " (" << proportion << " through hairpin from " << startingVelocity << " to " << endingVelocity <<")" << endl;
	if (velocity < 10) velocity = 10;
	if (velocity > 127) velocity = 127;
	e->set<Int>(VELOCITY, velocity);
	velocityToApply = velocity;
    }
}

void
AdjustMenuInterpretCommand::stressBeats()
{
    Rosegarden::Composition *c = getSegment().getComposition();

    for (EventSelection::eventcontainer::iterator itr =
	     m_selection->getSegmentEvents().begin();
	 itr != m_selection->getSegmentEvents().end(); ++itr) {

	Event *e = *itr;
	if (!e->isa(Note::EventType)) continue;

	timeT t = e->getNotationAbsoluteTime();
	Rosegarden::TimeSignature timeSig = c->getTimeSignatureAt(t);
	timeT barStart = getSegment().getBarStartForTime(t);
	int stress = timeSig.getEmphasisForTime(t - barStart);

	// stresses are from 0 to 4, so we add 12% to the velocity
	// at the maximum stress, subtract 4% at the minimum
	int velocityChange = stress * 4 - 4;

	// do this even if velocityChange == 0, in case the event
	// has no velocity yet
	long velocity = 100;
	e->get<Int>(VELOCITY, velocity);
	velocity += velocity * velocityChange / 100;
	if (velocity < 10) velocity = 10;
	if (velocity > 127) velocity = 127;
	e->set<Int>(VELOCITY, velocity);
    }
}

void
AdjustMenuInterpretCommand::articulate()
{
    // Basic articulations:
    //
    // -- Anything marked tenuto or within a slur gets 100% of its
    //    nominal duration (that's what we need the quantizer for,
    //    to get the display nominal duration), and its velocity
    //    is unchanged.
    // 
    // -- Anything marked marcato gets 60%, or 70% if slurred (!),
    //    and gets an extra 15% of velocity.
    //
    // -- Anything marked staccato gets 55%, or 70% if slurred,
    //    and unchanged velocity.
    //
    // -- Anything marked staccatissimo gets 30%, or 50% if slurred (!),
    //    and loses 5% of velocity.
    // 
    // -- Anything marked sforzando gains 35% of velocity.
    // 
    // -- Anything marked with an accent gains 30% of velocity.
    // 
    // -- Anything marked rinforzando gains 15% of velocity and has
    //    its full duration.  Guess we really need to use some proper
    //    controllers here.
    // 
    // -- Anything marked down-bow gains 5% of velocity, anything
    //    marked up-bow loses 5%.
    // 
    // -- Anything unmarked and unslurred, or marked tenuto and
    //    slurred, gets 90% of duration.

    std::set<Event *> toErase;
    std::set<Event *> toInsert;
    Segment &segment(getSegment());

    for (EventSelection::eventcontainer::iterator ecitr =
	     m_selection->getSegmentEvents().begin();
	 ecitr != m_selection->getSegmentEvents().end(); ++ecitr) {

	Event *e = *ecitr;
	if (!e->isa(Note::EventType)) continue;
	Segment::iterator itr = segment.findSingle(e);
	Rosegarden::Chord chord(segment, itr, m_quantizer);

	// the things that affect duration
	bool staccato = false;
	bool staccatissimo = false;
	bool marcato = false;
	bool tenuto = false;
	bool rinforzando = false;
	bool slurred = false;

	int velocityChange = 0;

	std::vector<Mark> marks(chord.getMarksForChord());
	
	for (std::vector<Mark>::iterator i = marks.begin();
	     i != marks.end(); ++i) {

	    if (*i == Rosegarden::Marks::Accent) {
		velocityChange += 30;
	    } else if (*i == Rosegarden::Marks::Tenuto) {
		tenuto = true;
	    } else if (*i == Rosegarden::Marks::Staccato) {
		staccato = true;
	    } else if (*i == Rosegarden::Marks::Staccatissimo) {
		staccatissimo = true;
		velocityChange -= 5;
	    } else if (*i == Rosegarden::Marks::Marcato) {
		marcato = true;
		velocityChange += 15;
	    } else if (*i == Rosegarden::Marks::Sforzando) {
		velocityChange += 35;
	    } else if (*i == Rosegarden::Marks::Rinforzando) {
		rinforzando = true;
		velocityChange += 15;
	    } else if (*i == Rosegarden::Marks::DownBow) {
		velocityChange += 5;
	    } else if (*i == Rosegarden::Marks::UpBow) {
		velocityChange -= 5;
	    }
	}

	IndicationMap::iterator inditr =
	    findEnclosingIndication(e, Indication::Slur);

	if (inditr != m_indications.end()) slurred = true;
	if (slurred) {
	    // last note in a slur should be treated as if unslurred
	    timeT slurEnd =
		inditr->first + inditr->second->getIndicationDuration();
	    if (slurEnd == e->getNotationAbsoluteTime() + e->getNotationDuration() ||
		slurEnd == e->getAbsoluteTime() + e->getDuration()) {
		slurred = false;
	    }
/*!!!
	    Segment::iterator slurEndItr = segment.findTime(slurEnd);
	    if (slurEndItr != segment.end() &&
		(*slurEndItr)->getNotationAbsoluteTime() <=
		            e->getNotationAbsoluteTime()) {
		slurred = false;
	    } 
*/
	}

	int durationChange = 0;
	
	if (slurred) {
	    //!!! doesn't seem to be picking up slurs correctly
	    if (tenuto) durationChange = -10;
	    else if (marcato || staccato) durationChange = -30;
	    else if (staccatissimo) durationChange = -50;
	    else durationChange = 0;
	} else {
	    if (tenuto) durationChange = 0;
	    else if (marcato) durationChange = -40;
	    else if (staccato) durationChange = -45;
	    else if (staccatissimo) durationChange = -70;
	    else if (rinforzando) durationChange = 0;
	    else durationChange = -10;
	}

	NOTATION_DEBUG << "AdjustMenuInterpretCommand::modifySegment: chord has " << chord.size() << " notes in it" << endl;

	for (Rosegarden::Chord::iterator ci = chord.begin();
	     ci != chord.end(); ++ci) {

	    e = **ci;

	NOTATION_DEBUG << "AdjustMenuInterpretCommand::modifySegment: For note at " << e->getAbsoluteTime() << ", velocityChange is " << velocityChange << " and durationChange is " << durationChange << endl;

	    // do this even if velocityChange == 0, in case the event
	    // has no velocity yet
	    long velocity = 100;
	    e->get<Int>(VELOCITY, velocity);
	    velocity += velocity * velocityChange / 100;
	    if (velocity < 10) velocity = 10;
	    if (velocity > 127) velocity = 127;
	    e->set<Int>(VELOCITY, velocity);

	    timeT duration = e->getNotationDuration();
	    
	    // don't mess with the duration of a tied note
	    bool tiedForward = false;
	    if (e->get<Bool>(TIED_FORWARD, tiedForward) && tiedForward) {
		durationChange = 0;
	    }

	    timeT newDuration = duration + duration * durationChange / 100;
	    
	    // this comparison instead of "durationChange != 0"
	    // because we want to permit the possibility of resetting
	    // the performance duration of a note (that's perhaps been
	    // articulated wrongly) based on the notation duration:

	    if (e->getDuration() != newDuration) {

		if (toErase.find(e) == toErase.end()) {
		
		    //!!! deal with tuplets
		    
		    Event *newEvent = new Event(*e,
						e->getAbsoluteTime(),
						newDuration,
						e->getSubOrdering(),
						e->getNotationAbsoluteTime(),
						duration);
		    toInsert.insert(newEvent);
		    toErase.insert(e);
		}
	    }
	}

	// what we want to do here is jump our iterator to the final
	// element in the chord -- but that doesn't work because we're
	// iterating through the selection, not the segment.  So for
	// now we just accept the fact that notes in chords might be
	// processed multiple times (slow) and added into the toErase
	// set more than once (hence the nasty tests in the loop just
	// after the close of this loop).
    }

    for (std::set<Event *>::iterator j = toErase.begin(); j != toErase.end(); ++j) {
	Segment::iterator jtr(segment.findSingle(*j));
	if (jtr != segment.end()) segment.erase(jtr);
    }
	       
    for (std::set<Event *>::iterator j = toInsert.begin(); j != toInsert.end(); ++j) {
	segment.insert(*j);
    }
}

AdjustMenuInterpretCommand::IndicationMap::iterator
AdjustMenuInterpretCommand::findEnclosingIndication(Event *e,
							std::string type)
{
    // a bit slow, but let's wait and see whether it's a bottleneck
    // before we worry about that

    timeT t = e->getAbsoluteTime();
    IndicationMap::iterator itr = m_indications.lower_bound(t);

    while (1) {
	if (itr != m_indications.end()) {

	    if (itr->second->getIndicationType() == type &&
		itr->first <= t &&
		itr->first + itr->second->getIndicationDuration() > t) {
		return itr;
	    }
	}
	if (itr == m_indications.begin()) break;
	--itr;
    }

    return m_indications.end();
}

QString
RespellCommand::getGlobalName(Type type, Accidental accidental)
{
    switch(type) {

    case Set:
    {
	QString s(i18n("Respell with %1"));
	//!!! should be in notationstrings:
	if (accidental == DoubleSharp) {
	    s = s.arg(i18n("Do&uble Sharp"));
	} else if (accidental == Sharp) {
	    s = s.arg(i18n("&Sharp"));
	} else if (accidental == Flat) {
	    s = s.arg(i18n("&Flat"));
	} else if (accidental == DoubleFlat) {
	    s = s.arg(i18n("Dou&ble Flat"));
	} else if (accidental == Natural) {
	    s = s.arg(i18n("&Natural"));
	} else {
	    s = s.arg(i18n("N&one"));
	}
	return s;
    }

    case Up:
	return i18n("Respell Accidentals &Upward");

    case Down:
	return i18n("Respell Accidentals &Downward");

    case Restore:
	return i18n("&Restore Computed Accidentals");
    }

    return i18n("Respell Accidentals");
}

void
RespellCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if ((*i)->isa(Note::EventType)) {
	    
	    if (m_type == Up || m_type == Down) {

		Accidental acc = NoAccidental;
		(*i)->get<String>(ACCIDENTAL, acc);
		
		if (m_type == Down) {
		    if (acc == DoubleFlat) {
			acc =  Flat;
		    } else if (acc == Flat || acc == NoAccidental) {
			acc =  Sharp;
		    } else if (acc == Sharp) {
			acc =  DoubleSharp;
		    }
		} else {
		    if (acc == Flat) {
			acc =  DoubleFlat;
		    } else if (acc == Sharp || acc == NoAccidental) {
			acc =  Flat;
		    } else if (acc == DoubleSharp) {
			acc =  Sharp;
		    }
		}

		(*i)->set<String>(ACCIDENTAL, acc);

	    } else if (m_type == Set) {
		
		// trap respelling black key notes as natural; which is
		// impossible, and makes rawPitchToDisplayPitch() do crazy
		// things as a consequence (fixes #1349782)
		// 1 = C#, 3 = D#, 6 = F#, 8 = G#, 10 = A#
		long pitch;
		(*i)->get<Int>(PITCH, pitch);
		pitch %= 12;
		if ((pitch == 1 || pitch == 3 || pitch == 6 || pitch == 8 || pitch == 10 )
			&& m_accidental == Natural) {
		    // fail silently; is there anything to do here?
		} else {
                    (*i)->set<String>(ACCIDENTAL, m_accidental);
		}

	    } else {

		(*i)->unset(ACCIDENTAL);
	    }
	}
    }
}


QString
MakeAccidentalsCautionaryCommand::getGlobalName(bool cautionary)
{
    if (cautionary) return i18n("Use &Cautionary Accidentals");
    else return i18n("Cancel C&autionary Accidentals");
}

void
MakeAccidentalsCautionaryCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if ((*i)->isa(Note::EventType)) {
	    if (m_cautionary) {
		(*i)->set<Bool>(NotationProperties::USE_CAUTIONARY_ACCIDENTAL,
				true);
	    } else {
		(*i)->unset(NotationProperties::USE_CAUTIONARY_ACCIDENTAL);
	    }
	}
    }
}


void
IncrementDisplacementsCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	long prevX = 0, prevY = 0;
	(*i)->get<Int>(DISPLACED_X, prevX);
	(*i)->get<Int>(DISPLACED_Y, prevY);
	(*i)->setMaybe<Int>(DISPLACED_X, prevX + long(m_dx));
	(*i)->setMaybe<Int>(DISPLACED_Y, prevY + long(m_dy));
    }
}


void
ResetDisplacementsCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	(*i)->unset(DISPLACED_X);
	(*i)->unset(DISPLACED_Y);
    }
}


void
SetVisibilityCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if (m_visible) {
	    (*i)->unset(INVISIBLE);
	} else {
	    (*i)->set<Bool>(INVISIBLE, true);
	}
    }
}


FretboardInsertionCommand::FretboardInsertionCommand(Segment &segment,
        Rosegarden::timeT time,
        Guitar::Fingering chord) :
        BasicCommand(i18n("Insert Fretboard"), segment, time, time + 1, true),
        m_chord(chord)
{
    // nothing
}

FretboardInsertionCommand::~FretboardInsertionCommand()
{}

void
FretboardInsertionCommand::modifySegment()
{
    Segment::iterator i = getSegment().insert(m_chord.getAsEvent(getStartTime()));
    if (i != getSegment().end())
    {
        m_lastInsertedEvent = *i;
    }
}



		
