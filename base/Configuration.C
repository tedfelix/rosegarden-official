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

#include "Configuration.h"

#if (__GNUC__ < 3)
#include <strstream>
#define stringstream strstream
#else
#include <sstream>
#endif


namespace Rosegarden
{

Configuration::Configuration()
{
    set<Int>("metronomepitch", 37);
    set<Int>("metronomebarvelocity", 120);
    set<Int>("metronomebeatvelocity", 80);
    set<RealTimeT>("fetchlatency",      RealTime(0, 50000));    
    set<RealTimeT>("metronomeduration", RealTime(0, 10000));    
}

Configuration::~Configuration()
{
}


// Convert to XML string for export
//
std::string
Configuration::toXmlString()
{
    using std::endl;

    std::stringstream config;

    config << endl
           << "<configuration>" << endl
    
           << "<metronomepitch>"        << get<Int>("metronomepitch") << "</metronomepitch>" << endl
           << "<metronomebarvelocity>"  << get<Int>("metronomebarvelocity") << "</metronomebarvelocity>" << endl
           << "<metronomebeatvelocity>" << get<Int>("metronomebeatvelocity") << "</metronomebeatvelocity>" << endl;

    RealTime r = get<RealTimeT>("fetchlatency");
    
    config << "<fetchlatency>" << r.sec << "," << r.usec << "</fetchlatency>" << endl;

    r =  get<RealTimeT>("metronomeduration");

    config << "<metronomeduration>" << r.sec << "," << r.usec << "</metronomeduration>" << endl;

    config << "</configuration>" << endl << std::ends;

    return config.str();
}

}


