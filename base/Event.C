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

#include <cstdio>
#include <iostream>
#include "Event.h"

namespace Rosegarden 
{
using std::string;
using std::ostream;


Event::EventData::EventData(const std::string &type, timeT absoluteTime,
			    timeT duration, int subOrdering) :
    m_refCount(1),
    m_type(type),
    m_absoluteTime(absoluteTime),
    m_duration(duration),
    m_subOrdering(subOrdering)
{
    // empty
}

Event::EventData *Event::EventData::unshare()
{
    --m_refCount;

    EventData *newData = new EventData
	(m_type, m_absoluteTime, m_duration, m_subOrdering); 

    for (PropertyMap::const_iterator i = m_properties.begin();
         i != m_properties.end(); ++i) {
        newData->m_properties.insert
	    (PropertyPair(i->first, i->second->clone()));
    }

    return newData;
}

Event::EventData::~EventData()
{
    // nothing -- the PropertyStore objects are deleted in the PropertyMap dtor
}

Event::PropertyMap *
Event::find(const PropertyName &name, PropertyMap::iterator &i)
{
    PropertyMap *map = &m_data->m_properties;
    i = map->find(name);

    if (i == map->end()) {

	map = m_nonPersistentProperties;
	if (!map) return 0;

	i = map->find(name);
	if (i == map->end()) return 0;
    }

    return map;
}

bool
Event::has(const PropertyName &name) const
{
#ifndef NDEBUG
    ++m_hasCount;
#endif

    PropertyMap::const_iterator i;
    const PropertyMap *map = find(name, i);
    if (map) return true;
    else return false;
}

void
Event::unset(const PropertyName &name)
{
#ifndef NDEBUG
    ++m_unsetCount;
#endif

    unshare();
    PropertyMap::iterator i;
    PropertyMap *map = find(name, i);
    if (map) {
	delete i->second;
	map->erase(i);
    }
}
    

PropertyType
Event::getPropertyType(const PropertyName &name) const
    // throw (NoData)
{
    PropertyMap::const_iterator i;
    const PropertyMap *map = find(name, i);
    if (map) {
        return i->second->getType();
    } else {
        throw NoData();
    }
}
      

string
Event::getPropertyTypeAsString(const PropertyName &name) const
    // throw (NoData)
{
    PropertyMap::const_iterator i;
    const PropertyMap *map = find(name, i);
    if (map) {
        return i->second->getTypeName();
    } else {
        throw NoData();
    }
}
   

string
Event::getAsString(const PropertyName &name) const
    // throw (NoData)
{
    PropertyMap::const_iterator i;
    const PropertyMap *map = find(name, i);
    if (map) {
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

    out << "\tAbsolute Time : " << m_data->m_absoluteTime
	<< "\n\tDuration : " << m_data->m_duration
        << "\n\tSub-ordering : " << m_data->m_subOrdering
        << "\n\tPersistent properties : \n";

    for (PropertyMap::const_iterator i = m_data->m_properties.begin();
         i != m_data->m_properties.end(); ++i) {
        out << "\t\t" << i->first << '\t' << *(i->second) << '\n';
    }

    if (m_nonPersistentProperties) {
	out << "\n\tNon-persistent properties : \n";

	for (PropertyMap::const_iterator i = m_nonPersistentProperties->begin();
	     i != m_nonPersistentProperties->end(); ++i) {
	    out << "\t\t" << i->first << '\t' << *(i->second) << '\n';
	}
    }

    out << "Event storage size : " << getStorageSize() << '\n';
}


int Event::m_getCount = 0;
int Event::m_setCount = 0;
int Event::m_setMaybeCount = 0;
int Event::m_hasCount = 0;
int Event::m_unsetCount = 0;
clock_t Event::m_lastStats = clock();

void
Event::dumpStats(ostream& out)
{
    clock_t now = clock();
    int ms = (now - m_lastStats) * 1000 / CLOCKS_PER_SEC;
    out << "\nEvent stats, since start of run or last report ("
	<< ms << "ms ago):" << std::endl;

    out << "Calls to get<>: " << m_getCount << std::endl;
    out << "Calls to set<>: " << m_setCount << std::endl;
    out << "Calls to setMaybe<>: " << m_setMaybeCount << std::endl;
    out << "Calls to has: " << m_hasCount << std::endl;
    out << "Calls to unset: " << m_unsetCount << std::endl;

    m_getCount = m_setCount = m_setMaybeCount = m_hasCount = m_unsetCount = 0;
    m_lastStats = clock();
}

#else

void
Event::dumpStats(ostream&)
{
    // nothing
}

#endif

Event::PropertyNames
Event::getPropertyNames() const
{
    PropertyNames v;
    for (PropertyMap::const_iterator i = m_data->m_properties.begin();
         i != m_data->m_properties.end(); ++i) {
        v.push_back(i->first);
    }
    if (m_nonPersistentProperties) {
	for (PropertyMap::const_iterator i = m_nonPersistentProperties->begin();
	     i != m_nonPersistentProperties->end(); ++i) {
	    v.push_back(i->first);
	}
    }
    return v;
}

Event::PropertyNames
Event::getPersistentPropertyNames() const
{
    PropertyNames v;
    for (PropertyMap::const_iterator i = m_data->m_properties.begin();
         i != m_data->m_properties.end(); ++i) {
        v.push_back(i->first);
    }
    return v;
}

Event::PropertyNames
Event::getNonPersistentPropertyNames() const
{
    PropertyNames v;
    if (m_nonPersistentProperties) {
	for (PropertyMap::const_iterator i = m_nonPersistentProperties->begin();
	     i != m_nonPersistentProperties->end(); ++i) {
	    v.push_back(i->first);
	}
    }
    return v;
}

size_t
Event::getStorageSize() const
{
    size_t s = sizeof(*this) + m_data->m_type.size();
    for (PropertyMap::const_iterator i = m_data->m_properties.begin();
         i != m_data->m_properties.end(); ++i) {
        s += sizeof(i->first);
        s += i->second->getStorageSize();
    }
    if (m_nonPersistentProperties) {
	for (PropertyMap::const_iterator i = m_nonPersistentProperties->begin();
	     i != m_nonPersistentProperties->end(); ++i) {
	    s += sizeof(i->first);
	    s += i->second->getStorageSize();
	}
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
