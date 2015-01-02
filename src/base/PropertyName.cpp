/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */


/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <iostream>
#include <string>

#include "base/PropertyName.h"
#include "base/Exception.h"

#include <QtGlobal>

namespace Rosegarden 
{
using std::string;

PropertyName::intern_map *PropertyName::m_interns = 0;
PropertyName::intern_reverse_map *PropertyName::m_internsReversed = 0;
int PropertyName::m_nextValue = 0;

int PropertyName::intern(const string &s)
{
    if (!m_interns) {
        m_interns = new intern_map;
        m_internsReversed = new intern_reverse_map;
    }

    intern_map::iterator i(m_interns->find(s));
    
    if (i != m_interns->end()) {
        return i->second;
    } else {
        int nv = ++m_nextValue;
        m_interns->insert(intern_pair(s, nv));
        m_internsReversed->insert(intern_reverse_pair(nv, s));
        return nv;
    }
}

string PropertyName::getName() const
{
    intern_reverse_map::iterator i(m_internsReversed->find(m_value));
    if (i != m_internsReversed->end()) return i->second;

    // dump some informative data, even if we aren't in debug mode,
    // because this really shouldn't be happening
    std::cerr << "ERROR: PropertyName::getName: value corrupted!\n";
    std::cerr << "PropertyName's internal value is " << m_value << std::endl;
    std::cerr << "Reverse interns are ";
    i = m_internsReversed->begin();
    if (i == m_internsReversed->end()) std::cerr << "(none)";
    else while (i != m_internsReversed->end()) {
	if (i != m_internsReversed->begin()) {
	    std::cerr << ", ";
	}
	std::cerr << i->first << "=" << i->second;
	++i;
    }
    std::cerr << std::endl;

    Q_ASSERT(0); // exit if debug is on
    throw Exception
	("Serious problem in PropertyName::getName(): property "
	 "name's internal value is corrupted -- see stderr for details");
}

const PropertyName PropertyName::EmptyPropertyName = "";

}

