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
#include <iostream>
#include "Event.h"

namespace Rosegarden 
{
using std::string;
using std::ostream;

Event::Event()
    : m_duration(0),
      m_absoluteTime(0),
      m_viewElementRefCount(0)
{
}

Event::Event(const std::string &type)
    : m_type(type),
      m_duration(0),
      m_absoluteTime(0),
      m_viewElementRefCount(0)
{
}


Event::Event(const Event &e) :
    m_type(e.getType()),
    m_duration(e.getDuration()),
    m_absoluteTime(e.getAbsoluteTime()),
    m_viewElementRefCount(0) // because we do not copy the ViewElements,
			     // since we can't access them
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
Event::has(const PropertyName &name) const
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
        m_properties.insert(PropertyPair(i->first, i->second->clone()));
    }
}

void
Event::unset(const PropertyName &name)
{
    m_properties.erase(name);
}
    

string
Event::getPropertyType(const PropertyName &name) const
    // throw (NoData)
{
    PropertyMap::const_iterator i = m_properties.find(name);
    if (i != m_properties.end()) {
        return i->second->getTypeName();
    } else {
        throw NoData();
    }
}
   

string
Event::getAsString(const PropertyName &name) const
    // throw (NoData)
{
    PropertyMap::const_iterator i = m_properties.find(name);
    if (i != m_properties.end()) {
        return i->second->unparse();
    } else {
        throw NoData();
    }
}

#ifndef NDEBUG
void
Event::dump(ostream& out) const
{
    out << "Event type : " << m_type.c_str() << '\n';

    out << "\tDuration : " << m_duration
        << "\n\tAbsolute Time : " << m_absoluteTime
        << "\n\tProperties : \n";

    for (PropertyMap::const_iterator i = m_properties.begin();
         i != m_properties.end(); ++i) {
        out << "\t\t" << i->first << '\t'
            << *(i->second)
            << (i->second->isPersistent() ?
                "\t(persistent)" : "\t(not persistent)")
            << '\n';
    }

    out << "Event storage size : " << getStorageSize() << '\n';
}
#endif

Event::PropertyNames
Event::getPropertyNames() const
{
    PropertyNames v;
    for (PropertyMap::const_iterator i = m_properties.begin();
         i != m_properties.end(); ++i) {
        v.push_back(i->first);
    }
    return v;
}

Event::PropertyNames
Event::getPersistentPropertyNames() const
{
    PropertyNames v;
    for (PropertyMap::const_iterator i = m_properties.begin();
         i != m_properties.end(); ++i) {
        if (i->second->isPersistent()) v.push_back(i->first);
    }
    return v;
}

Event::PropertyNames
Event::getNonPersistentPropertyNames() const
{
    PropertyNames v;
    for (PropertyMap::const_iterator i = m_properties.begin();
         i != m_properties.end(); ++i) {
        if (!i->second->isPersistent()) v.push_back(i->first);
    }
    return v;
}

size_t
Event::getStorageSize() const
{
    size_t s = sizeof(*this) + m_type.size();
    for (PropertyMap::const_iterator i = m_properties.begin();
         i != m_properties.end(); ++i) {
        s += sizeof(i->first);
        s += i->second->getStorageSize();
    }
    return s;
}

bool
operator<(const Event &a, const Event &b)
{
    return a.getAbsoluteTime() < b.getAbsoluteTime();
}
 
}
