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
 * Staff is the base class for classes which represent a Track as an
 * on-screen graphic.  It manages the relationship between Track/Event
 * and specific implementations of ViewElement.
 *
 * The template argument T must be a subclass of ViewElement.
 *
 * Staff was formerly known as ViewElementsManager.
 */

template <class T>
class Staff : public TrackObserver
{
public: 
    virtual ~Staff();

    /**
     * Create a new ViewElementList wrapping all Events in the
     * track, or return the previously created one
     */
    ViewElementList<T> *getViewElementList();

    /**
     * Create a new ViewElementList wrapping Events in the
     * [from, to[ interval, or return the previously created one
     * (even if passed new arguments)
     */
    ViewElementList<T> *getViewElementList(Track::iterator from,
					   Track::iterator to);

    /**
     * Wrap Event in a ViewElement if it doesn't have one already, and
     * insert it in a ViewElements list.
     *
     * If insertInTrack is true, insert the Event itself in the
     * wrapped Track as well.  In this case behaviour is equivalent to
     * simply inserting in the track instead of calling this method,
     * so you should usually only use this method if you don't want
     * the event to appear in the underlying track.
     */
    void insert(Event *, bool insertInTrack = false);

    /**
     * Erase the element pointed to by iterator.  If eraseFromTrack is
     * true, erase the Event from the wrapped Track as well.  In this
     * case behaviour is equivalent to simply erasing from the track
     * instead of calling this method, so you should usually only use
     * this method if you know that the event is not in the Track.
     */
    void erase(ViewElementList<T>::iterator, bool eraseFromTrack = false);

    /**
     * Return the Track wrapped by this object 
     */
    Track &getTrack() { return m_track; }

    /**
     * Return the Track wrapped by this object 
     */
    const Track &getTrack() const { return m_track; }

    /**
     * TrackObserver method - called after the event has been added to
     * the track
     */
    virtual void eventAdded(const Track *, Event *);

    /**
     * TrackObserver method - called after the event has been removed
     * from the track, and just before it is deleted
     */
    virtual void eventRemoved(const Track *, Event *);

protected:
    Staff(Track &);

    Track &m_track;
    ViewElementList<T> *m_viewElementList;
    ViewElementList<T>::iterator findEvent(Rosegarden::Event *);
};



template <class T>
Staff<T>::Staff(Track &t) :
    m_track(t),
    m_viewElementList(0)
{
    // empty
}

template <class T>
Staff<T>::~Staff()
{
    if (m_viewElementList) m_track.removeObserver(this);
}

template <class T>
ViewElementList<T> *
Staff<T>::getViewElementList()
{
    return getViewElementList(m_track.begin(), m_track.end());
}

template <class T>
ViewElementList<T> *
Staff<T>::getViewElementList(Track::iterator from,
			     Track::iterator to)
{
    if (m_viewElementList) return m_viewElementList;

    m_viewElementList = new ViewElementList<T>;

    for (Track::iterator i = from; i != to; ++i) {
        T *el = new T(*i);
        m_viewElementList->insert(el);
    }

    m_track.addObserver(this);
    return m_viewElementList;
}

template <class T>
void
Staff<T>::insert(Event *e, bool insertInTrack)
{
    if (insertInTrack) {
        m_track.insert(e);
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
				   bool eraseFromTrack)
{
    if (eraseFromTrack) {
        m_track.eraseSingle((*it)->event());
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
Staff<T>::eventAdded(const Track *t, Event *e)
{
    assert(t == &m_track);

    // If it isn't already wrapped, wrap it.  The already-wrapped test
    // is rather slow, and if all goes according to plan we should
    // eventually be able to lose it.

    //!!! [Note to self: Lose this eventually]

    if (findEvent(e) == m_viewElementList->end()) {
        T *el = new T(e);
        m_viewElementList->insert(el);
    }
}

template <class T>
void
Staff<T>::eventRemoved(const Track *t, Event *e)
{
    assert(t == &m_track);

    // If we have it, lose it

    ViewElementList<T>::iterator i = findEvent(e);
    if (i != m_viewElementList->end()) {
        m_viewElementList->erase(i);
        return;
    }
}

}

#endif

