// -*- c-basic-offset: 4 -*-


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

#include "PropertyName.h"


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
    throw CorruptedValue(m_value);
}

}

