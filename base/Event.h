
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

#include <hash_map>
#include <string>

struct eqstring
{
    bool operator()(const string &s1, const string &s2) const {
        return s1 == s2;
    }
};

struct hashstring
{
    static hash<const char*> _H;
    size_t operator()(const string &s) const {
        return _H(s.c_str());
    }
};

hash<const char*> hashstring::_H;

class Event
{
private:

public:
    typedef hash_map<string, PropertyStoreBase*, hashstring, eqstring>
        PropertyMap;
    typedef int timeT;

    struct NoData { };
    struct BadType { };

    Event();

    Event(const string &package, const string &type);

    Event(const Event &e);

    virtual ~Event();

    Event &operator=(const Event &e);
    friend bool operator<(const Event&, const Event&);

    // Accessors
    const string &package() const { return m_package; }
    const string &type() const    { return m_type; }
    void setPackage(const string &p) { m_package = p; }
    void setType(const string &t)    { m_type = t; }

    bool isa(const string &p, const string &t) const {
        return (m_package == p && m_type == t);
    }

    timeT getAbsoluteTime() const  { return m_absoluteTime; }
    timeT getDuration()     const  { return m_duration; }
    void setAbsoluteTime(timeT d)  { m_absoluteTime = d; }
    void setDuration(timeT d)      { m_duration = d; }

    bool has(const string &name) const;

    template <PropertyType P>
    PropertyDefn<P>::basic_type get(const string &name) const
        throw (NoData, BadType);

    // no throw, returns bool
    template <PropertyType P>
    bool get(const string &name, PropertyDefn<P>::basic_type &val) const;

    template <PropertyType P>
    string getAsString(const string &name) const
	throw (NoData, BadType);

    template <PropertyType P>
    void set(const string &name, PropertyDefn<P>::basic_type value)
	throw (BadType);

    template <PropertyType P>
    void setFromString(const string &name, string value)
	throw (BadType);

    PropertyMap&       properties()       { return m_properties; }
    const PropertyMap& properties() const { return m_properties; }

#ifndef NDEBUG
    void dump(ostream&) const;
#else
    void dump(ostream&) const {}
#endif

private:
    void scrapMap();
    void copyFrom(const Event &e);

    string m_package;
    string m_type;
    timeT m_duration;
    timeT m_absoluteTime;

    //    typedef map<string, PropertyStoreBase *> PropertyMap;
    typedef PropertyMap::value_type PropertyPair;
    PropertyMap m_properties;
};


template <PropertyType P>
bool
Event::get(const string &name, PropertyDefn<P>::basic_type &val) const
{
    PropertyMap::const_iterator i = m_properties.find(name);
    if (i != m_properties.end()) { 

        PropertyStoreBase *sb = (*i).second;
        if (sb->getType() == P) {
            val = ((PropertyStore<P> *)sb)->getData();
            return true;
        }
        else {
#ifndef NDEBUG
            cerr << "Event::get() Error: Attempt to get property \"" << name
                 << "\" as " << PropertyDefn<P>::name() <<", actual type is "
                 << sb->getTypeName() << endl;
#endif
            return false;
        }
	    
    } else {
#ifndef NDEBUG
        cerr << "Event::get() Error: Attempt to get property \"" << name
             << "\" which doesn't exist for this element" << endl;
#endif
        return false;
    }
}


template <PropertyType P>
PropertyDefn<P>::basic_type
Event::get(const string &name) const
    throw (NoData, BadType)
{
    PropertyMap::const_iterator i = m_properties.find(name);
    if (i != m_properties.end()) { 

        PropertyStoreBase *sb = (*i).second;
        if (sb->getType() == P) return ((PropertyStore<P> *)sb)->getData();
        else {
#ifndef NDEBUG
            cerr << "Event::get() Error: Attempt to get property \"" << name
                 << "\" as " << PropertyDefn<P>::name() <<", actual type is "
                 << sb->getTypeName() << endl;
#endif
            throw BadType();
        }
	    
    } else {
#ifndef NDEBUG
        cerr << "Event::get() Error: Attempt to get property \"" << name
             << "\" which doesn't exist for this element" << endl;
#endif
        throw NoData();
    }
}


template <PropertyType P>
string
Event::getAsString(const string &name) const
    throw (NoData, BadType)
{
    return PropertyDefn<P>::unparse(get<P>(name));
}


template <PropertyType P>
void
Event::set(const string &name, PropertyDefn<P>::basic_type value)
    throw (BadType)
{
    PropertyMap::const_iterator i = m_properties.find(name);
    if (i != m_properties.end()) {

        PropertyStoreBase *sb = (*i).second;
        if (sb->getType() == P) ((PropertyStore<P> *)sb)->setData(value);
        else {
            cerr << "Error: Element: Attempt to get property \"" << name
                 << "\" as " << PropertyDefn<P>::name() <<", actual type is "
                 << sb->getTypeName() << endl;
            throw BadType();
        }
	    
    } else {
        PropertyStoreBase *p = new PropertyStore<P>(value);
        m_properties.insert(PropertyPair(name, p));
    }
}


template <PropertyType P>
void
Event::setFromString(const string &name, string value)
    throw (BadType)
{
    set<P>(name, PropertyDefn<P>::parse(value));
}

//////////////////////////////////////////////////////////////////////

class EventCmp
{
public:
    bool operator()(const Event *e1, const Event *e2) const
    {
        return *e1 < *e2;
    }
};

#endif
