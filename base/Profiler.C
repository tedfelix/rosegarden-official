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
#ifndef NO_TIMING    
    ProfilePair &pair(m_profiles[id]);
    ++pair.first;
    pair.second += time;
#endif
}

void Profiles::dump()
{
#ifndef NO_TIMING    
    cerr << "Profiles::dump() :\n";

    // I'm finding these two confusing dumped out in random order,
    // so I'm going to sort them alphabetically:

    std::vector<const char *> profileNames;
    for (ProfileMap::iterator i = m_profiles.begin();
	 i != m_profiles.end(); ++i) {
	profileNames.push_back((*i).first);
    }

    std::sort(profileNames.begin(), profileNames.end());

    for (std::vector<const char *>::iterator i = profileNames.begin();
	 i != profileNames.end(); ++i) {

        cerr << "-> " << *i << ": " 
	     << m_profiles[*i].first << " calls, "
	     << int((m_profiles[*i].second * 1000.0) / CLOCKS_PER_SEC) << "ms, "
	     << (((double)m_profiles[*i].second * 1000000.0 /
		  (double)m_profiles[*i].first) / CLOCKS_PER_SEC) << "us/call"
	     << endl;
    }

    cerr << "Profiles::dump() finished\n";
#endif
}

Profiler::Profiler(const char* c, bool showOnDestruct)
    : m_c(c),
      m_showOnDestruct(showOnDestruct)
{
#ifndef NO_TIMING
    m_startTime = clock();
#endif
}

void
Profiler::update()
{
#ifndef NO_TIMING
    clock_t elapsedTime = clock() - m_startTime;

    cerr << "Profiler : id = " << m_c
	 << " - elapsed so far = " << ((elapsedTime * 1000) / CLOCKS_PER_SEC)
	 << "ms" << endl;
#endif
}    

Profiler::~Profiler()
{
#ifndef NO_TIMING
    clock_t elapsedTime = clock() - m_startTime;

    Profiles::getInstance()->accumulate(m_c, elapsedTime);

    if (m_showOnDestruct)
        cerr << "Profiler : id = " << m_c
             << " - elapsed = " << ((elapsedTime * 1000) / CLOCKS_PER_SEC)
	     << "ms" << endl;
#endif
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
