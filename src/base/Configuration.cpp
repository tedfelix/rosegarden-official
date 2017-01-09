/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


// Class to hold extraneous bits of configuration which
// don't sit inside the Composition itself - sequencer
// and other general stuff that we want to keep separate.
//
//

#include <string>
#include <algorithm>

#include "Configuration.h"

#include <sstream>


namespace Rosegarden
{

Configuration::Configuration(const Configuration &conf) :
    PropertyMap(),
    XmlExportable()
{
    clear();

    // Copy everything
    //
    for (const_iterator i = conf.begin(); i != conf.end(); ++i)
        insert(PropertyPair(i->first, i->second->clone()));
}

Configuration::~Configuration()
{
    clear();
}


std::vector<std::string>
Configuration::getPropertyNames()
{
    std::vector<std::string> v;
    for (const_iterator i = begin(); i != end(); ++i) {
	v.push_back(i->first.getName());
    }
    std::sort(v.begin(), v.end());
    return v;
}


bool
Configuration::has(const PropertyName &name) const
{
    const_iterator i = find(name);
    return (i != end());
}
    

std::string
Configuration::toXmlString() const
{
    using std::endl;
    std::stringstream config;

    // This simple implementation just assumes everything's a string.
    // Override it if you want something fancier (or reimplement it to
    // support the whole gamut -- the reader in rosexmlhandler.cpp
    // already can)

    for (const_iterator i = begin(); i != end(); ++i) {
        config <<  "<property name=\""
	       << encode(i->first.getName()) << "\" value=\""
	       << encode(get<String>(i->first)) << "\"/>" << endl;
    }

    config << endl;

    return config.str();
}

Configuration&
Configuration::operator=(const Configuration &conf)
{
    clear();

    // Copy everything
    //
    for (const_iterator i = conf.begin(); i != conf.end(); ++i)
        insert(PropertyPair(i->first, i->second->clone()));

    return (*this);
}



namespace CompositionMetadataKeys
{
    const PropertyName Copyright = "copyright";
    const PropertyName Composer = "composer";
    const PropertyName Title = "title";
    const PropertyName Subtitle = "subtitle";
    const PropertyName Arranger = "arranger";
    // The following are recognized only by LilyPond output
    const PropertyName Dedication = "dedication";
    const PropertyName Subsubtitle = "subsubtitle";
    const PropertyName Poet = "poet";
    const PropertyName Meter = "meter";
    const PropertyName Opus = "opus";
    const PropertyName Instrument = "instrument";
    const PropertyName Piece = "piece";
    const PropertyName Tagline = "tagline";

    // The tab order of the edit fields in HeadersConfigurationPage
    // is defined by the creation order of the edit fields.
    // The edit fields are created in the order of the keys in getFixedKeys().
    std::vector<PropertyName> getFixedKeys() {
	std::vector<PropertyName> keys;
	keys.push_back(Dedication);
	keys.push_back(Title);
	keys.push_back(Subtitle);
	keys.push_back(Subsubtitle);
	keys.push_back(Poet);
	keys.push_back(Instrument);
	keys.push_back(Composer);
	keys.push_back(Meter);
	keys.push_back(Arranger);
	keys.push_back(Piece);
	keys.push_back(Opus);
	keys.push_back(Copyright);
	keys.push_back(Tagline);

	return keys;
    }
}


// Keep these in lower case
const PropertyName DocumentConfiguration::SequencerOptions      = "sequenceroptions";
const PropertyName DocumentConfiguration::ZoomLevel             = "zoomlevel";
const PropertyName DocumentConfiguration::TransportMode         = "transportmode";


DocumentConfiguration::DocumentConfiguration()
{
    set<Int>(ZoomLevel, 0);
    set<String>(TransportMode, ""); // apparently generates an exception if not initialized
}
    
DocumentConfiguration::DocumentConfiguration(const DocumentConfiguration &conf):
    Configuration()
{
    for (const_iterator i = conf.begin(); i != conf.end(); ++i)
        insert(PropertyPair(i->first, i->second->clone()));
}

DocumentConfiguration::~DocumentConfiguration()
{
    clear();
}


DocumentConfiguration&
DocumentConfiguration::operator=(const DocumentConfiguration &conf)
{
    clear();

    for (const_iterator i = conf.begin(); i != conf.end(); ++i)
        insert(PropertyPair(i->first, i->second->clone()));

    return *this;
}


// Convert to XML string for export
//
std::string
DocumentConfiguration::toXmlString() const
{
    using std::endl;

    std::stringstream config;

    config << endl << "<configuration>" << endl;

    config << "    <" << ZoomLevel << " type=\"Int\">" << get<Int>(ZoomLevel)
           << "</" << ZoomLevel << ">\n";

    config << "    <" << TransportMode << " type=\"String\">" << get<String>(TransportMode)
           << "</" << TransportMode << ">\n";

    config << "</configuration>" << endl;

    config << endl;

    return config.str();
}

}


