
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

#include <list>
#include <multiset.h>
#include <hash_map>
#include <string>

// Need to associate one of those param names (Int, String etc) with a
// storage type and a parser/writer.  The storage-type association
// obviously has to be made at compile-time, with a template.

// This is still a wee bit messy at the moment, but the principle's
// interesting.  Performance is almost identical to that of the more
// static-stylee Element (probably dominated by map lookup/insert; a
// vector might be faster in practice as we expect relatively few
// properties per element)

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

enum PropertyType { Int, String, Bool, Tag };

template <PropertyType P>
class PropertyDefn
{
public:
    static string name() { return "Undefined"; }
    struct PropertyDefnNotDefined {
	PropertyDefnNotDefined() { assert(0); }
    };
    typedef PropertyDefnNotDefined basic_type;
    static basic_type parse(string);
    static string unparse(basic_type);

};

template <PropertyType P>
PropertyDefn<P>::basic_type
PropertyDefn<P>::parse(string)
{
    assert(0);
}

template <PropertyType P>
string 
PropertyDefn<P>::unparse(PropertyDefn<P>::basic_type)
{
    assert(0);
}


template <>
class PropertyDefn<Int>
{
public:
    static string name();
    typedef long basic_type;

    static basic_type parse(string s);
    static string unparse(basic_type i);
};


template <>
class PropertyDefn<String>
{
public:
    static string name();
    typedef string basic_type;

    static basic_type parse(string s);
    static string unparse(basic_type i);
};

template <>
class PropertyDefn<Bool>
{
public:
    static string name();
    typedef bool basic_type;

    static basic_type parse(string s);
    static string unparse(basic_type i);
};


class PropertyStoreBase {
public:
    virtual ~PropertyStoreBase();

    virtual PropertyType getType() const = 0;
    virtual string getTypeName() const = 0;
    virtual PropertyStoreBase *clone() = 0;
    virtual string unparse() = 0;

#ifndef NDEBUG
    virtual void dump(ostream&) const = 0;
#else
    virtual void dump(ostream&) const {}
#endif
};

#ifndef NDEBUG
inline ostream& operator<<(ostream &out, PropertyStoreBase &e)
{ e.dump(out); return out; }
#endif

template <PropertyType P>
class PropertyStore : public PropertyStoreBase
{
public:
    PropertyStore(PropertyDefn<P>::basic_type d) : m_data(d) { }
    PropertyStore(const PropertyStore<P> &p) : PropertyStoreBase(p), m_data(p.m_data) { }
    PropertyStore &operator=(const PropertyStore<P> &p);

    virtual PropertyType getType() const;
    virtual string getTypeName() const;

    virtual PropertyStoreBase* clone();
    

    virtual string unparse();
    

    PropertyDefn<P>::basic_type getData() { return m_data; }
    void setData(PropertyDefn<P>::basic_type data) { m_data = data; }

#ifndef NDEBUG
    void dump(ostream&) const;
#endif

private:
    PropertyDefn<P>::basic_type m_data;
};

template <PropertyType P>
PropertyStore<P>&
PropertyStore<P>::operator=(const PropertyStore<P> &p) {
    if (this != &p) m_data = p.m_data;
    return *this;
}

template <PropertyType P>
PropertyType
PropertyStore<P>::getType() const
{
    return P;
}

template <PropertyType P>
string
PropertyStore<P>::getTypeName() const
{
    return PropertyDefn<P>::name();
}

template <PropertyType P>
PropertyStoreBase*
PropertyStore<P>::clone()
{
    return new PropertyStore<P>(*this);
}

template <PropertyType P>
string
PropertyStore<P>::unparse()
{
    return PropertyDefn<P>::unparse(m_data);
}

#ifndef NDEBUG
template <PropertyType P>
void
PropertyStore<P>::dump(ostream &out) const
{
    out << getTypeName() << " - " << m_data;
}
#endif


//////////////////////////////////////////////////////////////////////

class ViewElement; // defined below
class ViewElements : public vector<ViewElement*>
{
public:
    ViewElements() : vector<ViewElement*>() {}
    ViewElements(const ViewElements &e) : vector<ViewElement*>(e) {}
    ~ViewElements();
};

//////////////////////////////////////////////////////////////////////

// see rosegarden/docs/discussion/names.txt - Events are the basic datatype

class Event
{
private:

public:
    typedef hash_map<string, PropertyStoreBase*, hashstring, eqstring> PropertyMap;
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
    void setViewElements(ViewElements*);
    void scrapViewElements();
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


/**
 * This class owns the Events its items are pointing at
 */
class Track : public multiset<Event*, EventCmp>
{
public:
    Track(unsigned int nbBars = 0, unsigned int startIdx = 0);
    ~Track();

    unsigned int getStartIndex() const         { return m_startIdx; }
    void         setStartIndex(unsigned int i) { m_startIdx = i; }

    unsigned int getNbBars() const;
    
protected:
    unsigned int m_startIdx;
    unsigned int m_nbBars;
};

/**
 * A set of tracks.
 * This class owns the event lists it is holding
 * It will delete them on destruction.
 */
class Composition
{
    
public:
    typedef vector<Track*> trackcontainer;

    typedef trackcontainer::iterator iterator;
    typedef trackcontainer::const_iterator const_iterator;

    Composition(unsigned int nbTracks = 64);
    ~Composition();

    vector<Track*>& tracks() { return m_tracks; }

    bool addTrack(Track *track = 0, int idx = -1);
    void deleteTrack(int idx);

    unsigned int getNbTracks() const { return m_tracks.size(); }
    unsigned int getNbBars() const;
    void         clear();

    unsigned int getNbTicksPerBar() const { return m_nbTicksPerBar; }
    void setNbTicksPerBar(unsigned int n) { m_nbTicksPerBar = n; }

    // Some vector<> API delegation
    iterator       begin()       { return m_tracks.begin(); }
    const_iterator begin() const { return m_tracks.begin(); }
    iterator       end()         { return m_tracks.end(); }
    const_iterator end() const   { return m_tracks.end(); }

    Track*       operator[](int i)       { return m_tracks[i]; }
    const Track* operator[](int i) const { return m_tracks[i]; }

protected:
    trackcontainer m_tracks;

    unsigned int m_nbTicksPerBar;
};


//////////////////////////////////////////////////////////////////////


class ViewElement
{
public:
    ViewElement(Event*);
    virtual ~ViewElement();

    const Event* event() const { return m_event; }
    Event*       event()       { return m_event; }

    Event::timeT getAbsoluteTime() const { return event()->getAbsoluteTime(); }
    void setAbsoluteTime(Event::timeT d) { event()->setAbsoluteTime(d); }

    void dump(ostream&) const;

    friend bool operator<(const ViewElement&, const ViewElement&);

protected:
    Event *m_event;
};

#endif
