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

#include <qregexp.h>

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

PasteEventsCommand::PasteTypeMap
PasteEventsCommand::getPasteTypes()
{
    static PasteTypeMap types;
    static bool haveTypes = false;
    if (!haveTypes) {
	types[Restricted] =
	    "Paste into an existing gap [\"restricted\"]";
	types[Simple] =
	    "Erase existing events to make room [\"simple\"]";
	types[OpenAndPaste] =
	    "Move existing events out of the way [\"open-n-paste\"]";
	types[NoteOverlay] =
	    "Overlay notes, tying against present notes [\"note-overlay\"]";
	types[MatrixOverlay] =
	    "Overlay notes, ignoring present notes [\"matrix-overlay\"]";
    }
    return types;
}

timeT
PasteEventsCommand::getEffectiveEndTime(Rosegarden::Segment &segment,
					Rosegarden::Clipboard *clipboard,
					Rosegarden::timeT pasteTime)
{
    if (!clipboard->isSingleSegment()) return pasteTime;
    
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

    RG_DEBUG << "NotationView::slotEditPaste: paste time is " << pasteTime << ", origin is " << origin << ", duration is " << duration << endl;

    SegmentNotationHelper helper(getSegment());
    return helper.removeRests(pasteTime, duration, true);
}


void
PasteEventsCommand::modifySegment()
{
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
		    (**i, (*i)->getAbsoluteTime() - origin + pasteTime);
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

    // Get start and end times
    //
    for (;i != m_selection->getSegmentEvents().end(); ++i)
    {
        if ((*i)->getAbsoluteTime() < startTime)
            startTime = (*i)->getAbsoluteTime();
        
        if ((*i)->getAbsoluteTime() > endTime)
            endTime = (*i)->getAbsoluteTime();
    }

    double step = double(m_value1 - m_value2) / double(endTime - startTime);
    double lowStep = double(m_value2) / double(endTime - startTime);

    for (i = m_selection->getSegmentEvents().begin();
         i != m_selection->getSegmentEvents().end(); ++i)
    {
        if (m_pattern == Rosegarden::Flat)
            (*i)->set<Rosegarden::Int>(m_property, m_value1);
        else if (m_pattern == Rosegarden::Alternating)
        {
            if (count % 2 == 0)
                (*i)->set<Rosegarden::Int>(m_property, m_value1);
            else
                (*i)->set<Rosegarden::Int>(m_property, m_value2);

        } else if (m_pattern == Rosegarden::Crescendo)
        {

            (*i)->set<Rosegarden::Int>(m_property,
                                       m_value2 +
                                       int(step * (*i)->getAbsoluteTime()));
        } else if (m_pattern == Rosegarden::Diminuendo)
        {
            (*i)->set<Rosegarden::Int>(m_property,
                                       m_value1 -
                                       int(step * (*i)->getAbsoluteTime()));
        } else if (m_pattern == Rosegarden::Ringing)
        {
            if (count % 2 == 0)
                (*i)->set<Rosegarden::Int>
                    (m_property,
                     m_value1 - int(step * (*i)->getAbsoluteTime()));
            else
            {
                int value = m_value2 - int(lowStep * (*i)->getAbsoluteTime());
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

        m_quantizer.quantize(m_selection);

    } else {
	m_quantizer.quantize(&segment,
			     segment.findTime(getStartTime()),
			     segment.findTime(getEndTime()));
    }
}


// ---------------- Unquantize -----------
EventUnquantizeCommand::EventUnquantizeCommand(Rosegarden::Segment &segment,
					       Rosegarden::timeT startTime,
					       Rosegarden::timeT endTime,
					       Rosegarden::Quantizer quantizer) :
    BasicCommand(i18n("Unquantize Events"), segment, startTime, endTime,
		 true), // bruteForceRedo
    m_quantizer(quantizer),
    m_selection(0)
{
    // nothing else
}

EventUnquantizeCommand::EventUnquantizeCommand(
        Rosegarden::EventSelection &selection,
        Rosegarden::Quantizer quantizer) :
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

QString
EventUnquantizeCommand::getGlobalName(Rosegarden::Quantizer *quantizer)
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
EventUnquantizeCommand::modifySegment()
{
    Segment &segment = getSegment();

    if (m_selection) {

        m_quantizer.unquantize(m_selection);

    } else {
	m_quantizer.unquantize(&segment,
			       segment.findTime(getStartTime()),
			       segment.findTime(getEndTime()));
    }
}


SetLyricsCommand::SetLyricsCommand(Segment *segment, QString newLyricData) :
    XKCommand(getGlobalName()),
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
		    (*i)->getAbsoluteTime() <= laterThan)) ++i;

	    timeT time = m_segment->getEndMarkerTime();
	    if (m_segment->isBeforeEndMarker(i))
		time = (*i)->getAbsoluteTime();

	    QString syllable = *ssi;
	    syllable.replace(QRegExp("~"), " ");
	    syllable = syllable.simplifyWhiteSpace();
	    if (syllable == "") continue;
	    laterThan = time + 1;
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

    
