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

#include "ViewElement.h"
#include <iostream>

namespace Rosegarden 
{

extern const int MIN_SUBORDERING;

ViewElement::ViewElement(Event *e) :
    m_layoutX(0.0),
    m_layoutY(0.0),
    m_event(e)
{
    // nothing
}

ViewElement::~ViewElement()
{
    // nothing
}

//////////////////////////////////////////////////////////////////////

bool
operator<(const ViewElement &a, const ViewElement &b)
{
    timeT at = a.getViewAbsoluteTime(), bt = b.getViewAbsoluteTime();
    if (at == bt) return *(a.event()) < *(b.event());
    else return (at < bt);
}

//////////////////////////////////////////////////////////////////////


ViewElementList::~ViewElementList()
{
    for (iterator i = begin(); i != end(); ++i) {
        delete (*i);
    }
}

void
ViewElementList::insert(ViewElement* el)
{
    set_type::insert(el);
    notifyAdd(el);
}

void
ViewElementList::erase(iterator pos)
{
    notifyRemove(*pos);

    delete *pos;
    set_type::erase(pos);
}

void
ViewElementList::erase(iterator from, iterator to)
{
    for (iterator i = from; i != to; ++i) {
        notifyRemove(*i);
        delete *i;
    }

    set_type::erase(from, to);
}

void
ViewElementList::eraseSingle(ViewElement *el)
{
    iterator elPos = findSingle(el);
    if (elPos != end()) erase(elPos);
}

ViewElementList::iterator
ViewElementList::findPrevious(const std::string &type, iterator i)

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

ViewElementList::iterator
ViewElementList::findNext(const std::string &type, iterator i)
{
    if (i == end()) return i;
    for (++i; i != end() && !(*i)->event()->isa(type); ++i);
    return i;
}

ViewElementList::iterator
ViewElementList::findSingle(ViewElement *el)
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

ViewElementList::iterator
ViewElementList::findTime(timeT time) const
{
    Event dummy("dummy", time, 0, MIN_SUBORDERING);
    ViewElement dummyT(&dummy);
    return lower_bound(&dummyT);
}

ViewElementList::iterator
ViewElementList::findNearestTime(timeT t) const
{
    iterator i = findTime(t);
    if (i == end() || (*i)->getViewAbsoluteTime() > t) {
	if (i == begin()) return end();
	else --i;
    }
    return i;
}

void
ViewElementList::notifyAdd(ViewElement *e) const
{
    for (ObserverSet::const_iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->elementAdded(e);
    }
}

void
ViewElementList::notifyRemove(ViewElement *e) const
{
    for (ObserverSet::const_iterator i = m_observers.begin();
	 i != m_observers.end(); ++i) {
	(*i)->elementRemoved(e);
    }
}



 
}

