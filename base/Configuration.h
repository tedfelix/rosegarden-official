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


// Class to hold extraenous bits of configuration which
// don't sit inside the Composition itself - sequencer
// and other general stuff that we want to keep separate.
//
//

#include <string>
#include <vector>

#include "Instrument.h"
#include "RealTime.h"
#include "PropertyMap.h"
#include "Exception.h"
#include "XmlExportable.h"

#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

namespace Rosegarden
{

class Configuration : public PropertyMap, public XmlExportable
{
public:
    class NoData : public Exception {
    public:
	NoData(std::string property, std::string file, int line) :
	    Exception("No data found for property " + property, file, line) { }
    };

    class BadType : public Exception {
    public:
	BadType(std::string property, std::string expected, std::string actual,
		std::string file, int line) :
	    Exception("Bad type for " + property + " (expected " +
		      expected + ", found " + actual + ")", file, line) { }
    };

    Configuration() {;}
    Configuration(const Configuration &);
    ~Configuration();

    bool has(const PropertyName &name) const;

    template <PropertyType P>
    void
    set(const PropertyName &name,
	typename PropertyDefn<P>::basic_type value);

    /**
     * get() with a default value
     */
    template <PropertyType P>
    typename PropertyDefn<P>::basic_type
    get(const PropertyName &name,
	typename PropertyDefn<P>::basic_type defaultVal) const;

    /**
     * regular get()
     */
    template <PropertyType P>
    typename PropertyDefn<P>::basic_type get(const PropertyName &name) const;

    // For exporting -- doesn't write the <configuration> part of
    // the element in case you want to write it into another element
    //
    virtual std::string toXmlString();

    /// Return all the contained property names in alphabetical order
    std::vector<std::string> getPropertyNames();

    // Assignment
    //
    Configuration& operator=(const Configuration &);

private:

};

namespace CompositionMetadataKeys
{
    extern const PropertyName Copyright;
    extern const PropertyName Composer;
    extern const PropertyName Notes;
}

class DocumentConfiguration : public Configuration
{
public:
    DocumentConfiguration();
    DocumentConfiguration(const DocumentConfiguration &);
    ~DocumentConfiguration();

    DocumentConfiguration& operator=(const DocumentConfiguration &);

    // for exporting -- doesn't write the <configuration> part of
    // the element in case you want to write it into another element
    // 
    virtual std::string toXmlString();

    // Property names
    static const PropertyName MetronomePitch;
    static const PropertyName MetronomeBarVelocity;
    static const PropertyName MetronomeBeatVelocity;
    static const PropertyName FetchLatency;
    static const PropertyName MetronomeDuration;
    static const PropertyName SequencerOptions;

    static const PropertyName PluginTaxonomyFile;
    static const PropertyName PluginDescriptionsFile;
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
                   typename PropertyDefn<P>::basic_type defaultVal) const

{
    const_iterator i = find(name);

    if (i == end()) return defaultVal;

    PropertyStoreBase *sb = i->second;
    if (sb->getType() == P) {
        return (static_cast<PropertyStore<P> *>(sb))->getData();
    } else {
	throw BadType(name.getName(),
		      PropertyDefn<P>::typeName(), sb->getTypeName(),
		      __FILE__, __LINE__);
    }
}

template <PropertyType P>
typename PropertyDefn<P>::basic_type
Configuration::get(const PropertyName &name) const

{
    const_iterator i = find(name);

    if (i == end()) throw NoData(name.getName(), __FILE__, __LINE__);

    PropertyStoreBase *sb = i->second;
    if (sb->getType() == P) {
        return (static_cast<PropertyStore<P> *>(sb))->getData();
    } else {
	throw BadType(name.getName(),
		      PropertyDefn<P>::typeName(), sb->getTypeName(),
		      __FILE__, __LINE__);
    }
}

 
}

#endif // _AUDIODEVICE_H_
