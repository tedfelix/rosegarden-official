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
#include "misc/Debug.h"

#include <rosegardenprivate_export.h>

#include <string>
#include <vector>
#include <iostream>


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
 * PropertyMap.  One key downside is that older versions of rg would then
 * be unable to preserve properties that they do not understand.
 * Not sure that's a very big deal given that the properties have been
 * pretty stable for quite a while.
 *
 * There are concrete types such as Note (in NotationTypes.h) and
 * ProgramChange (in MidiTypes.h) which can create Event objects as
 * needed.  Generally, the concrete types provide a "getAs*Event()" routine
 * to create a corresponding Event object.
 */
class ROSEGARDENPRIVATE_EXPORT Event
{
public:

    // *** Exceptions

    /// Attempt to access a property that is not present in the Event
    class NoData : public Exception {
    public:
        NoData(const std::string &property) :
            Exception("No data found for property " + property)
        { }
        NoData(const std::string &property, const std::string &file, int line) :
            Exception("No data found for property " + property, file, line)
        { }
    };

    /// Attempt to access a property with the wrong type
    class BadType : public Exception {
    public:
        BadType(const std::string &property, const std::string &expected, const std::string &actual) :
            Exception("Bad type for " + property + " (expected " +
                      expected + ", found " + actual + ")")
        { }
        BadType(const std::string &property, const std::string &expected, const std::string &actual,
                const std::string &file, int line) :
            Exception("Bad type for " + property + " (expected " +
                      expected + ", found " + actual + ")", file, line)
        { }
    };

    // *** Constructors

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

    /**
     * Returns the type of the Event (E.g. Note, Accidental, Key, etc...)
     * See NotationTypes.h and MidiTypes.h for more examples.
     */
    const std::string &getType() const  { return  m_data->m_type; }
    /// Check Event type.
    bool isa(const std::string &type) const  { return (m_data->m_type == type); }

    timeT getAbsoluteTime() const  { return m_data->m_absoluteTime; }
    timeT getNotationAbsoluteTime() const  { return m_data->getNotationTime(); }
    /// Move Event in time without any ancillary coordination.
    /**
     * UNSAFE.  Don't call this unless you know exactly what you're doing.
     */
    void unsafeChangeTime(timeT offset);

    timeT getDuration() const  { return m_data->m_duration; }
    timeT getNotationDuration() const  { return m_data->getNotationDuration(); }
    /**
     * Returns the greater of getDuration() or getNotationDuration() for Note
     * Events.  Returns getDuration() for all other Event types.
     *
     * \author Tito Latini
     */
    timeT getGreaterDuration();

    short getSubOrdering() const  { return m_data->m_subOrdering; }

    /**
     * Return whether this Event's section of a triggered ornament
     * is masked, for use when the Event is part of a multiple-tied-note
     * ornament trigger.  Uses the TRIGGER_EXPAND property.
     */
    bool maskedInTrigger() const;

    // *** Properties (name/value pairs)

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

    typedef std::vector<PropertyName> PropertyNames;
    PropertyNames getPersistentPropertyNames() const;
    PropertyNames getNonPersistentPropertyNames() const;

    /**
     * Destroy all the non persistent properties.
     */
    void clearNonPersistentProperties();

    /// Compare Event objects using Event::operator<.
    /**
     * Used when creating sets and multisets of Event objects, like Segment.
     */
    struct EventCmp
    {
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
        bool operator()(const Event *e1, const Event *e2) const
        {
            return e1->getAbsoluteTime() + e1->getDuration() <=
                e2->getAbsoluteTime() + e2->getDuration();
        }
    };

    /// Get the XML string representing the object.
    /**
     * If the absolute time of the event differs from the expected time,
     * include the difference between the two as a timeOffset attribute.
     * If expectedTime == 0, include an absoluteTime attribute instead.
     */
    std::string toXmlString(timeT expectedTime) const;

    // *** DEBUG

    /// Approximate.  For debugging and inspection purposes.
    size_t getStorageSize() const;

    // UNUSED
    static void dumpStats(std::ostream &);

protected:

    // Interface for subclasses such as XmlStorableEvent.

    Event() :
        m_data(new EventData("", 0, 0, 0)),
        m_nonPersistentProperties(nullptr)
    { }

    void setType(const std::string &t) { unshare(); m_data->m_type = t; }
    void setAbsoluteTime(timeT t)      { unshare(); m_data->m_absoluteTime = t; }
    void setDuration(timeT d)          { unshare(); m_data->m_duration = d; }
    void setSubOrdering(short o)       { unshare(); m_data->m_subOrdering = o; }
    void setNotationAbsoluteTime(timeT t) { unshare(); m_data->setNotationTime(t); }
    void setNotationDuration(timeT d) { unshare(); m_data->setNotationDuration(d); }

private:
    friend QDebug &operator<<(QDebug &dbg, const Event &event);

    /// Data that are shared between shallow-copied instances
    struct EventData
    {
        EventData(const std::string &type,
                  timeT absoluteTime, timeT duration, short subOrdering);
        EventData(const std::string &type,
                  timeT absoluteTime, timeT duration, short subOrdering,
                  const PropertyMap *properties);
        /// Make a unique copy.  Used for Copy On Write.
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
        void setNotationTime(timeT t)
            { setTime(NotationTime, t, m_absoluteTime); }
        timeT getNotationTime() const;
        void setNotationDuration(timeT d)
            { setTime(NotationDuration, d, m_duration); }
        timeT getNotationDuration() const;

    private:
        EventData(const EventData &);
        EventData &operator=(const EventData &);

        static PropertyName NotationTime;
        static PropertyName NotationDuration;

        /// Add the time property unless (t == deft).
        void setTime(const PropertyName &name, timeT t, timeT deft);
    };

    // ??? This is a reference counted smart pointer supporting Copy
    //     On Write.  We probably can't replace it with a QSharedPointer.
    //     It would be more interesting to make Copy On Write disable-able
    //     and see if it makes any sort of performance or memory difference.
    //     Might also be a good idea to see if C++11 move semantics would help.
    EventData *m_data;
    // ??? This doesn't seem to participate in Copy On Write, so a
    //     QSharedPointer should simplify managing this.
    PropertyMap *m_nonPersistentProperties; // Unique to an instance

    void share(const Event &e)
    {
        m_data = e.m_data;
        m_data->m_refCount++;
    }

    /// Makes a copy.  Used for Copy On Write.
    /**
     * Returns true if unshare was actually necessary.
     */
    bool unshare()
    {
        if (m_data->m_refCount > 1) {
            m_data = m_data->unshare();
            return true;
        } else {
            return false;
        }
    }

    /// Dereference and delete.
    void lose()
    {
        if (--m_data->m_refCount == 0)
            delete m_data;
        delete m_nonPersistentProperties;
        m_nonPersistentProperties = nullptr;
    }

    /// Find a property in both the persistent and non-persistent properties.
    /**
     * @param[in]  name The property to find.
     * @param[out] i    An iterator pointing to the property that was found.
     *                  Invalid if the function return value is nullptr.
     * \return The map in which the property was found.  Returns nullptr
     *         otherwise.
     */
    PropertyMap *find(const PropertyName &name, PropertyMap::iterator &i);

    /// Find a property in both the persistent and non-persistent properties.
    /**
     * @param[in]  name The property to find.
     * @param[out] i    An iterator pointing to the property that was found.
     *                  Invalid if the function return value is nullptr.
     * \return The map in which the property was found.  Returns nullptr
     *         otherwise.
     */
    const PropertyMap *find(const PropertyName &name,
                            PropertyMap::const_iterator &i) const
    {
        PropertyMap::iterator j;
        PropertyMap *map = const_cast<Event *>(this)->find(name, j);
        i = j;
        return map;
    }

    PropertyMap::iterator insert(const PropertyPair &pair, bool persistent)
    {
        PropertyMap **map =
            (persistent ? &m_data->m_properties : &m_nonPersistentProperties);

        // If the map hasn't been created yet, create it.
        if (!*map)
            *map = new PropertyMap();

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

extern QDebug &operator<<(QDebug &dbg, const Event &event);

template <PropertyType P>
bool
Event::get(const PropertyName &name,
           typename PropertyDefn<P>::basic_type &val) const
{
#ifndef NDEBUG
    ++m_getCount;
#endif

    PropertyMap::const_iterator i;
    const PropertyMap *map = find(name, i);

    // Not found?  Bail.
    if (!map)
        return false;

    PropertyStoreBase *sb = i->second;
    if (sb->getType() == P) {
        val = (static_cast<PropertyStore<P> *>(sb))->getData();
        return true;
    } else {
#ifndef NDEBUG
        RG_DEBUG << "get() Error: Attempt to get property \"" << name.getName() << "\" as" << PropertyDefn<P>::typeName() <<", actual type is" << sb->getTypeName();
#endif
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
        RG_DEBUG << "Event::get(): Property" << name.getName().c_str() << "not found for Event:";
        RG_DEBUG << *this;
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

    if (!map)
        throw NoData(name.getName(), __FILE__, __LINE__);

    return (map == m_data->m_properties);
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

    // Copy on Write
    unshare();

    PropertyMap::iterator i;
    PropertyMap *map = find(name, i);

    // If found, update.
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

    } else {  // Create
        PropertyStoreBase *p = new PropertyStore<P>(value);
        insert(PropertyPair(name, p), persistent);
    }
}


template <PropertyType P>
void
Event::setMaybe(const PropertyName &name, typename PropertyDefn<P>::basic_type value)
    // throw (BadType)
{
    // setMaybe<> is actually called rather more frequently than set<>, so
    // it makes sense for best performance to implement it separately
    // rather than through calls to has, isPersistent and set<>

#ifndef NDEBUG
    ++m_setMaybeCount;
#endif

    // Copy On Write
    unshare();

    PropertyMap::iterator i;
    PropertyMap *map = find(name, i);

    // If found, update only if not persistent
    if (map) {
        // If persistent, bail.
        if (map == m_data->m_properties)
            return;

        PropertyStoreBase *sb = i->second;

        if (sb->getType() == P) {
            (static_cast<PropertyStore<P> *>(sb))->setData(value);
        } else {
            throw BadType(name.getName(),
                          PropertyDefn<P>::typeName(), sb->getTypeName(),
                          __FILE__, __LINE__);
        }
    } else {  // Create
        PropertyStoreBase *p = new PropertyStore<P>(value);
        insert(PropertyPair(name, p),
               false);  // persistent
    }
}


}

#endif
