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

#include <cstdio>
#include <iostream>
#include "Event.h"

namespace Rosegarden 
{
using std::string;
using std::ostream;

Event::EventData::EventData() :
    m_refCount(1),
    m_duration(0),
    m_absoluteTime(0),
    m_subOrdering(0)
{
    // empty
}

Event::EventData::EventData(const std::string &type) :
    m_refCount(1),
    m_type(type),
    m_duration(0),
    m_absoluteTime(0),
    m_subOrdering(0)
{
    // empty
}

Event::EventData *Event::EventData::unshare()
{
    --m_refCount;

    EventData *newData = new EventData(m_type); 
    newData->m_duration = m_duration;
    newData->m_absoluteTime = m_absoluteTime;
    newData->m_subOrdering = m_subOrdering;

    for (PropertyMap::const_iterator i = m_properties.begin();
         i != m_properties.end(); ++i) {
        newData->m_properties.insert
	    (EventData::PropertyPair(i->first, i->second->clone()));
    }

    return newData;
}

Event::EventData::~EventData()
{
    for (PropertyMap::iterator i = m_properties.begin();
         i != m_properties.end(); ++i) {
        delete i->second;
    }
}

bool
Event::has(const PropertyName &name) const
{
    EventData::PropertyMap::const_iterator i = m_data->m_properties.find(name);
    return (i != m_data->m_properties.end());
}

void
Event::unset(const PropertyName &name)
{
    m_data->m_properties.erase(name);
}
    

string
Event::getPropertyType(const PropertyName &name) const
    // throw (NoData)
{
    EventData::PropertyMap::const_iterator i = m_data->m_properties.find(name);
    if (i != m_data->m_properties.end()) {
        return i->second->getTypeName();
    } else {
        throw NoData();
    }
}
   

string
Event::getAsString(const PropertyName &name) const
    // throw (NoData)
{
    EventData::PropertyMap::const_iterator i = m_data->m_properties.find(name);
    if (i != m_data->m_properties.end()) {
        return i->second->unparse();
    } else {
        throw NoData();
    }
}

#ifndef NDEBUG
void
Event::dump(ostream& out) const
{
    out << "Event type : " << m_data->m_type.c_str() << '\n';

    out << "\tDuration : " << m_data->m_duration
        << "\n\tAbsolute Time : " << m_data->m_absoluteTime
        << "\n\tSub-ordering : " << m_data->m_subOrdering
        << "\n\tProperties : \n";

    for (EventData::PropertyMap::const_iterator i = m_data->m_properties.begin();
         i != m_data->m_properties.end(); ++i) {
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
    for (EventData::PropertyMap::const_iterator i = m_data->m_properties.begin();
         i != m_data->m_properties.end(); ++i) {
        v.push_back(i->first);
    }
    return v;
}

Event::PropertyNames
Event::getPersistentPropertyNames() const
{
    PropertyNames v;
    for (EventData::PropertyMap::const_iterator i = m_data->m_properties.begin();
         i != m_data->m_properties.end(); ++i) {
        if (i->second->isPersistent()) v.push_back(i->first);
    }
    return v;
}

Event::PropertyNames
Event::getNonPersistentPropertyNames() const
{
    PropertyNames v;
    for (EventData::PropertyMap::const_iterator i = m_data->m_properties.begin();
         i != m_data->m_properties.end(); ++i) {
        if (!i->second->isPersistent()) v.push_back(i->first);
    }
    return v;
}

size_t
Event::getStorageSize() const
{
    size_t s = sizeof(*this) + m_data->m_type.size();
    for (EventData::PropertyMap::const_iterator i = m_data->m_properties.begin();
         i != m_data->m_properties.end(); ++i) {
        s += sizeof(i->first);
        s += i->second->getStorageSize();
    }
    return s;
}

bool
operator<(const Event &a, const Event &b)
{
    timeT at = a.getAbsoluteTime();
    timeT bt = b.getAbsoluteTime();
    if (at != bt) return at < bt;
    else return a.getSubOrdering() < b.getSubOrdering();
}
 
}
