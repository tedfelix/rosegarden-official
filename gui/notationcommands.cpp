/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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
#include "eventselection.h"
#include "notationview.h"
#include "rosegardenguidoc.h"

#include "rosedebug.h"
#include <iostream>

using Rosegarden::Segment;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::Event;
using Rosegarden::timeT;
using Rosegarden::Note;
using Rosegarden::Accidental;
using Rosegarden::NoAccidental;

using std::cerr;
using std::endl;


BasicCommand::BasicCommand(const QString &name, Segment &segment,
			   timeT begin, timeT end) :
    KCommand(name),
    m_segment(segment),
    m_savedEvents(begin),
    m_endTime(end)
{
    // nothing
}

BasicCommand::~BasicCommand()
{
    deleteSavedEvents();
}

void
BasicCommand::beginExecute()
{
    for (Segment::iterator i = m_segment.findTime
	     (m_savedEvents.getStartIndex());
	 i != m_segment.findTime(m_endTime); ++i) {
	m_savedEvents.insert(new Event(**i));
    }

    //!!! handle through command history stuff
//    m_view->getDocument()->setModified();
}

void
BasicCommand::finishExecute()
{
    //!!! we could do this a lot more elegantly with a signal
    NotationView::redoLayout(&m_segment,
			     m_savedEvents.getStartIndex(),
			     getRelayoutEndTime());
}

void
BasicCommand::deleteSavedEvents()
{
    m_savedEvents.erase(m_savedEvents.begin(), m_savedEvents.end());
}

void
BasicCommand::execute()
{
    beginExecute();
    SegmentNotationHelper helper(m_segment);
    modifySegment(helper);
    finishExecute();
}

void
BasicCommand::unexecute()
{
    m_segment.erase(m_segment.findTime(m_savedEvents.getStartIndex()),
		    m_segment.findTime(m_endTime));

    for (Segment::iterator i = m_savedEvents.begin();
	 i != m_savedEvents.end(); ++i) {
	m_segment.insert(new Event(**i));
    }

    deleteSavedEvents();
    finishExecute();
}
    
BasicSelectionCommand::BasicSelectionCommand(const QString &name,
					     EventSelection &selection) :
    BasicCommand(name,
		 selection.getSegment(),
		 selection.getBeginTime(),
		 selection.getEndTime())
{
    // nothing else
}

BasicSelectionCommand::~BasicSelectionCommand()
{
    // nothing
}


NoteInsertionCommand::NoteInsertionCommand(const QString &name,
					   Segment &segment, timeT time,
                                           timeT endTime, Note note, int pitch,
                                           Accidental accidental) :
    BasicCommand(name, segment, time, endTime),
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
NoteInsertionCommand::modifySegment(SegmentNotationHelper &helper)
{
    Segment::iterator i =
        helper.insertNote(getBeginTime(), m_note, m_pitch, m_accidental);
    if (i != helper.segment().end()) m_lastInsertedEvent = *i;
}


RestInsertionCommand::RestInsertionCommand(const QString &name,
					   Segment &segment, timeT time,
                                           timeT endTime, Note note) :
    NoteInsertionCommand(name, segment, time, endTime,
                         note, 0, NoAccidental)
{
    // nothing
}

RestInsertionCommand::~RestInsertionCommand()
{
    // nothing
}

void
RestInsertionCommand::modifySegment(SegmentNotationHelper &helper)
{
    Segment::iterator i =
        helper.insertRest(getBeginTime(), m_note);
    if (i != helper.segment().end()) m_lastInsertedEvent = *i;
}
