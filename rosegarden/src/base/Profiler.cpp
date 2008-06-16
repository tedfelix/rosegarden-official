// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <iostream>
#include "Profiler.h"

#include <vector>
#include <algorithm>

//#define NO_TIMING 1

#ifdef NDEBUG
#define NO_TIMING 1
#endif

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

void Profiles::accumulate(const char* id, clock_t time, RealTime rt)
{
#ifndef NO_TIMING    
    ProfilePair &pair(m_profiles[id]);
    ++pair.first;
    pair.second.first += time;
    pair.second.second = pair.second.second + rt;

    TimePair &timePair(m_lastCalls[id]);
    timePair.first = time;
    timePair.second = rt;
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

        cerr << "-> " << *i << ":  CPU: " 
	     << m_profiles[*i].first << " calls, "
	     << int((m_profiles[*i].second.first * 1000.0) / CLOCKS_PER_SEC) << "ms, "
	     << (((double)m_profiles[*i].second.first * 1000000.0 /
		  (double)m_profiles[*i].first) / CLOCKS_PER_SEC) << "us/call"
	     << endl;

        cerr << "-> " << *i << ": real: " 
	     << m_profiles[*i].first << " calls, "
	     << m_profiles[*i].second.second << ", "
	     << (m_profiles[*i].second.second / m_profiles[*i].first)
	     << "/call"
	     << endl;

	cerr << "-> " << *i << ": last:  CPU: "
	     << int((m_lastCalls[*i].first * 1000.0) / CLOCKS_PER_SEC) << "ms, "
	     << "   real: "
	     << m_lastCalls[*i].second << endl;
    }

    cerr << "Profiles::dump() finished\n";
#endif
}

Profiler::Profiler(const char* c, bool showOnDestruct)
    : m_c(c),
      m_showOnDestruct(showOnDestruct)
{
#ifndef NO_TIMING
    m_startCPU = clock();

    struct timeval tv;
    (void)gettimeofday(&tv, 0);
    m_startTime = RealTime(tv.tv_sec, tv.tv_usec * 1000);
#endif
}

void
Profiler::update()
{
#ifndef NO_TIMING
    clock_t elapsedCPU = clock() - m_startCPU;

    struct timeval tv;
    (void)gettimeofday(&tv, 0);
    RealTime elapsedTime = RealTime(tv.tv_sec, tv.tv_usec * 1000) - m_startTime;

    cerr << "Profiler : id = " << m_c
	 << " - elapsed so far = " << ((elapsedCPU * 1000) / CLOCKS_PER_SEC)
	 << "ms CPU, " << elapsedTime << " real" << endl;
#endif
}    

Profiler::~Profiler()
{
#ifndef NO_TIMING
    clock_t elapsedCPU = clock() - m_startCPU;

    struct timeval tv;
    (void)gettimeofday(&tv, 0);
    RealTime elapsedTime = RealTime(tv.tv_sec, tv.tv_usec * 1000) - m_startTime;

    Profiles::getInstance()->accumulate(m_c, elapsedCPU, elapsedTime);

    if (m_showOnDestruct)
        cerr << "Profiler : id = " << m_c
             << " - elapsed = " << ((elapsedCPU * 1000) / CLOCKS_PER_SEC)
	     << "ms CPU, " << elapsedTime << " real" << endl;
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
