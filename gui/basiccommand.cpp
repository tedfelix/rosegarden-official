// -*- c-basic-offset: 4 -*-

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

#include "basiccommand.h"
#include "Selection.h"

#include <qstring.h>

#include "rosedebug.h"

using Rosegarden::Segment;
using Rosegarden::EventSelection;
using Rosegarden::Event;
using Rosegarden::timeT;

BasicCommand::BasicCommand(const QString &name, Segment &segment,
			   timeT begin, timeT end, bool bruteForceRedo) :
    IntraSegmentCommand(name),
    m_segment(segment),
    m_savedEvents(segment.getType(), begin),
    m_endTime(end),
    m_doBruteForceRedo(false),
    m_redoEvents(0)
{
    if (bruteForceRedo)
        m_redoEvents = new Segment(segment.getType(), begin);
}

BasicCommand::~BasicCommand()
{
    m_savedEvents.clear();
    if (m_redoEvents) m_redoEvents->clear();
    delete m_redoEvents;
}

void
BasicCommand::beginExecute()
{
    copyTo(&m_savedEvents);
}

void
BasicCommand::execute()
{
    beginExecute();

    if (!m_doBruteForceRedo) {

	modifySegment();

    } else {
	copyFrom(m_redoEvents);
    }
}

void
BasicCommand::unexecute()
{
    if (m_redoEvents) {
	copyTo(m_redoEvents);
	m_doBruteForceRedo = true;
    }

    copyFrom(&m_savedEvents);
}
    
void
BasicCommand::copyTo(Rosegarden::Segment *events)
{
    for (Segment::iterator i = m_segment.findTime(events->getStartTime());
	 i != m_segment.findTime(m_endTime); ++i) {
	events->insert(new Event(**i));
    }
}
   
void
BasicCommand::copyFrom(Rosegarden::Segment *events)
{
    m_segment.erase(m_segment.findTime(events->getStartTime()),
		    m_segment.findTime(m_endTime));

    for (Segment::iterator i = events->begin(); i != events->end(); ++i) {
	m_segment.insert(new Event(**i));
    }

    events->clear();
}



BasicSelectionCommand::BasicSelectionCommand(const QString &name,
					     EventSelection &selection,
					     bool bruteForceRedo) :
    BasicCommand(name,
		 selection.getSegment(),
		 selection.getBeginTime(),
		 selection.getEndTime(),
		 bruteForceRedo)
{
    // nothing
}

BasicSelectionCommand::~BasicSelectionCommand()
{
    // nothing
}
