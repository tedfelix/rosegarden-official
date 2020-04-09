/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */


/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_EVENT_H
#define RG_EVENT_H

#include "PropertyMap.h"
#include "Exception.h"
#include "TimeT.h"

#include <rosegardenprivate_export.h>

#include <string>
#include <vector>
#include <iostream> // TODO remove (after changing the dump() signature)
#include "misc/Debug.h"


namespace Rosegarden
{

/// A generic Event.
/**
 * The Event class represents an event of arbitrary type with some basic
 * common attributes and an arbitrary number of properties of dynamically-
 * determined name and type.
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
 *
 * Segment is the primary container of Event objects.
 *
 * This class is both generic and polymorphic without using C++ language
 * features (templates and inheritance) to implement those qualities.
 * It would be interesting to explore whether inheritance/polymorphism
 * would lead to an easier to understand and faster implementation of
 * Event.  The concrete types like Note would inherit directly from Event
 * and would provide member objects without using properties and a
 * PropertyMap.
 *
 * There are concrete types such as Note (in NotationTypes.h) and
 * ProgramChange (in MidiTypes.h) which can create Event objects as
 * needed.  Generally, the concrete types provide a "getAs*Event()" routine
 * to create a corresponding Event object.
 */
class ROSEGARDENPRIVATE_EXPORT Event
{
public:
    /// Attempt to access a property that is not present in the Event
    class NoData : public Exception {
    public:
        NoData(const std::string &property) :
            Exception("No data found for property " + property) { }
        NoData(const std::string &property, const std::string &file, int line) :
            Exception("No data found for property " + property, file, line) { }
    };

    /// Attempt to access a property with the wrong type
    class BadType : public Exception {
    public:
        BadType(const std::string &property, const std::string &expected, const std::string &actl) :
            Exception("Bad type for " + property + " (expected " +
                      expected + ", found " + actl + ")") { }
        BadType(const std::string &property, const std::string &expected, const std::string &actual,
                const std::string &file, int line) :
            Exception("Bad type for " + property + " (expected " +
                      expected + ", found " + actual + ")", file, line) { }
    };

    ///////////////////////////////////////////////////////////
    ////////////////////// CONSTRUCTORS ///////////////////////
    ///////////////////////////////////////////////////////////

    Event(const std::string &type,
          timeT absoluteTime, timeT duration = 0, short subOrdering = 0) :
        m_data(new EventData(type, absoluteTime, duration, subOrdering)),
        m_nonPersistentProperties(nullptr)
    { }

    Event(const std::string &type,
          timeT absoluteTime, timeT duration, short subOrdering,
          timeT notationAbsoluteTime, timeT notationDuration) :
        m_data(new EventData(type, absoluteTime, duration, subOrdering)),
        m_nonPersistentProperties(nullptr)
    {
        setNotationAbsoluteTime(notationAbsoluteTime);
        setNotationDuration(notationDuration);
    }

    // these ctors can't use default args: default has to be obtained from e

    Event(const Event &e, timeT absoluteTime) :
        m_nonPersistentProperties(nullptr)
    {
        share(e);
        unshare();
        m_data->m_absoluteTime = absoluteTime;
        setNotationAbsoluteTime(absoluteTime);
        setNotationDuration(m_data->m_duration);
    }

    Event(const Event &e, timeT absoluteTime, timeT duration) :
        m_nonPersistentProperties(nullptr)
    {
        share(e);
        unshare();
        m_data->m_absoluteTime = absoluteTime;
        m_data->m_duration = duration;
        setNotationAbsoluteTime(absoluteTime);
        setNotationDuration(duration);
    }

    Event(const Event &e, timeT absoluteTime,
          timeT duration, short subOrdering):
        m_nonPersistentProperties(nullptr)
    {
        share(e);
        unshare();
        m_data->m_absoluteTime = absoluteTime;
        m_data->m_duration = duration;
        m_data->m_subOrdering = subOrdering;
        setNotationAbsoluteTime(absoluteTime);
        setNotationDuration(duration);
    }

    Event(const Event &e, timeT absoluteTime, timeT duration, short subOrdering,
          timeT notationAbsoluteTime) :
        m_nonPersistentProperties(nullptr)
    {
        share(e);
        unshare();
        m_data->m_absoluteTime = absoluteTime;
        m_data->m_duration = duration;
        m_data->m_subOrdering = subOrdering;
        setNotationAbsoluteTime(notationAbsoluteTime);
        setNotationDuration(duration);
    }

    Event(const Event &e, timeT absoluteTime, timeT duration, short subOrdering,
          timeT notationAbsoluteTime, timeT notationDuration) :
        m_nonPersistentProperties(nullptr)
    {
        share(e);
        unshare();
        m_data->m_absoluteTime = absoluteTime;
        m_data->m_duration = duration;
        m_data->m_subOrdering = subOrdering;
        setNotationAbsoluteTime(notationAbsoluteTime);
        setNotationDuration(notationDuration);
    }

    ~Event()  { lose(); }

    Event(const Event &e) :
        m_nonPersistentProperties(nullptr)
    {
        share(e);
    }

    Event &operator=(const Event &e)
    {
        // If they aren't the same...
        if (&e != this) {
            lose();
            share(e);
        }

        return *this;
    }

    Event *copyMoving(timeT offset) const
    {
        return new Event(*this,
                         m_data->m_absoluteTime + offset,
                         m_data->m_duration,
                         m_data->m_subOrdering,
                         getNotationAbsoluteTime() + offset,
                         getNotationDuration());
    }

    friend bool operator<(const Event&, const Event&);

    ///////////////////////////////////////////////////////////
    /////////////////////// ACCESSORS /////////////////////////
    ///////////////////////////////////////////////////////////

    /**
     * Returns the type of the Event (usually a Note, an Accidental, a
     * Key ... see NotationTypes.h and MidiTypes.h for more examples)
     */
    const std::string &getType() const  { return  m_data->m_type; }

    /// Check Event type.
    bool isa(const std::string &type) const  { return (m_data->m_type == type); }

    timeT getAbsoluteTime() const  { return m_data->m_absoluteTime; }
    timeT getDuration() const  { return m_data->m_duration; }
    short getSubOrdering() const  { return m_data->m_subOrdering; }

    /**
     * Tests if the Event has the property/data in parameter
     */
    bool has(const PropertyName &name) const;

    /// Get the value for a property
    /**
     * \returns The value of the property.
     * \throws NoData
     * \throws BadType
     */
    template <PropertyType P>
    typename PropertyDefn<P>::basic_type get(const PropertyName &name) const;

    /// Get the value for a property
    /**
     * \returns true if the property value was successfully retrieved.
     */
    template <PropertyType P>
    bool get(const PropertyName &name, typename PropertyDefn<P>::basic_type &val) const;

    /// Get the value for a property as a std::string
    /**
     * \throws NoData
     */
    std::string getAsString(const PropertyName &name) const;

    /**
     * Tests if the specified property/data is persistent (is copied
     * when duplicating the Event) or not
     * \throws NoData
     */
    template <PropertyType P>
    bool isPersistent(const PropertyName &name) const;

    /**
     * \throws NoData
     */
    //template <PropertyType P>
    //void setPersistence(const PropertyName &name, bool persistent);

    /**
     * \throws NoData
     */
    PropertyType getPropertyType(const PropertyName &name) const;

    /**
     * \throws NoData
     */
    std::string getPropertyTypeAsString(const PropertyName &name) const;

    /// Set the value for a property.
    /**
     * If the property/data already exists, this function just modifies the
     * stored value, and if not, it creates the association.
     *
     * \throws BadType
     */
    template <PropertyType P>
    void set(const PropertyName &name, typename PropertyDefn<P>::basic_type value,
             bool persistent = true);

    /// Set the value for a property if it doesn't exist as a persistent value.
    /**
     * \throws BadType
     */
    template <PropertyType P>
    void setMaybe(const PropertyName &name, typename PropertyDefn<P>::basic_type value);

    /// Destroy a property.
    /**
     * Does nothing if the property does not exist.
     */
    void unset(const PropertyName &name);

    timeT getNotationAbsoluteTime() const  { return m_data->getNotationTime(); }
    timeT getNotationDuration() const  { return m_data->getNotationDuration(); }

    /**
     * Returns the greater of getDuration() or getNotationDuration() for Note
     * Events.  Returns getDuration() for all other Event types.
     *
     * \author Tito Latini
     */
    timeT getGreaterDuration();

    /**
     * Return whether this Event's section of a triggered ornament
     * is masked, for use when the Event is part of a multiple-tied-note
     * ornament trigger.
     **/
    bool maskedInTrigger() const;
    
    typedef std::vector<PropertyName> PropertyNames;
    //PropertyNames getPropertyNames() const;
    PropertyNames getPersistentPropertyNames() const;
    PropertyNames getNonPersistentPropertyNames() const;

    /**
     * Destroy all the non persistent properties.
     */
    void clearNonPersistentProperties();

    /// Move Event in time without any ancillary co-ordination.
    /**
     * UNSAFE.  Don't call this unless you know exactly what you're
     * doing.
     */
    void unsafeChangeTime(timeT offset);

    /// Compare Event objects using Event::operator<.
    /**
     * Used when creating sets and multisets of Event objects, like Segment.
     */
    struct EventCmp
    {
        //bool operator()(const Event &e1, const Event &e2) const
        //{
        //    return e1 < e2;
        //}
        bool operator()(const Event *e1, const Event *e2) const
        {
            return *e1 < *e2;
        }
    };

    /// Compare Event objects based on their end times.
    /**
     * Used for example in classes that export to other formats, like
     * Lilypond.
     */
    struct EventEndCmp
    {
        //bool operator()(const Event &e1, const Event &e2) const
        //{
        //    return e1.getAbsoluteTime() + e1.getDuration() <=
        //        e2.getAbsoluteTime() + e2.getDuration();
        //}
        bool operator()(const Event *e1, const Event *e2) const
        {
            return e1->getAbsoluteTime() + e1->getDuration() <=
                e2->getAbsoluteTime() + e2->getDuration();
        }
    };

    /// approximate, for debugging and inspection purposes
    size_t getStorageSize() const;

    /**
     * Get the XML string representing the object.  If the absolute
     * time of the event differs from the given absolute time, include
     * the difference between the two as a timeOffset attribute.
     * If expectedTime == 0, include an absoluteTime attribute instead.
     */
    std::string toXmlString(timeT expectedTime) const;

#ifndef NDEBUG
    void dump(std::ostream&) const;
#else
    void dump(std::ostream&) const {}
#endif
    static void dumpStats(std::ostream&);

    // UNUSED

    /// Set the value for a property from a std::string.
    /**
     * If the property/data already exists, this function just modifies the
     * stored value, and if not, it creates the association.
     *
     * \param name the name of the property/data
     * \param value the value of the property/data in a string form.  This
     *              string will be parsed to compute the actual value stored
     *              in the property/data.
     *
     * \throws BadType
     */
    //template <PropertyType P>
    //void setFromString(const PropertyName &name, std::string value,
    //                   bool persistent = true);

    /// Does the Event start before (strict) time t?
    //static bool compareEvent2Time(const Event *e, timeT t)
    //{
    //    return e->getAbsoluteTime() < t;
    //}

    /**
     * Tests if the input Event starts after (strict) the time in parameter
     */
    //static bool compareTime2Event(timeT t, const Event *e) {
    //    return t <  e->getAbsoluteTime();
    //}

    /**
     * Get the XML string representing the object.
     */
    //std::string toXmlString() const;


protected:
    // these are for subclasses such as XmlStorableEvent

    Event() :
        m_data(new EventData("", 0, 0, 0)),
        m_nonPersistentProperties(nullptr) { }

    void setType(const std::string &t) { unshare(); m_data->m_type = t; }
    void setAbsoluteTime(timeT t)      { unshare(); m_data->m_absoluteTime = t; }
    void setDuration(timeT d)          { unshare(); m_data->m_duration = d; }
    void setSubOrdering(short o)       { unshare(); m_data->m_subOrdering = o; }
    void setNotationAbsoluteTime(timeT t) { unshare(); m_data->setNotationTime(t); }
    void setNotationDuration(timeT d) { unshare(); m_data->setNotationDuration(d); }

private:
    bool operator==(const Event &); // not implemented

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
        m_nonPersistentProperties = nullptr;
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
            RG_DEBUG << "get() Error: Attempt to get property \"" << name.getName()
                 << "\" as" << PropertyDefn<P>::typeName() <<", actual type is"
                 << sb->getTypeName();
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
        RG_DEBUG << "get(): Error dump follows:";
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

#if 0
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
#endif


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

#if 0
template <PropertyType P>
void
Event::setFromString(const PropertyName &name, std::string value, bool persistent)
    // throw (BadType)
{
    set<P>(name, PropertyDefn<P>::parse(value), persistent);
}
#endif

//////////////////////////////////////////////////////////////////////

}

#endif
