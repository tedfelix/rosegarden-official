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

Configuration::Configuration():
        m_playLatency(0, 100000),     // the sequencer's head start
        m_fetchLatency(0, 50000),     // to fetch and queue new events
        m_readAhead(0, 40000),        // how many events to fetch
        m_metronomePitch(37),
        m_metronomeBarVelocity(120),
        m_metronomeBeatVelocity(80),
        m_metronomeDuration(0, 10000),
        m_client(NotationView)
{
}

Configuration::~Configuration()
{
}


// Convert to XML string for export
//
std::string
Configuration::toXmlString()
{
    stringstream config;

    config << std::endl;
    config << "<configuration>" << std::endl;
    config << "</configuration>" << std::endl << std::ends;

    return config.str();
}

}


