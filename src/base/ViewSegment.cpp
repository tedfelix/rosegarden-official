/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ViewSegment.h"
#include "base/Profiler.h"

#include <QtGlobal>

namespace Rosegarden 
{

ViewSegment::ViewSegment(Segment &t) :
    m_segment(t),
    m_viewElementList(nullptr)
{
    // empty
}

ViewSegment::~ViewSegment()
{
    if (m_viewElementList) m_segment.removeObserver(this);
    notifySourceDeletion();
    delete m_viewElementList;
}

ViewElementList *
ViewSegment::getViewElementList()
{
    if (!m_viewElementList) {

        m_viewElementList = new ViewElementList;

        for (Segment::const_iterator i = m_segment.begin(); i != m_segment.end(); ++i) {

            if (!wrapEvent(*i)) continue;

            ViewElement *el = makeViewElement(*i);
            m_viewElementList->insert(el);
        }

        m_segment.addObserver(this);
    }
    
    return m_viewElementList;
}

bool
ViewSegment::wrapEvent(Event *e)
{
    timeT emt = m_segment.getEndMarkerTime();
    return
	(e->getAbsoluteTime() <  emt) ||
	(e->getAbsoluteTime() == emt && e->getDuration() == 0);
}

ViewElementList::iterator
ViewSegment::findEvent(Event *e)
{
    // Note that we have to create this using the virtual
    // makeViewElement, because the result of equal_range depends on
    // the value of the view absolute time for the element, which
    // depends on the particular subclass of ViewElement in use.
    
    // (This is also why this method has to be here and not in
    // ViewElementList -- ViewElementList has no equivalent of
    // makeViewElement.)
    
    ViewElement *dummy = makeViewElement(e);
    
    std::pair<ViewElementList::iterator,
  	      ViewElementList::iterator>
        r = m_viewElementList->equal_range(dummy);
 
    delete dummy;

    for (ViewElementList::iterator i = r.first; i != r.second; ++i) {
        if ((*i)->event() == e) {
            return i;
        }
    }

    return m_viewElementList->end();
}

void
ViewSegment::eventAdded(const Segment *t, Event *e)
{
    Profiler profiler("ViewSegment::eventAdded");
    Q_ASSERT(t == &m_segment);
    (void)t; // avoid warnings

    if (wrapEvent(e)) {
        ViewElement *el = makeViewElement(e);
        m_viewElementList->insert(el);
        notifyAdd(el);
    }
}

void
ViewSegment::eventRemoved(const Segment *t, Event *e)
{
    Profiler profiler("ViewSegment::eventRemoved");
    Q_ASSERT(t == &m_segment);
    (void)t; // avoid warnings

    // If we have it, lose it

    ViewElementList::iterator i = findEvent(e);
    if (i != m_viewElementList->end()) {
        notifyRemove(*i);
        m_viewElementList->erase(i);
        return;
    }

//    std::cerr << "Event at " << e->getAbsoluteTime() << ", notation time " << e->getNotationAbsoluteTime() << ", type " << e->getType()
//	      << " not found in ViewSegment" << std::endl;
}

void
ViewSegment::endMarkerTimeChanged(const Segment *segment, bool shorten)
{
    Segment *s = const_cast<Segment *>(segment);

    Q_ASSERT(s == &m_segment);

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
		if (wrapEvent(*j)) {
		    m_viewElementList->insert(makeViewElement(*j));
		}
	    }
	}
    }
}
void
ViewSegment::segmentDeleted(const Segment *s)
{
    Q_ASSERT(s == &m_segment);
    (void)s; // avoid warnings
    /*
    std::cerr << "WARNING: ViewSegment notified of segment deletion: this is probably a bug "
	      << "(ViewSegment should have been deleted before Segment)" << std::endl;
              */
}

void
ViewSegment::notifyAdd(ViewElement *e) const
{
    Profiler profiler("ViewSegment::notifyAdd");
    for (ObserverSet::const_iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->elementAdded(this, e);
    }
}

void
ViewSegment::notifyRemove(ViewElement *e) const
{
    Profiler profiler("ViewSegment::notifyRemove");
    for (ObserverSet::const_iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->elementRemoved(this, e);
    }
}

void
ViewSegment::notifySourceDeletion() const
{
    for (ObserverSet::const_iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->viewSegmentDeleted(this);
    }
}


}
