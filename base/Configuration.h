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


// Class to hold extraenous bits of configuration which
// don't sit inside the Composition itself - sequencer
// and other general stuff that we want to keep separate.
//
//

#include <string>

#include "Instrument.h"
#include "RealTime.h"
#include "PropertyMap.h"

#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

namespace Rosegarden
{

class Configuration : public PropertyMap
{
public:

    Configuration(); // defaults
    ~Configuration();

    struct NoData { };
    struct BadType { };

    template <PropertyType P>
    void set(const PropertyName &name,
             typename PropertyDefn<P>::basic_type value);

    /**
     * get() with a default value
     */
    template <PropertyType P>
    typename PropertyDefn<P>::basic_type get(const PropertyName &name,
                                             PropertyDefn<P>::basic_type defaultVal) const;

    /**
     * regulat get()
     */
    template <PropertyType P>
    typename PropertyDefn<P>::basic_type get(const PropertyName &name) const;

//     void setFetchLatency(const RealTime &value) { m_fetchLatency = value; }
//     RealTime getFetchLatency() const { return m_fetchLatency; }

//     void setMetronomePitch(MidiByte value) { m_metronomePitch = value; }
//     MidiByte getMetronomePitch() const { return m_metronomePitch; }

//     void setMetronomeBarVelocity(MidiByte value)
//         { m_metronomeBarVelocity = value; }
//     MidiByte getMetronomeBarVelocity() const { return m_metronomeBarVelocity; }

//     void setMetronomeBeatVelocity(MidiByte value)
//         { m_metronomeBeatVelocity = value; }
//     MidiByte getMetronomeBeatVelocity() const {return m_metronomeBeatVelocity;}

//     void setMetronomeDuration(const RealTime &value)
//         { m_metronomeDuration = value; }
//     RealTime getMetronomeDuration() const { return m_metronomeDuration; }

    // for exporting
    //
    virtual std::string toXmlString();

private:

    RealTime     m_fetchLatency;

    MidiByte     m_metronomePitch;
    MidiByte     m_metronomeBarVelocity;
    MidiByte     m_metronomeBeatVelocity;
    RealTime     m_metronomeDuration;

//     DoubleClickClient   m_client;
    
};

template <PropertyType P>
void
Configuration::set(const PropertyName &name,
                   typename PropertyDefn<P>::basic_type value)
{
    iterator i = find(name);

    if (i != end()) {

        // A property with the same name has 
        // already been set - recycle it, just change the data
        PropertyStoreBase *sb = i->second;
        (static_cast<PropertyStore<P> *>(sb))->setData(value);

    } else {
        
        PropertyStoreBase *p = new PropertyStore<P>(value);
        insert(PropertyPair(name, p));

    }
    
}

template <PropertyType P>
typename PropertyDefn<P>::basic_type
Configuration::get(const PropertyName &name,
                   PropertyDefn<P>::basic_type defaultVal) const

{
    const_iterator i = find(name);

    PropertyStoreBase *sb = i->second;
    if (sb->getType() == P)
        return (static_cast<PropertyStore<P> *>(sb))->getData();

    return defaultVal;
}

template <PropertyType P>
typename PropertyDefn<P>::basic_type
Configuration::get(const PropertyName &name) const

{
    const_iterator i = find(name);

    PropertyStoreBase *sb = i->second;
    if (sb->getType() == P)
        return (static_cast<PropertyStore<P> *>(sb))->getData();


    std::cerr << "Configuration::get(): Error: Attempt to get property \"" << name
              << "\" which doesn't exist\n" << std::endl;

    throw NoData();
}

 
}

#endif // _AUDIODEVICE_H_
