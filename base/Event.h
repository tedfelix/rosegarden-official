
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

#ifndef _EVENT_H_
#define _EVENT_H_

#include "Property.h"

#if (__GNUC__ < 3)
#include <hash_map>
#else
#include <ext/hash_map>
#endif

#include <string>
#ifndef NDEBUG
#include <iostream>
#endif

struct eqstring
{
    bool operator()(const std::string &s1, const std::string &s2) const {
        return s1 == s2;
    }
};

struct hashstring
{
    static std::hash<const char*> _H;
    size_t operator()(const std::string &s) const {
        return _H(s.c_str());
    }
};

std::hash<const char*> hashstring::_H;

namespace Rosegarden 
{
    
class Event
{
private:
    
public:
    typedef int timeT;

    struct NoData { };
    struct BadType { };

    Event() :
        m_duration(0), m_absoluteTime(0) { }
    Event(const std::string &type) :
        m_type(type), m_duration(0), m_absoluteTime(0) { }
    Event(const Event &e);

    virtual ~Event();

    Event &operator=(const Event &e);
    friend bool operator<(const Event&, const Event&);

    // Accessors
    const std::string &getType() const    { return m_type; }
    void setType(const std::string &t)    { m_type = t; }

    bool isa(const std::string &t) const  { return (m_type == t); }

    timeT getAbsoluteTime() const    { return m_absoluteTime; }
    timeT getDuration()     const    { return m_duration; }
    void setAbsoluteTime(timeT d)    { m_absoluteTime = d; }
    void addAbsoluteTime(timeT d)    { m_absoluteTime += d; }
    void setDuration(timeT d)        { m_duration = d; }

    bool has(const std::string &name) const;

    template <PropertyType P>
    PropertyDefn<P>::basic_type get(const std::string &name) const;
//         throw (NoData, BadType);

    // no throw, returns bool
    template <PropertyType P>
    bool get(const std::string &name, PropertyDefn<P>::basic_type &val) const;

    template <PropertyType P>
    bool isPersistent(const std::string &name) const;
//         throw (NoData);

    template <PropertyType P>
    void setPersistence(const std::string &name, bool persistent);
//         throw (NoData);

    std::string getPropertyType(const std::string &name) const;
// 	throw (NoData);
    std::string getAsString(const std::string &name) const;
// 	throw (NoData);

    template <PropertyType P>
    void set(const std::string &name, PropertyDefn<P>::basic_type value,
             bool persistent = true);
// 	throw (BadType);

    // set non-persistent, but only if there's no persistent value already
    template <PropertyType P>
    void setMaybe(const std::string &name, PropertyDefn<P>::basic_type value);
//         throw (BadType);

    template <PropertyType P>
    void setFromString(const std::string &name, std::string value,
                       bool persistent = true);
// 	throw (BadType);
    
    typedef std::vector<std::string> PropertyNames;
    PropertyNames getPropertyNames() const;
    PropertyNames getPersistentPropertyNames() const;
    PropertyNames getNonPersistentPropertyNames() const;

    struct EventCmp
    {
        bool operator()(const Event *e1, const Event *e2) const
        {
            return *e1 < *e2;
        }
    };

    static bool compareEvent2Time(const Event *e, Event::timeT t)
    {
        return e->getAbsoluteTime() < t;
    }

    static bool compareTime2Event(Event::timeT t, const Event *e)
    {
        return t <  e->getAbsoluteTime();
    }

    size_t getStorageSize() const; // for debugging and inspection purposes

#ifndef NDEBUG
    void dump(std::ostream&) const;
#else
    void dump(std::ostream&) const {}
#endif

private:
    void scrapMap();
    void copyFrom(const Event &e);

    std::string m_type;
    timeT m_duration;
    timeT m_absoluteTime;

    typedef std::hash_map<std::string, PropertyStoreBase*, hashstring, eqstring>
        PropertyMap;
    typedef PropertyMap::value_type PropertyPair;
    PropertyMap m_properties;
};


template <PropertyType P>
bool
Event::get(const std::string &name, PropertyDefn<P>::basic_type &val) const
{
    PropertyMap::const_iterator i = m_properties.find(name);
    if (i != m_properties.end()) { 

        PropertyStoreBase *sb = i->second;
        if (sb->getType() == P) {
            val = ((PropertyStore<P> *)sb)->getData();
            return true;
        }
        else {
#ifndef NDEBUG
            std::cerr << "Event::get() Error: Attempt to get property \"" << name
                 << "\" as " << PropertyDefn<P>::name() <<", actual type is "
                 << sb->getTypeName() << std::endl;
#endif
            return false;
        }
	    
    } else {

// No, this is not an error -- this sort of thing is what the method is for

#ifdef NOT_DEFINED

#ifndef NDEBUG
        std::cerr << "Event::get() Error: Attempt to get property \"" << name
             << "\" which doesn't exist for this element" << std::endl;
#endif

#endif

        return false;
    }
}


template <PropertyType P>
PropertyDefn<P>::basic_type
Event::get(const std::string &name) const
//     throw (NoData, BadType)
{
    PropertyMap::const_iterator i = m_properties.find(name);
    if (i != m_properties.end()) { 

        PropertyStoreBase *sb = i->second;
        if (sb->getType() == P) return ((PropertyStore<P> *)sb)->getData();
        else {
#ifndef NDEBUG
            std::cerr << "Event::get() Error: Attempt to get property \"" << name
                 << "\" as " << PropertyDefn<P>::name() <<", actual type is "
                 << sb->getTypeName() << std::endl;
#endif
            throw BadType();
        }
	    
    } else {
#ifndef NDEBUG
        std::cerr << "Event::get() Error: Attempt to get property \"" << name
             << "\" which doesn't exist for this element" << std::endl;
#endif
        throw NoData();
    }
}


template <PropertyType P>
bool
Event::isPersistent(const std::string &name) const
//     throw (NoData)
{
    PropertyMap::const_iterator i = m_properties.find(name);
    if (i != m_properties.end()) return i->second->isPersistent();
    else {
#ifndef NDEBUG
        std::cerr << "Event::get() Error: Attempt to get persistence of property \""
             << name << "\" which doesn't exist for this element" << std::endl;
#endif
        throw NoData();
    }
}


template <PropertyType P>
void
Event::setPersistence(const std::string &name, bool persistent)
//     throw (NoData)
{
    PropertyMap::const_iterator i = m_properties.find(name);
    if (i != m_properties.end()) i->second->setPersistence(persistent);
    else {
#ifndef NDEBUG
        std::cerr << "Event::get() Error: Attempt to set persistence of property \""
             << name << "\" which doesn't exist for this element" << std::endl;
#endif
        throw NoData();
    }
}


template <PropertyType P>
void
Event::set(const std::string &name, PropertyDefn<P>::basic_type value,
           bool persistent)
//     throw (BadType)
{
    PropertyMap::const_iterator i = m_properties.find(name);
    if (i != m_properties.end()) {

        PropertyStoreBase *sb = i->second;
        if (sb->getType() == P) {
            ((PropertyStore<P> *)sb)->setData(value);
            sb->setPersistence(persistent);
        } else {
#ifndef NDEBUG
            std::cerr << "Error: Element: Attempt to set property \"" << name
                 << "\" as " << PropertyDefn<P>::name() <<", actual type is "
                 << sb->getTypeName() << std::endl;
#endif
            throw BadType();
        }
	    
    } else {
        PropertyStoreBase *p = new PropertyStore<P>(value, persistent);
        m_properties.insert(PropertyPair(name, p));
    }
}


template <PropertyType P>
void
Event::setMaybe(const std::string &name, PropertyDefn<P>::basic_type value)
//     throw (BadType)
{
    // no need to catch NoData from isPersistent, as the has() check
    // should mean it never happens
    if (has(name) && isPersistent<P>(name)) return;
    set<P>(name, value, false);
}


template <PropertyType P>
void
Event::setFromString(const std::string &name, std::string value, bool persistent)
//     throw (BadType)
{
    set<P>(name, PropertyDefn<P>::parse(value), persistent);
}


//////////////////////////////////////////////////////////////////////

}

#endif
