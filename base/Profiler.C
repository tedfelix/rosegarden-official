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

#include <iostream>

#include "Profiler.h"

using std::cerr;
using std::endl;

namespace Rosegarden 
{

Profiles* Profiles::m_instance = 0;

Profiles* Profiles::getInstance()
{
    if (!m_instance) m_instance = new Profiles();
    
    return m_instance;
}

Profiles::Profiles()
{
}

Profiles::~Profiles()
{
    dump();
}

void Profiles::accumulate(const char* id, clock_t time)
{
//     cerr << "Profiles::accumulate() : "
//               << id << " += " << time << endl;

    m_profiles[id] += time;
}

void Profiles::dump()
{
    cerr << "Profiles::dump() :\n";

    for(profilesmap::iterator i = m_profiles.begin();
        i != m_profiles.end(); ++i) {

        cerr << (*i).first << " : " << (*i).second << endl;
    }
    
    cerr << "Profiles::dump() finished\n";
}

struct tms Profiler::m_spare;

Profiler::Profiler(const char* c, bool showOnDestruct)
    : m_c(c),
      m_showOnDestruct(showOnDestruct)
{
//     cerr << "Profiler" << c << endl;

    m_startTime = times(&m_spare);
}

Profiler::~Profiler()
{
    clock_t elapsedTime = times(&m_spare) - m_startTime;

    Profiles::getInstance()->accumulate(m_c, elapsedTime);

    if (m_showOnDestruct)
        cerr << "Profiler : id = " << m_c
             << " - elapsed = " << elapsedTime << endl;
}
 
}

/* A little usage demo

int main()
{
    {
        Profiler foo("test");
        sleep(1);
    }

    {
        Profiler foo("test");
        sleep(1);
    }

    {
        Profiler foo("test2");
        sleep(1);
    }
    
    Profiles::getInstance()->dump();

    return 0;
}
*/
