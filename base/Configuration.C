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


// Class to hold extraneous bits of configuration which
// don't sit inside the Composition itself - sequencer
// and other general stuff that we want to keep separate.
//
//

#include <string>
#include <algorithm>

#include "Configuration.h"

#if (__GNUC__ < 3)
#include <strstream>
#define stringstream strstream
#else
#include <sstream>
#endif


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
Configuration::toXmlString()
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

#if (__GNUC__ < 3)
    config << endl << std::ends;
#else
    config << endl;
#endif

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
    const PropertyName Notes = "notes";
}


// Keep these in lower case
const PropertyName DocumentConfiguration::MetronomeDevice        = "metronomedevice";
const PropertyName DocumentConfiguration::FetchLatency          = "fetchlatency";
const PropertyName DocumentConfiguration::SequencerOptions      = "sequenceroptions";
const PropertyName DocumentConfiguration::ZoomLevel             = "zoomlevel";


DocumentConfiguration::DocumentConfiguration()
{
    set<Int>(MetronomeDevice, 0);
    set<RealTimeT>(FetchLatency,      RealTime(0, 50000));    
    set<Int>(ZoomLevel, 0);
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
DocumentConfiguration::toXmlString()
{
    using std::endl;

    std::stringstream config;

    config << endl << "<configuration>" << endl
           << "    <" << MetronomeDevice        << " type=\"Int\">" << get<Int>(MetronomeDevice)        << "</" << MetronomeDevice        << ">\n";

    RealTime r = get<RealTimeT>(FetchLatency);
    
    config << "    <" << FetchLatency << " type=\"RealTime\">" << r.sec << "," << r.usec << "</" << FetchLatency << ">" << endl;

    config << "    <" << ZoomLevel << " type=\"Int\">" << get<Int>(ZoomLevel)
           << "</" << ZoomLevel << ">\n";

    config << "</configuration>" << endl;

#if (__GNUC__ < 3)
    config << endl << std::ends;
#else
    config << endl;
#endif

    return config.str();
}

}


