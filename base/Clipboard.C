
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

#include "Clipboard.h"
#include "Selection.h"

namespace Rosegarden
{

Clipboard::Clipboard()
{
    // nothing
}

Clipboard::Clipboard(const Clipboard &c) 
{
    copyFrom(&c);
}

Clipboard &
Clipboard::operator=(const Clipboard &c)
{
    copyFrom(&c);
    return *this;
}

Clipboard::~Clipboard()
{
    clear();
}

void
Clipboard::clear()
{
    for (iterator i = begin(); i != end(); ++i) {
	delete *i;
    }
    m_segments.clear();
}

bool
Clipboard::isEmpty() const
{
    return (m_segments.size() == 0);
}

bool
Clipboard::isSingleSegment() const
{
    return (m_segments.size() == 1);
}

Segment *
Clipboard::getSingleSegment() const
{
    if (isSingleSegment()) return *begin();
    else return 0;
}

Segment *
Clipboard::newSegment()
{
    Segment *s = new Segment();
    m_segments.insert(s);
    return s;
}

Segment *
Clipboard::newSegment(const Segment *copyFrom)
{
    Segment *s = new Segment(*copyFrom);
    m_segments.insert(s);
    return s;
}

Segment *
Clipboard::newSegment(const Segment *copyFrom, timeT from, timeT to)
{
    // create with copy ctor so as to inherit track, instrument etc
    Segment *s = new Segment(*copyFrom);

    if (from == s->getStartTime() && to == s->getEndTime()) {
	m_segments.insert(s);
	return s;
    }

    s->erase(s->begin(), s->end());

    Segment::iterator ifrom = copyFrom->findTime(from);
    Segment::iterator ito   = copyFrom->findTime(to);

    for (Segment::iterator i = ifrom; i != ito && i != copyFrom->end(); ++i) {
	s->insert(new Event(**i));
    }

#ifdef OLD_SEGMENT_API
    s->recalculateStartTime();
#endif
    m_segments.insert(s);
    return s;
}

Segment *
Clipboard::newSegment(const EventSelection *copyFrom)
{
    // create with copy ctor so as to inherit track, instrument etc
    Segment *s = new Segment(copyFrom->getSegment());
    s->erase(s->begin(), s->end());

    const EventSelection::eventcontainer &events(copyFrom->getSegmentEvents());
    for (EventSelection::eventcontainer::iterator i = events.begin();
	 i != events.end(); ++i) {
	s->insert(new Event(**i));
    }

#ifdef OLD_SEGMENT_API
    s->recalculateStartTime();
#endif
    m_segments.insert(s);
    return s;
}

void
Clipboard::copyFrom(const Clipboard *c)
{
    if (c == this) return;
    clear();

    for (Clipboard::iterator i = c->begin(); i != c->end(); ++i) {
	newSegment(*i);
    }
}

}
