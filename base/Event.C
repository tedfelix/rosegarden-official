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

#include <cstdio>
#include "Event.h"

Event::Event()
    : m_duration(0),
      m_absoluteTime(0)
{
}

Event::Event(const string &package, const string &type)
    : m_package(package),
      m_type(type),
      m_duration(0),
      m_absoluteTime(0)
{
}

Event::Event(const Event &e)
    : m_package(e.package()),
      m_type(e.type()),
      m_duration(e.getDuration()),
      m_absoluteTime(e.getAbsoluteTime())
{
    copyFrom(e);
}

Event::~Event()
{
    scrapMap();
}

Event&
Event::operator=(const Event &e)
{
    if (&e != this) {
        copyFrom(e);
    }
    return *this;
}    

bool
Event::has(const string &name) const
{
    PropertyMap::const_iterator i = m_properties.find(name);
    return (i != m_properties.end());
}

void
Event::scrapMap()
{
    for (PropertyMap::iterator i = m_properties.begin();
         i != m_properties.end(); ++i) {
        delete (*i).second;
    }
}

void
Event::copyFrom(const Event &e)
{
    scrapMap();

    for (PropertyMap::const_iterator i = e.m_properties.begin();
         i != e.m_properties.end(); ++i) {
        m_properties.insert(PropertyPair((*i).first, (*i).second->clone()));
    }
}


void
Event::dump(ostream& out) const
{
#ifndef NDEBUG
    if (m_package.length()) {
        out << "Event type : " << m_type << " - package : "
            << m_package << '\n';
    } else {    
        out << "Event type : " << m_type.c_str() << '\n';
    }

    out << "\tDuration : " << m_duration
        << "\n\tAbsolute Time : " << m_absoluteTime
        << "\n\tProperties : \n";

    for (PropertyMap::const_iterator i = m_properties.begin();
         i != m_properties.end(); ++i) {
        out << "\t\t" << (*i).first << '\t'
            << *((*i).second) << '\n';
    }
#endif
}

bool
operator<(const Event &a, const Event &b)
{
    return a.getAbsoluteTime() < b.getAbsoluteTime();
}
