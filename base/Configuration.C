// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
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

// Keep these in lower case
const char* const Configuration::MetronomePitch        = "metronomepitch";
const char* const Configuration::MetronomeBarVelocity  = "metronomebarvelocity";
const char* const Configuration::MetronomeBeatVelocity = "metronomebeatvelocity";
const char* const Configuration::FetchLatency          = "fetchlatency";
const char* const Configuration::MetronomeDuration     = "metronomeduration";
const char* const Configuration::SequencerOptions      = "sequenceroptions";


Configuration::Configuration()
{
    set<Int>(MetronomePitch, 37);
    set<Int>(MetronomeBarVelocity, 120);
    set<Int>(MetronomeBeatVelocity, 80);
    set<RealTimeT>(FetchLatency,      RealTime(0, 50000));    
    set<RealTimeT>(MetronomeDuration, RealTime(0, 10000));    
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
    
           << "<" << MetronomePitch        << " type=\"Int\">" << get<Int>(MetronomePitch)        << "</" << MetronomePitch        << ">\n"
           << "<" << MetronomeBarVelocity  << " type=\"Int\">" << get<Int>(MetronomeBarVelocity)  << "</" << MetronomeBarVelocity  << ">\n"
           << "<" << MetronomeBeatVelocity << " type=\"Int\">" << get<Int>(MetronomeBeatVelocity) << "</" << MetronomeBeatVelocity << ">\n";

    RealTime r = get<RealTimeT>(FetchLatency);
    
    config << "<" << FetchLatency << " type=\"RealTime\">" << r.sec << "," << r.usec << "</" << FetchLatency << ">" << endl;

    r =  get<RealTimeT>(MetronomeDuration);

    config << "<" << MetronomeDuration << " type=\"RealTime\">" << r.sec << "," << r.usec << "</" << MetronomeDuration << ">" << endl;

#if (__GNUC__ < 3)
    config << "</configuration>" << endl << std::ends;
#else
    config << "</configuration>" << endl;
#endif

    return config.str();
}

}


