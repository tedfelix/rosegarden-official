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

#ifndef _VIEWELEMENT_H_
#define _VIEWELEMENT_H_


#include "Event.h"

#include <set>


namespace Rosegarden 
{

/**
 * The abstract base for classes which represent an Event as an
 * on-screen graphic item (a note, a rectangle on a piano roll).
 */

class ViewElement
{
public:
    virtual ~ViewElement();

    const Event* event() const { return m_event; }
    Event*       event()       { return m_event; }

    timeT getAbsoluteTime() const  { return event()->getAbsoluteTime(); }
    timeT getDuration() const      { return event()->getDuration();     }

    void dump(std::ostream&) const;

    friend bool operator<(const ViewElement&, const ViewElement&);

protected:
    ViewElement(Event *);

    Event *m_event;
};



template <class T>
class ViewElementComparator
{
public:
    bool operator()(const T *e1, const T *e2) const {
	const ViewElement &ve1 = *e1;
	const ViewElement &ve2 = *e2;
        return ve1 < ve2;
    }
};


/**
 * This class owns the objects its items are pointing at.
 *
 * The template argument T must be a subclass of ViewElement.
 */

template <class T>
class ViewElementList : public std::multiset<T *, ViewElementComparator<T> >
{
    typedef std::multiset<T *, ViewElementComparator<T> > set_type;
public:
    typedef typename set_type::iterator iterator;

    ViewElementList() : set_type() { }
    virtual ~ViewElementList();

    void erase(iterator i);
    void erase(iterator from, iterator to);
    void eraseSingle(T *);

    iterator findPrevious(const std::string &type, iterator i);
    iterator findNext(const std::string &type, iterator i);

    /**
     * Returns an iterator pointing to that specific element,
     * end() otherwise
     */
    iterator findSingle(T *);

    /**
     * Returns first iterator pointing at or after the given time,
     * end() if time is beyond the end of the list
     */ 
    iterator findTime(timeT time) const;

    /**
     * Returns iterator pointing to the first element starting at
     * or before the given absolute time
     */
    iterator findNearestTime(timeT time) const;
};


template <class T>
ViewElementList<T>::~ViewElementList()
{
    for (iterator i = begin(); i != end(); ++i) {
        delete (*i);
    }
}

template <class T>
void
ViewElementList<T>::erase(iterator pos)
{
    delete *pos;
    set_type::erase(pos);
}

template <class T>
void
ViewElementList<T>::erase(iterator from, iterator to)
{
    for (iterator i = from; i != to; ++i) delete *i;
    set_type::erase(from, to);
}

template <class T>
void
ViewElementList<T>::eraseSingle(T *el)
{
    iterator elPos = findSingle(el);
    if (elPos != end()) erase(elPos);
}

template <class T>
typename ViewElementList<T>::iterator
ViewElementList<T>::findPrevious(const std::string &type, iterator i)

{
    // what to return on failure? I think probably
    // end(), as begin() could be a success case
    if (i == begin()) return end();
    --i;
    for (;;) {
        if ((*i)->event()->isa(type)) return i;
        if (i == begin()) return end();
        --i;
    }
}

template <class T>
typename ViewElementList<T>::iterator
ViewElementList<T>::findNext(const std::string &type, iterator i)
{
    if (i == end()) return i;
    for (++i; i != end() && !(*i)->event()->isa(type); ++i);
    return i;
}

template <class T>
typename ViewElementList<T>::iterator
ViewElementList<T>::findSingle(T *el)
{
    iterator res = end();

    std::pair<iterator, iterator> interval = equal_range(el);
    
    for (iterator i = interval.first; i != interval.second; ++i) {
        if (*i == el) {
            res = i;
            break;
        }
    }

    return res;
}

template <class T>
typename ViewElementList<T>::iterator
ViewElementList<T>::findTime(timeT time) const
{
    Event dummy("dummy", time, 0, MIN_SUBORDERING);
    T dummyT(&dummy);
    return lower_bound(&dummyT);
}

template <class T>
ViewElementList<T>::iterator
ViewElementList<T>::findNearestTime(timeT t) const
{
    iterator i = findTime(t);
    if (i == end() || (*i)->getAbsoluteTime() > t) {
	if (i == begin()) return end();
	else --i;
    }
    return i;
}




}


#endif

