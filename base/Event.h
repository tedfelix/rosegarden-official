// -*- c-basic-offset: 4 -*-


/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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
#include "Exception.h"

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
    class NoData : public Exception {
    public:
	NoData(std::string property) :
	    Exception("No data found for property " + property) { }
	NoData(std::string property, std::string file, int line) :
	    Exception("No data found for property " + property, file, line) { }
    };

    class BadType : public Exception {
    public:
	BadType(std::string property, std::string expected, std::string actl) :
	    Exception("Bad type for " + property + " (expected " +
		      expected + ", found " + actl + ")") { }
	BadType(std::string property, std::string expected, std::string actual,
		std::string file, int line) :
	    Exception("Bad type for " + property + " (expected " +
		      expected + ", found " + actual + ")", file, line) { }
    };

    Event(const std::string &type,
	  timeT absoluteTime, timeT duration = 0, short subOrdering = 0) :
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
	setNotationAbsoluteTime(absoluteTime);
	setNotationDuration(m_data->m_duration);
    }

    Event(const Event &e, timeT absoluteTime, timeT duration) :
	m_nonPersistentProperties(0) {
	share(e);
	unshare();
	m_data->m_absoluteTime = absoluteTime;
	m_data->m_duration = duration;
	setNotationAbsoluteTime(absoluteTime);
	setNotationDuration(duration);
    }

    Event(const Event &e, timeT absoluteTime, timeT duration, short subOrdering):
	m_nonPersistentProperties(0) {
	share(e);
	unshare();
	m_data->m_absoluteTime = absoluteTime;
	m_data->m_duration = duration;
	m_data->m_subOrdering = subOrdering;
	setNotationAbsoluteTime(absoluteTime);
	setNotationDuration(duration);
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
    short getSubOrdering()  const    { return m_data->m_subOrdering; }

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

    timeT getNotationAbsoluteTime() const { return m_data->getNotationTime(); }
    timeT getNotationDuration() const { return m_data->getNotationDuration(); }
    void setNotationAbsoluteTime(timeT t) { unshare(); m_data->setNotationTime(t); }
    void setNotationDuration(timeT d) { unshare(); m_data->setNotationDuration(d); }
    
    typedef std::vector<PropertyName> PropertyNames;
    PropertyNames getPropertyNames() const;
    PropertyNames getPersistentPropertyNames() const;
    PropertyNames getNonPersistentPropertyNames() const;

    void clearNonPersistentProperties();

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

    /**
     * Get the XML string representing the object.
     */
    std::string toXmlString();

    /**
     * Get the XML string representing the object.  If the absolute
     * time of the event differs from the given absolute time, include
     * the difference between the two as a timeOffset attribute.
     * If expectedTime == 0, include an absoluteTime attribute instead.
     */
    std::string toXmlString(timeT expectedTime);

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
    void setSubOrdering(short o)       { unshare(); m_data->m_subOrdering = o; }

private:
    struct EventData // Data that are shared between shallow-copied instances
    {
	EventData(const std::string &type,
		  timeT absoluteTime, timeT duration, short subOrdering);
	EventData(const std::string &type,
		  timeT absoluteTime, timeT duration, short subOrdering,
		  const PropertyMap *properties);
	EventData *unshare();
	~EventData();
	unsigned int m_refCount;

	std::string m_type;
	timeT m_absoluteTime;
	timeT m_duration;
	short m_subOrdering;

	PropertyMap *m_properties;

	// These are properties because we don't care so much about
	// raw speed in get/set, but we do care about storage size for
	// events that don't have them or that have zero values:
	timeT getNotationTime() const;
	timeT getNotationDuration() const;
	void setNotationTime(timeT t) {
	    setTime(NotationTime, t, m_absoluteTime);
	}
	void setNotationDuration(timeT d) {
	    setTime(NotationDuration, d, m_duration);
	}

    private:
	EventData(const EventData &);
	EventData &operator=(const EventData &);
	static PropertyName NotationTime;
	static PropertyName NotationDuration;
	void setTime(const PropertyName &name, timeT value, timeT deft);
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
	PropertyMap **map =
	    (persistent ? &m_data->m_properties : &m_nonPersistentProperties);
	if (!*map) *map = new PropertyMap();
	return (*map)->insert(pair).first;
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
            throw BadType(name.getName(),
			  PropertyDefn<P>::typeName(), sb->getTypeName(),
			  __FILE__, __LINE__);
        }
	    
    } else {

#ifndef NDEBUG
	std::cerr << "Event::get(): Error dump follows:" << std::endl;
	dump(std::cerr);
#endif
        throw NoData(name.getName(), __FILE__, __LINE__);
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
	return (map == m_data->m_properties);
    } else {
	throw NoData(name.getName(), __FILE__, __LINE__);
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
        throw NoData(name.getName(), __FILE__, __LINE__);
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
	bool persistentBefore = (map == m_data->m_properties);
	if (persistentBefore != persistent) {
	    i = insert(*i, persistent);
	    map->erase(name);
	}

        PropertyStoreBase *sb = i->second;
        if (sb->getType() == P) {
            (static_cast<PropertyStore<P> *>(sb))->setData(value);
        } else {
            throw BadType(name.getName(),
			  PropertyDefn<P>::typeName(), sb->getTypeName(),
			  __FILE__, __LINE__);
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
	if (map == m_data->m_properties) return; // persistent, so ignore it

        PropertyStoreBase *sb = i->second;

        if (sb->getType() == P) {
	    (static_cast<PropertyStore<P> *>(sb))->setData(value);
        } else {
            throw BadType(name.getName(),
			  PropertyDefn<P>::typeName(), sb->getTypeName(),
			  __FILE__, __LINE__);
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
