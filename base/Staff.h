// -*- c-basic-offset: 4 -*-

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

#ifndef _STAFF_H_
#define _STAFF_H_

#include "ViewElement.h"
#include "Track.h"

namespace Rosegarden 
{

/**
 * Staff is the base class for classes which represent a Segment as an
 * on-screen graphic.  It manages the relationship between Segment/Event
 * and specific implementations of ViewElement.
 *
 * The template argument T must be a subclass of ViewElement.
 *
 * Staff was formerly known as ViewElementsManager.
 */

template <class T>
class Staff : public SegmentObserver
{
public: 
    virtual ~Staff();

    /**
     * Create a new ViewElementList wrapping all Events in the
     * segment, or return the previously created one
     */
    ViewElementList<T> *getViewElementList();

    /**
     * Create a new ViewElementList wrapping Events in the
     * [from, to[ interval, or return the previously created one
     * (even if passed new arguments)
     */
    ViewElementList<T> *getViewElementList(Segment::iterator from,
					   Segment::iterator to);

    /**
     * Wrap Event in a ViewElement if it doesn't have one already, and
     * insert it in a ViewElements list.
     *
     * If insertInSegment is true, insert the Event itself in the
     * wrapped Segment as well.  In this case behaviour is equivalent to
     * simply inserting in the segment instead of calling this method,
     * so you should usually only use this method if you don't want
     * the event to appear in the underlying segment.
     */
    void insert(Event *, bool insertInSegment = false);

    /**
     * Erase the element pointed to by iterator.  If eraseFromSegment is
     * true, erase the Event from the wrapped Segment as well.  In this
     * case behaviour is equivalent to simply erasing from the segment
     * instead of calling this method, so you should usually only use
     * this method if you know that the event is not in the Segment.
     */
    void erase(ViewElementList<T>::iterator, bool eraseFromSegment = false);

    /**
     * Return the Segment wrapped by this object 
     */
    Segment &getSegment() { return m_segment; }

    /**
     * Return the Segment wrapped by this object 
     */
    const Segment &getSegment() const { return m_segment; }

    /**
     * SegmentObserver method - called after the event has been added to
     * the segment
     */
    virtual void eventAdded(const Segment *, Event *);

    /**
     * SegmentObserver method - called after the event has been removed
     * from the segment, and just before it is deleted
     */
    virtual void eventRemoved(const Segment *, Event *);

protected:
    Staff(Segment &);

    Segment &m_segment;
    ViewElementList<T> *m_viewElementList;
    ViewElementList<T>::iterator findEvent(Rosegarden::Event *);
};



template <class T>
Staff<T>::Staff(Segment &t) :
    m_segment(t),
    m_viewElementList(0)
{
    // empty
}

template <class T>
Staff<T>::~Staff()
{
    if (m_viewElementList) m_segment.removeObserver(this);
}

template <class T>
ViewElementList<T> *
Staff<T>::getViewElementList()
{
    return getViewElementList(m_segment.begin(), m_segment.end());
}

template <class T>
ViewElementList<T> *
Staff<T>::getViewElementList(Segment::iterator from,
			     Segment::iterator to)
{
    if (m_viewElementList) return m_viewElementList;

    m_viewElementList = new ViewElementList<T>;

    for (Segment::iterator i = from; i != to; ++i) {
        T *el = new T(*i);
        m_viewElementList->insert(el);
    }

    m_segment.addObserver(this);
    return m_viewElementList;
}

template <class T>
void
Staff<T>::insert(Event *e, bool insertInSegment)
{
    if (insertInSegment) {
        m_segment.insert(e);
        // and let the eventAdded callback do the wrapping
    } else {
	if (!e->hasViewElement()) {
	    T *el = new T(e);
	    m_viewElementList->insert(el);
	}
    }
}

template <class T>
void
Staff<T>::erase(ViewElementList<T>::iterator it,
				   bool eraseFromSegment)
{
    if (eraseFromSegment) {
        m_segment.eraseSingle((*it)->event());
        // and let the eventRemoved callback do the unwrapping
    } else {
        m_viewElementList->erase(it);
    }
}


template <class T>
ViewElementList<T>::iterator
Staff<T>::findEvent(Event *e)
{
    T dummy(e);

    std::pair<ViewElementList<T>::iterator,
	      ViewElementList<T>::iterator>
        r = m_viewElementList->equal_range(&dummy);

    for (ViewElementList<T>::iterator i = r.first; i != r.second; ++i) {
        if ((*i)->event() == e) {
            return i;
        }
    }

    return m_viewElementList->end();
}


template <class T>
void
Staff<T>::eventAdded(const Segment *t, Event *e)
{
    assert(t == &m_segment);

    // If it isn't already wrapped, wrap it.  The already-wrapped test
    // is rather slow, and if all goes according to plan we should
    // eventually be able to lose it.

    //!!! [Note to self: Lose this eventually]

//expt    if (findEvent(e) == m_viewElementList->end()) {
        T *el = new T(e);
        m_viewElementList->insert(el);
//expt    }
}

template <class T>
void
Staff<T>::eventRemoved(const Segment *t, Event *e)
{
    assert(t == &m_segment);

    // If we have it, lose it

    ViewElementList<T>::iterator i = findEvent(e);
    if (i != m_viewElementList->end()) {
        m_viewElementList->erase(i);
        return;
    }
}

}

#endif

