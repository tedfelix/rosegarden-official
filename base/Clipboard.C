
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
    Segment *s = new Segment();
    m_segments.insert(s);

    for (Segment::iterator i = copyFrom->begin(); i != copyFrom->end(); ++i) {
	s->insert(new Event(**i));
    }

    return s;
}

Segment *
Clipboard::newSegment(const Segment *copyFrom, timeT from, timeT to)
{
    Segment *s = new Segment();
    m_segments.insert(s);

    Segment::iterator ifrom = copyFrom->findTime(from);
    Segment::iterator ito   = copyFrom->findTime(to);

    for (Segment::iterator i = ifrom; i != ito && i != copyFrom->end(); ++i) {
	s->insert(new Event(**i));
    }

    return s;
}

Segment *
Clipboard::newSegment(const EventSelection *copyFrom)
{
    Segment *s = new Segment();
    m_segments.insert(s);

    const EventSelection::eventcontainer &events(copyFrom->getSegmentEvents());
    for (EventSelection::eventcontainer::iterator i = events.begin();
	 i != events.end(); ++i) {
	s->insert(new Event(**i));
    }

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
