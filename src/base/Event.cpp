/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Event.h"
#include "XmlExportable.h"
#include "MidiTypes.h"
#include "NotationTypes.h"
#include "BaseProperties.h"

#include "misc/Debug.h"
#include "base/figuration/GeneratedRegion.h"
#include "base/figuration/SegmentID.h"
#include "gui/editors/guitar/Chord.h"

#include <sstream>
#include <cstdio>
#include <cctype>
#include <iostream>

namespace Rosegarden
{
using std::string;
using std::ostream;

const PropertyName Event::NotationTime("!notationtime");
const PropertyName Event::NotationDuration("!notationduration");


Event::EventData::EventData(const std::string &type, timeT absoluteTime,
                            timeT duration, short subOrdering) :
    m_refCount(1),
    m_type(type),
    m_absoluteTime(absoluteTime),
    m_duration(duration),
    m_subOrdering(subOrdering),
    m_properties(nullptr)
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
    m_properties(properties ? new PropertyMap(*properties) : nullptr)
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
    if (m_properties) {
        delete m_properties;
        m_properties = nullptr;
    }
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
Event::getGreaterDuration() const
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
        if (!map) return nullptr;

        i = map->find(name);
        if (i == map->end()) return nullptr;
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

int Event::m_getCount = 0;
int Event::m_setCount = 0;
int Event::m_setMaybeCount = 0;
int Event::m_hasCount = 0;
int Event::m_unsetCount = 0;
clock_t Event::m_lastStats = clock();

void
Event::dumpStats(ostream &out)
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
Event::maskedInTrigger() const
{
    using namespace BaseProperties;

    if (!has(TRIGGER_EXPAND)) { return false; }
    return !get<Bool>(TRIGGER_EXPAND);
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
Event::isCopyOf(const Event &e) const
{
    if (&e == this)
        return true;

    if (e.m_data == m_data)
        return true;

    return false;
}

int Event::getSubOrdering(std::string eventType)
{
    // Missing:
    // - TimeSignature (private, -150)

    if (eventType == Note::EventType) {
        return 0;  // Should be Note::EventSubOrdering but there is none.
    } else if (eventType == Note::EventRestType) {
        return Note::EventRestSubOrdering;
    } else if (eventType == ProgramChange::EventType) {
        return ProgramChange::EventSubOrdering;
    } else if (eventType == Controller::EventType) {
        return Controller::EventSubOrdering;
    } else if (eventType == PitchBend::EventType) {
        return PitchBend::EventSubOrdering;
    } else if (eventType == ChannelPressure::EventType) {
        return ChannelPressure::EventSubOrdering;
    } else if (eventType == KeyPressure::EventType) {
        return KeyPressure::EventSubOrdering;
    } else if (eventType == RPN::EventType) {
        return RPN::EventSubOrdering;
    } else if (eventType == NRPN::EventType) {
        return NRPN::EventSubOrdering;
    } else if (eventType == Indication::EventType) {
        return Indication::EventSubOrdering;
    } else if (eventType == Clef::EventType) {
        return Clef::EventSubOrdering;
    } else if (eventType == Key::EventType) {
        return Key::EventSubOrdering;
    } else if (eventType == Text::EventType) {
        return Text::EventSubOrdering;
    } else if (eventType == SystemExclusive::EventType) {
        return SystemExclusive::EventSubOrdering;
    } else if (eventType == GeneratedRegion::EventType) {
        return GeneratedRegion::EventSubOrdering;
    } else if (eventType == SegmentID::EventType) {
        return SegmentID::EventSubOrdering;
    } else if (eventType == Symbol::EventType) {
        return Symbol::EventSubOrdering;
    } else if (eventType == Guitar::Chord::EventType) {
        return Guitar::Chord::EventSubOrdering;
    }

    return 0;
}


bool
operator<(const Event &a, const Event &b)
{
    timeT at = a.getAbsoluteTime();
    timeT bt = b.getAbsoluteTime();
    if (at != bt) return at < bt;
    else return a.getSubOrdering() < b.getSubOrdering();
}

QDebug operator<<(QDebug dbg, const Event &event)
{
    dbg << "Event type :" << event.m_data->m_type << "\n";
    dbg << "  Absolute Time :" << event.m_data->m_absoluteTime << "\n";
    dbg << "  Duration :" << event.m_data->m_duration << "\n";
    dbg << "  Sub-ordering :" << event.m_data->m_subOrdering << "\n";
    dbg << "  Persistent properties :\n";

    if (event.m_data->m_properties) {
        for (const PropertyMap::value_type &property :
                 *(event.m_data->m_properties)) {
            dbg << "    " << property.first.getName() << "[" <<
                   property.first.getId() << "] :" << *(property.second) <<
                   "\n";
        }
    }

    if (event.m_nonPersistentProperties) {
        dbg << "  Non-persistent properties :\n";
        for (const PropertyMap::value_type &property :
                 *(event.m_nonPersistentProperties)) {
            dbg << "    " << property.first.getName() << "[" <<
                   property.first.getId() << "] :" << *(property.second) <<
                   "\n";
        }
    }

    dbg << "  Approximate storage size :" << event.getStorageSize();

    return dbg;
}


}
