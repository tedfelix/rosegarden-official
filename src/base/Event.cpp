/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <cstdio>
#include <cctype>
#include <iostream>
#include "Event.h"
#include "XmlExportable.h"
#include "NotationTypes.h"
#include "BaseProperties.h"

#include <sstream>

namespace Rosegarden 
{
using std::string;
using std::ostream;

PropertyName Event::EventData::NotationTime = "!notationtime";
PropertyName Event::EventData::NotationDuration = "!notationduration";


Event::EventData::EventData(const std::string &type, timeT absoluteTime,
			    timeT duration, short subOrdering) :
    m_refCount(1),
    m_type(type),
    m_absoluteTime(absoluteTime),
    m_duration(duration),
    m_subOrdering(subOrdering),
    m_properties(0)
{
    // empty
}

Event::EventData::EventData(const std::string &type, timeT absoluteTime,
			    timeT duration, short subOrdering,
			    const PropertyMap *properties) :
    m_refCount(1),
    m_type(type),
    m_absoluteTime(absoluteTime),
    m_duration(duration),
    m_subOrdering(subOrdering),
    m_properties(properties ? new PropertyMap(*properties) : 0)
{
    // empty
}

Event::EventData *Event::EventData::unshare()
{
    --m_refCount;

    EventData *newData = new EventData
	(m_type, m_absoluteTime, m_duration, m_subOrdering, m_properties);

    return newData;
}

Event::EventData::~EventData()
{
    if (m_properties) delete m_properties;
}

timeT
Event::EventData::getNotationTime() const
{
    if (!m_properties) return m_absoluteTime;
    PropertyMap::const_iterator i = m_properties->find(NotationTime);
    if (i == m_properties->end()) return m_absoluteTime;
    else return static_cast<PropertyStore<Int> *>(i->second)->getData();
}

timeT
Event::EventData::getNotationDuration() const
{
    if (!m_properties) return m_duration;
    PropertyMap::const_iterator i = m_properties->find(NotationDuration);
    if (i == m_properties->end()) return m_duration;
    else return static_cast<PropertyStore<Int> *>(i->second)->getData();
}

timeT
Event::getGreaterDuration()
{
    if (isa(Note::EventType)) {
        return std::max(getDuration(), getNotationDuration());
    }
    return getDuration();
}

void
Event::EventData::setTime(const PropertyName &name, timeT t, timeT deft)
{
    if (!m_properties) m_properties = new PropertyMap();
    PropertyMap::iterator i = m_properties->find(name);

    if (t != deft) {
	if (i == m_properties->end()) {
	    m_properties->insert(PropertyPair(name, new PropertyStore<Int>(t)));
	} else {
	    static_cast<PropertyStore<Int> *>(i->second)->setData(t);
	}
    } else if (i != m_properties->end()) {
	delete i->second;
	m_properties->erase(i);
    }
}

PropertyMap *
Event::find(const PropertyName &name, PropertyMap::iterator &i)
{
    PropertyMap *map = m_data->m_properties;

    if (!map || ((i = map->find(name)) == map->end())) {

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
        throw NoData(name.getName(), __FILE__, __LINE__);
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
        throw NoData(name.getName(), __FILE__, __LINE__);
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
        throw NoData(name.getName(), __FILE__, __LINE__);
    }
}

// We could derive from XmlExportable and make this a virtual method
// overriding XmlExportable's pure virtual.  We don't, because this
// class has no other virtual methods and for such a core class we
// could do without the overhead (given that it wouldn't really gain
// us anything anyway).

string
Event::toXmlString() const
{
    return toXmlString(0);
}

string
Event::toXmlString(timeT expectedTime) const
{
    std::stringstream out;

    out << "<event";
    
    if (getType().length() != 0) {
	out << " type=\"" << getType() << "\"";
    }

    // Check for zero note durations and fix it (fixing in setters and
    // constructors is problematic since -1 durations are used in recording
    // and many events are indeed 0 duration events.
    timeT duration = getDuration();
    
    if (isa(Note::EventType) &&
        duration < 1 &&
        !has(BaseProperties::IS_GRACE_NOTE)) {

        duration = 1;
    }
    
    if (duration != 0) {
	out << " duration=\"" << duration << "\"";
    }

    if (getSubOrdering() != 0) {
	out << " subordering=\"" << getSubOrdering() << "\"";
    }

    if (expectedTime == 0) {
	out << " absoluteTime=\"" << getAbsoluteTime() << "\"";
    } else if (getAbsoluteTime() != expectedTime) {
	out << " timeOffset=\"" << (getAbsoluteTime() - expectedTime) << "\"";
    }

    out << ">";

    // Save all persistent properties as <property> elements

    PropertyNames propertyNames(getPersistentPropertyNames());
    for (PropertyNames::const_iterator i = propertyNames.begin();
         i != propertyNames.end(); ++i) {

    out << "<property name=\""
	    << XmlExportable::encode(i->getName()) << "\" ";
	string type = getPropertyTypeAsString(*i);
	for (size_t j = 0; j < type.size(); ++j) {
	    type[j] = (isupper(type[j]) ? tolower(type[j]) : type[j]);
	}

	out << type << "=\""
	    << XmlExportable::encode(getAsString(*i))
	    << "\"/>";
    }

    // Save non-persistent properties (the persistence applies to
    // copying events, not load/save) as <nproperty> elements
    // unless they're view-local.  View-local properties are
    // assumed to have "::" in their name somewhere.

    propertyNames = getNonPersistentPropertyNames();
    for (PropertyNames::const_iterator i = propertyNames.begin();
         i != propertyNames.end(); ++i) {

	std::string s(i->getName());
	if (s.find("::") != std::string::npos) continue;

    out << "<nproperty name=\""
	    << XmlExportable::encode(s) << "\" ";
	string type = getPropertyTypeAsString(*i);
	for (size_t j = 0; j < type.size(); ++j) {
	    type[j] = (isupper(type[j]) ? tolower(type[j]) : type[j]);
	}
	out << type << "=\""
	    << XmlExportable::encode(getAsString(*i))
	    << "\"/>";
    }
  
    out << "</event>";

    return out.str();
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

    if (m_data->m_properties) {
	for (PropertyMap::const_iterator i = m_data->m_properties->begin();
	     i != m_data->m_properties->end(); ++i) {
	    out << "\t\t" << i->first.getName() << " [" << i->first.getValue() << "] \t" << *(i->second) << "\n";
	}
    }

    if (m_nonPersistentProperties) {
	out << "\n\tNon-persistent properties : \n";

	for (PropertyMap::const_iterator i = m_nonPersistentProperties->begin();
	     i != m_nonPersistentProperties->end(); ++i) {
	    out << "\t\t" << i->first.getName() << " [" << i->first.getValue() << "] \t" << *(i->second) << '\n';
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

bool
Event::maskedInTrigger(void) const
{
    using namespace BaseProperties;
    
    if (!has(TRIGGER_EXPAND)) { return false; }
    return !get<Bool>(TRIGGER_EXPAND);
}

Event::PropertyNames
Event::getPropertyNames() const
{
    PropertyNames v;
    if (m_data->m_properties) {
	for (PropertyMap::const_iterator i = m_data->m_properties->begin();
	     i != m_data->m_properties->end(); ++i) {
	    v.push_back(i->first);
	}
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
    if (m_data->m_properties) {
	for (PropertyMap::const_iterator i = m_data->m_properties->begin();
	     i != m_data->m_properties->end(); ++i) {
	    v.push_back(i->first);
	}
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

void
Event::clearNonPersistentProperties()
{
    if (m_nonPersistentProperties) m_nonPersistentProperties->clear();
}

void
Event::unsafeChangeTime(timeT offset)
{
    // Get the values first, because notation time will sometimes use
    // m_absoluteTime, sometimes not.
    const timeT oldTime = getAbsoluteTime();
    const timeT oldNotationTime = getNotationAbsoluteTime();
    setAbsoluteTime(oldTime + offset);
    setNotationAbsoluteTime(oldNotationTime + offset);
}

size_t
Event::getStorageSize() const
{
    size_t s = sizeof(Event) + sizeof(EventData) + m_data->m_type.size();
    if (m_data->m_properties) {
	for (PropertyMap::const_iterator i = m_data->m_properties->begin();
	     i != m_data->m_properties->end(); ++i) {
	    s += sizeof(i->first);
	    s += i->second->getStorageSize();
	}
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
