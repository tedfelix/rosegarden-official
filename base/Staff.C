// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#include "Staff.h"

namespace Rosegarden 
{

Staff::Staff(Segment &t) :
    m_segment(t),
    m_viewElementList(0)
{
    // empty
}

Staff::~Staff()
{
    if (m_viewElementList) m_segment.removeObserver(this);
}

ViewElementList *
Staff::getViewElementList()
{
    return getViewElementList(m_segment.begin(), m_segment.end());
}

ViewElementList *
Staff::getViewElementList(Segment::iterator from,
                          Segment::iterator to)
{
    if (!m_viewElementList) {

        m_viewElementList = new ViewElementList;

        for (Segment::iterator i = from; i != to; ++i) {

            if (!wrapEvent(*i)) continue;

            ViewElement *el = makeViewElement(*i);
            m_viewElementList->insert(el);
        }

        m_segment.addObserver(this);

    }
    
    return m_viewElementList;
}

bool
Staff::wrapEvent(Event *e)
{
    timeT emt = m_segment.getEndMarkerTime();
    return
	(e->getAbsoluteTime() <  emt) ||
	(e->getAbsoluteTime() == emt && e->getDuration() == 0);
}

ViewElementList::iterator
Staff::findEvent(Event *e)
{
    ViewElement dummy(e);

    std::pair<ViewElementList::iterator,
	      ViewElementList::iterator>
        r = m_viewElementList->equal_range(&dummy);

    for (ViewElementList::iterator i = r.first; i != r.second; ++i) {
        if ((*i)->event() == e) {
            return i;
        }
    }

    return m_viewElementList->end();
}

void
Staff::eventAdded(const Segment *t, Event *e)
{
    assert(t == &m_segment);

    if (wrapEvent(e)) {
        ViewElement *el = makeViewElement(e);
        m_viewElementList->insert(el);
    }
}

void
Staff::eventRemoved(const Segment *t, Event *e)
{
    assert(t == &m_segment);

    // If we have it, lose it

//    std::cerr << "Staff::eventRemoved for " << e << std::endl;

    ViewElementList::iterator i = findEvent(e);
    if (i != m_viewElementList->end()) {
//	std::cerr << "Found" << std::endl;
        m_viewElementList->erase(i);
        return;
    }

    std::cerr << "Event at " << e->getAbsoluteTime() << ", type " << e->getType()
	      << " not found in Staff" << std::endl;
}

void
Staff::endMarkerTimeChanged(const Segment *s, bool shorten)
{
    assert(s == &m_segment);

    if (shorten) {

	m_viewElementList->erase
	    (m_viewElementList->findTime(s->getEndMarkerTime()),
	     m_viewElementList->end());

    } else {

	timeT myLastEltTime = s->getStartTime();
	if (m_viewElementList->end() != m_viewElementList->begin()) {
	    ViewElementList::iterator i = m_viewElementList->end();
	    myLastEltTime = (*--i)->event()->getAbsoluteTime();
	}
	
	for (Segment::iterator j = s->findTime(myLastEltTime);
	     s->isBeforeEndMarker(j); ++j) {
	    
	    ViewElementList::iterator newi = findEvent(*j);
	    if (newi == m_viewElementList->end()) {
		m_viewElementList->insert(makeViewElement(*j));
	    }
	}
    }
}

}
