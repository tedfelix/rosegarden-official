// -*- c-basic-offset: 4 -*-


/*
    Rosegarden-4
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

#ifndef _EVENT_H_
#define _EVENT_H_

#include "PropertyMap.h"

#include <string>
#ifndef NDEBUG
#include <iostream>
#include <ctime>
#endif


namespace Rosegarden 
{

typedef long timeT;

/**
 * The Event class represents an event with some basic attributes and
 * an arbitrary number of properties of dynamically-determined name
 * and type.
 *
 * An Event has a type; a duration, often zero for events other than
 * notes; an absolute time, the time at which the event begins, which
 * is used to order events within a Segment; and a "sub-ordering", used
 * to determine an order for events that have the same absolute time
 * (for example to ensure that the clef always appears before the key
 * signature at the start of a piece).  Besides these, an event can
 * have any number of properties, which are typed values stored and
 * retrieved by name.  Properties may be persistent or non-persistent,
 * depending on whether they are saved to file with the rest of the
 * event data or are considered to be only cached values that can be
 * recomputed at will if necessary.
 */

class Event
{
public:
    struct NoData { };
    struct BadType { };

    Event(const std::string &type,
	  timeT absoluteTime, timeT duration = 0, int subOrdering = 0) :
	m_data(new EventData(type, absoluteTime, duration, subOrdering)),
	m_nonPersistentProperties(0) { }

    Event(const Event &e) :
	m_nonPersistentProperties(0) { share(e); }

    // next 3 ctors can't use default args: default has to be obtained from e

    Event(const Event &e, timeT absoluteTime) :
	m_nonPersistentProperties(0) {
	share(e);
	unshare();
	m_data->m_absoluteTime = absoluteTime;
    }

    Event(const Event &e, timeT absoluteTime, timeT duration) :
	m_nonPersistentProperties(0) {
	share(e);
	unshare();
	m_data->m_absoluteTime = absoluteTime;
	m_data->m_duration = duration;
    }

    Event(const Event &e, timeT absoluteTime, timeT duration, int subOrdering):
	m_nonPersistentProperties(0) {
	share(e);
	unshare();
	m_data->m_absoluteTime = absoluteTime;
	m_data->m_duration = duration;
	m_data->m_subOrdering = subOrdering;
    }

    ~Event() { lose(); }

    Event &operator=(const Event &e) {
	if (&e != this) { lose(); share(e); }
	return *this;
    }

    friend bool operator<(const Event&, const Event&);

    // Accessors
    const std::string &getType() const    { return  m_data->m_type; }
    bool  isa(const std::string &t) const { return (m_data->m_type == t); }
    timeT getAbsoluteTime() const    { return m_data->m_absoluteTime; }
    timeT getDuration()     const    { return m_data->m_duration; }
    int   getSubOrdering()  const    { return m_data->m_subOrdering; }

    bool  has(const PropertyName &name) const;

    template <PropertyType P>
    typename PropertyDefn<P>::basic_type get(const PropertyName &name) const;
    // throw (NoData, BadType);

    // no throw, returns bool
    template <PropertyType P>
    bool get(const PropertyName &name, typename PropertyDefn<P>::basic_type &val) const;

    template <PropertyType P>
    bool isPersistent(const PropertyName &name) const;
    // throw (NoData);

    template <PropertyType P>
    void setPersistence(const PropertyName &name, bool persistent);
    // throw (NoData);

    PropertyType getPropertyType(const PropertyName &name) const;
    // throw (NoData);

    std::string getPropertyTypeAsString(const PropertyName &name) const;
    // throw (NoData);

    std::string getAsString(const PropertyName &name) const;
    // throw (NoData);

    template <PropertyType P>
    void set(const PropertyName &name, typename PropertyDefn<P>::basic_type value,
             bool persistent = true);
    // throw (BadType);

    // set non-persistent, but only if there's no persistent value already
    template <PropertyType P>
    void setMaybe(const PropertyName &name, typename PropertyDefn<P>::basic_type value);
    // throw (BadType);

    template <PropertyType P>
    void setFromString(const PropertyName &name, std::string value,
                       bool persistent = true);
    // throw (BadType);

    void unset(const PropertyName &name);
    
    typedef std::vector<PropertyName> PropertyNames;
    PropertyNames getPropertyNames() const;
    PropertyNames getPersistentPropertyNames() const;
    PropertyNames getNonPersistentPropertyNames() const;

    struct EventCmp
    {
        bool operator()(const Event &e1, const Event &e2) const {
            return e1 < e2;
        }
        bool operator()(const Event *e1, const Event *e2) const {
            return *e1 < *e2;
        }
    };

    struct EventEndCmp
    {
        bool operator()(const Event &e1, const Event &e2) const {
            return e1.getAbsoluteTime() + e1.getDuration() <=
                e2.getAbsoluteTime() + e2.getDuration();
        }
        bool operator()(const Event *e1, const Event *e2) const {
            return e1->getAbsoluteTime() + e1->getDuration() <=
                e2->getAbsoluteTime() + e2->getDuration();
        }
    };

    static bool compareEvent2Time(const Event *e, timeT t) {
        return e->getAbsoluteTime() < t;
    }

    static bool compareTime2Event(timeT t, const Event *e) {
        return t <  e->getAbsoluteTime();
    }

    // approximate, for debugging and inspection purposes
    size_t getStorageSize() const;

#ifndef NDEBUG
    void dump(std::ostream&) const;
#else
    void dump(std::ostream&) const {}
#endif
    static void dumpStats(std::ostream&);

protected:
    // these are for subclasses such as XmlStorableEvent

    Event() :
	m_data(new EventData("", 0, 0, 0)),
	m_nonPersistentProperties(0) { }
    
    void setType(const std::string &t) { unshare(); m_data->m_type = t; }
    void setAbsoluteTime(timeT t)      { unshare(); m_data->m_absoluteTime = t; }
    void setDuration(timeT d)	       { unshare(); m_data->m_duration = d; }
    void setSubOrdering(int o)	       { unshare(); m_data->m_subOrdering = o; }

private:
    struct EventData // Data that are shared between shallow-copied instances
    {
	EventData(const std::string &type,
		  timeT absoluteTime, timeT duration, int subOrdering);
	EventData *unshare();
	~EventData();
	unsigned int m_refCount;

	std::string m_type;
	timeT m_absoluteTime;
	timeT m_duration;
	int m_subOrdering;

	PropertyMap m_properties;

    private:
	EventData(const EventData &);
	EventData &operator=(const EventData &);
    };	

    EventData *m_data;
    PropertyMap *m_nonPersistentProperties; // Unique to an instance

    void share(const Event &e) {
	m_data = e.m_data;
	m_data->m_refCount++;
    }

    bool unshare() { // returns true if unshare was necessary
	if (m_data->m_refCount > 1) {
	    m_data = m_data->unshare();
	    return true;
	} else {
	    return false;
	}
    }

    void lose() {
	if (--m_data->m_refCount == 0) delete m_data;
	delete m_nonPersistentProperties;
	m_nonPersistentProperties = 0;
    }

    // returned iterator (in i) only valid if return map value is non-zero
    PropertyMap *find(const PropertyName &name, PropertyMap::iterator &i);
    const PropertyMap *find(const PropertyName &name,
			    PropertyMap::const_iterator &i) const {
	PropertyMap::iterator j;
	PropertyMap *map = const_cast<Event *>(this)->find(name, j);
	i = j;
	return map;
    }

    PropertyMap::iterator insert(const PropertyPair &pair, bool persistent) {
	PropertyMap *map =
	    (persistent ? &m_data->m_properties : m_nonPersistentProperties);
	if (!map) map = m_nonPersistentProperties = new PropertyMap();
	return map->insert(pair).first;
    }

#ifndef NDEBUG
    static int m_getCount;
    static int m_setCount;
    static int m_setMaybeCount;
    static int m_hasCount;
    static int m_unsetCount;
    static clock_t m_lastStats;
#endif
};


template <PropertyType P>
bool
Event::get(const PropertyName &name, typename PropertyDefn<P>::basic_type &val) const
{
#ifndef NDEBUG
    ++m_getCount;
#endif

    PropertyMap::const_iterator i;
    const PropertyMap *map = find(name, i);

    if (map) { 

        PropertyStoreBase *sb = i->second;
        if (sb->getType() == P) {
            val = (static_cast<PropertyStore<P> *>(sb))->getData();
            return true;
        }
        else {
#ifndef NDEBUG
            std::cerr << "Event::get() Error: Attempt to get property \"" << name
                 << "\" as " << PropertyDefn<P>::typeName() <<", actual type is "
                 << sb->getTypeName() << std::endl;
#endif
            return false;
        }
	    
    } else {
        return false;
    }
}


template <PropertyType P>
typename PropertyDefn<P>::basic_type
Event::get(const PropertyName &name) const
    // throw (NoData, BadType)
{
#ifndef NDEBUG
    ++m_getCount;
#endif

    PropertyMap::const_iterator i;
    const PropertyMap *map = find(name, i);

    if (map) { 

        PropertyStoreBase *sb = i->second;
        if (sb->getType() == P)
            return (static_cast<PropertyStore<P> *>(sb))->getData();
        else {
            std::cerr << "Event::get() Error: Attempt to get property \"" << name
                 << "\" as " << PropertyDefn<P>::typeName() <<", actual type is "
                 << sb->getTypeName() << std::endl;
            throw BadType();
        }
	    
    } else {

        std::cerr << "Event::get(): Error: Attempt to get property \"" << name
		  << "\" which doesn't exist for this event\n" << std::endl;
#ifndef NDEBUG
	std::cerr << "Event::get(): Dump follows:" << std::endl;
	dump(std::cerr);
#endif
        throw NoData();
    }
}


template <PropertyType P>
bool
Event::isPersistent(const PropertyName &name) const
    // throw (NoData)
{
    PropertyMap::const_iterator i;
    const PropertyMap *map = find(name, i);

    if (map) {
	return (map == &m_data->m_properties);
    } else {
#ifndef NDEBUG
        std::cerr << "Event::get() Error: Attempt to get persistence of property \""
             << name << "\" which doesn't exist for this element" << std::endl;
#endif
        throw NoData();
    }
}


template <PropertyType P>
void
Event::setPersistence(const PropertyName &name, bool persistent)
    // throw (NoData)
{
    unshare();
    PropertyMap::iterator i;
    PropertyMap *map = find(name, i);

    if (map) {
	insert(*i, persistent);
	map->erase(i);
    } else {
#ifndef NDEBUG
        std::cerr << "Event::get() Error: Attempt to set persistence of property \""
             << name << "\" which doesn't exist for this element" << std::endl;
#endif
        throw NoData();
    }
}


template <PropertyType P>
void
Event::set(const PropertyName &name, typename PropertyDefn<P>::basic_type value,
           bool persistent)
    // throw (BadType)
{
#ifndef NDEBUG
    ++m_setCount;
#endif

    // this is a little slow, could bear improvement

    unshare();
    PropertyMap::iterator i;
    PropertyMap *map = find(name, i);

    if (map) {
	bool persistentBefore = (map == &m_data->m_properties);
	if (persistentBefore != persistent) {
	    i = insert(*i, persistent);
	    map->erase(name);
	}

        PropertyStoreBase *sb = i->second;
        if (sb->getType() == P) {
            (static_cast<PropertyStore<P> *>(sb))->setData(value);
        } else {
#ifndef NDEBUG
            std::cerr << "Error: Event: Attempt to set property \"" << name
                 << "\" as " << PropertyDefn<P>::typeName() <<", actual type is "
                 << sb->getTypeName() << std::endl;
#endif
            throw BadType();
        }
	    
    } else {
        PropertyStoreBase *p = new PropertyStore<P>(value);
	insert(PropertyPair(name, p), persistent);
    }
}



// setMaybe<> is actually called rather more frequently than set<>, so
// it makes sense for best performance to implement it separately
// rather than through calls to has, isPersistent and set<>

template <PropertyType P>
void
Event::setMaybe(const PropertyName &name, typename PropertyDefn<P>::basic_type value)
    // throw (BadType)
{
#ifndef NDEBUG
    ++m_setMaybeCount;
#endif

    unshare();
    PropertyMap::iterator i;
    PropertyMap *map = find(name, i);
    
    if (map) {
	if (map == &m_data->m_properties) return; // persistent, so ignore it

        PropertyStoreBase *sb = i->second;

        if (sb->getType() == P) {
	    (static_cast<PropertyStore<P> *>(sb))->setData(value);
        } else {
#ifndef NDEBUG
            std::cerr << "Error: Event: Attempt to setMaybe property \"" << name
                 << "\" as " << PropertyDefn<P>::typeName() <<", actual type is "
                 << sb->getTypeName() << std::endl;
#endif
            throw BadType();
        }
    } else {
        PropertyStoreBase *p = new PropertyStore<P>(value);
	insert(PropertyPair(name, p), false);
    }
}


template <PropertyType P>
void
Event::setFromString(const PropertyName &name, std::string value, bool persistent)
    // throw (BadType)
{
    set<P>(name, PropertyDefn<P>::parse(value), persistent);
}


//////////////////////////////////////////////////////////////////////

}

#endif
