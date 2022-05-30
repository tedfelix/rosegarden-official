/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ViewSegment.h"

#include <QtGlobal>

namespace Rosegarden
{


ViewSegment::ViewSegment(Segment &t) :
    m_segment(t),
    m_viewElementList(nullptr),
    m_modified(false),
    m_modStart(0),
    m_modEnd(0)
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

        for (Segment::const_iterator i = m_segment.begin();
             i != m_segment.end();
             ++i) {

            if (!wrapEvent(*i))
                continue;

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
    // This is more of an isBeforeSegmentEnd().

    timeT endMarkerTime = m_segment.getEndMarkerTime();

    return
        ((e->getAbsoluteTime() < endMarkerTime)  ||
        (e->getAbsoluteTime() == endMarkerTime  &&  e->getDuration() == 0));
}

ViewElementList::iterator
ViewSegment::findEvent(const Event *e)
{
    // Note that we have to create this using the virtual
    // makeViewElement(), because the result of equal_range() depends on
    // the value of the view absolute time for the element, which
    // depends on the particular subclass of ViewElement in use.

    // (This is also why this method has to be here and not in
    // ViewElementList -- ViewElementList has no equivalent of
    // makeViewElement.)

    // Cast away const since this is a temp we will not modify.
    ViewElement *dummy = makeViewElement(const_cast<Event *>(e));

    std::pair<ViewElementList::iterator,
              ViewElementList::iterator>
        r = m_viewElementList->equal_range(dummy);

    delete dummy;

    for (ViewElementList::iterator i = r.first; i != r.second; ++i) {
        if ((*i)->event() == e)
            return i;
    }

    return m_viewElementList->end();
}

void
ViewSegment::eventAdded(const Segment *t, Event *e)
{
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
    Q_ASSERT(t == &m_segment);
    (void)t; // avoid warnings

    // If we have it, lose it

    ViewElementList::iterator i = findEvent(e);
    if (i != m_viewElementList->end()) {
        // Let the velocity ruler know.
        notifyRemove(*i);
        // Remove from the list.
        m_viewElementList->erase(i);
        return;
    }

//    std::cerr << "Event at " << e->getAbsoluteTime() << ", notation time " << e->getNotationAbsoluteTime() << ", type " << e->getType()
//              << " not found in ViewSegment" << std::endl;
}

void
ViewSegment::endMarkerTimeChanged(const Segment *constSegment, bool shorten)
{
    // Cast away const since findTime() is unfortunately not const.
    Segment *segment = const_cast<Segment *>(constSegment);

    // If this isn't our Segment, bail.
    if (segment != &m_segment) {
        RG_WARNING << "endMarkerTimeChanged(): Unexpected Segment.";
        return;
    }

    if (shorten) {

        const ViewElementList::const_iterator newEndIter =
            m_viewElementList->findTime(segment->getEndMarkerTime());

        // For each ViewElement from the new end to the old end, notify
        // observers that the ViewElement will be removed.
        for (ViewElementList::const_iterator i = newEndIter;
             i != m_viewElementList->end();
             ++i){
            notifyRemove(*i);
        }

        // Remove the ViewElement(s).
        m_viewElementList->erase(newEndIter, m_viewElementList->end());

    } else {  // Segment size is growing.

        // Compute the time of the last ViewElement.
        timeT lastElementTime = segment->getStartTime();
        if (!m_viewElementList->empty()) {
            ViewElementList::iterator i = m_viewElementList->end();
            lastElementTime = (*--i)->event()->getAbsoluteTime();
        }

        // For each Event in the Segment that has been newly uncovered by
        // the expansion of the Segment end time...
        for (Segment::iterator j = segment->findTime(lastElementTime);
             segment->isBeforeEndMarker(j);
             ++j) {

            // If there is no ViewElement for this Event...
            if (findEvent(*j) == m_viewElementList->end()) {
                // If this Event is before the end of the Segment...
                if (wrapEvent(*j)) {
                    // Create a new ViewElement and add.
                    ViewElement *newElement = makeViewElement(*j);
                    m_viewElementList->insert(newElement);
                    notifyAdd(newElement);
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
    // Pass on to observers.
    for (ObserverSet::const_iterator i = m_observers.begin();
         i != m_observers.end();
         ++i) {
        (*i)->elementAdded(this, e);
    }
}

void
ViewSegment::notifyRemove(ViewElement *e) const
{
    // Pass on to observers.
    for (ObserverSet::const_iterator i = m_observers.begin();
         i != m_observers.end();
         ++i) {
        (*i)->elementRemoved(this, e);
    }
}

void
ViewSegment::notifySourceDeletion() const
{
    // Pass on to observers.
    for (ObserverSet::const_iterator i = m_observers.begin();
         i != m_observers.end();
         ++i) {
        (*i)->viewSegmentDeleted(this);
    }
}


}
