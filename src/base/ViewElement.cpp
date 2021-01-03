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

#include "ViewElement.h"
#include <iostream>

#include <QtGlobal>

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
/*
    if (at < bt) {
	if (!(*(a.event()) < *(b.event()))) {
	    std::cerr << "    types: a: " << a.event()->getType() << " b: " << b.event()->getType() << std::endl;
	    std::cerr << "performed: a: " << a.event()->getAbsoluteTime() << " b: " << b.event()->getAbsoluteTime() << std::endl;
	    std::cerr << "  notated: a: " << a.getViewAbsoluteTime() << " b: " << b.getViewAbsoluteTime() << std::endl;
//	assert(*(a.event()) < *(b.event()));
	}
    }
    else if (at > bt) {
	if (*(a.event()) < *(b.event())) {
	    std::cerr << "    types: a: " << a.event()->getType() << " b: " << b.event()->getType() << std::endl;
	    std::cerr << "performed: a: " << a.event()->getAbsoluteTime() << " b: " << b.event()->getAbsoluteTime() << std::endl;
	    std::cerr << "  notated: a: " << a.getViewAbsoluteTime() << " b: " << b.getViewAbsoluteTime() << std::endl;
//	    assert(!(*(a.event()) < *(b.event())));
	}
    }
*/
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
}

void
ViewElementList::erase(iterator pos)
{
    delete *pos;
    set_type::erase(pos);
}

void
ViewElementList::erase(iterator from, iterator to)
{
    for (iterator i = from; i != to; ++i) {
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
    for (++i; i != end() && !(*i)->event()->isa(type); ++i){ };
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
ViewElementList::findTime(timeT time)
{
    Event dummy("dummy", time, 0, MIN_SUBORDERING);
    ViewElement dummyT(&dummy);
    return lower_bound(&dummyT);
}

ViewElementList::iterator
ViewElementList::findNearestTime(timeT t)
{
    iterator i = findTime(t);
    if (i == end() || (*i)->getViewAbsoluteTime() > t) {
	if (i == begin()) return end();
	else --i;
    }
    return i;
}
 
}

