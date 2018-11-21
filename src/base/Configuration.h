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
#include "base/Exception.h"
#include "XmlExportable.h"

#ifndef RG_CONFIGURATION_H
#define RG_CONFIGURATION_H

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
    ~Configuration() override;

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
    std::string toXmlString() const override;

    /// Return all the contained property names in alphabetical order
    std::vector<std::string> getPropertyNames();

    // Assignment
    //
    Configuration& operator=(const Configuration &);

private:

};

namespace CompositionMetadataKeys
{
    extern const PropertyName Composer;
    extern const PropertyName Arranger;
    extern const PropertyName Copyright;
    extern const PropertyName Title;
    extern const PropertyName Subtitle;
    // The following are recognized only by LilyPond output
    extern const PropertyName Subsubtitle;
    extern const PropertyName Dedication;
    extern const PropertyName Poet;
    extern const PropertyName Meter;
    extern const PropertyName Opus;
    extern const PropertyName Instrument;
    extern const PropertyName Piece;
    extern const PropertyName Tagline;


    std::vector<PropertyName> getFixedKeys();
}

class DocumentConfiguration : public Configuration
{
public:
    DocumentConfiguration();
    DocumentConfiguration(const DocumentConfiguration &);
    ~DocumentConfiguration() override;

    DocumentConfiguration& operator=(const DocumentConfiguration &);

    // for exporting -- doesn't write the <configuration> part of
    // the element in case you want to write it into another element
    // 
    std::string toXmlString() const override;

    // Property names
    static const PropertyName SequencerOptions;

    static const PropertyName ZoomLevel;

    static const PropertyName TransportMode;
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

#endif // RG_CONFIGURATION_H
