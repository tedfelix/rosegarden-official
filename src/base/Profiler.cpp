/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
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
#include <set>
#include <map>

#include <stdio.h>

using std::cerr;
using std::endl;

namespace Rosegarden {

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

void Profiles::accumulate(
#ifndef NO_TIMING
    const char* id, clock_t time, RealTime rt
#else
    const char*, clock_t, RealTime
#endif
)
{
#ifndef NO_TIMING    
    ProfilePair &pair(m_profiles[id]);
    ++pair.first;
    pair.second.first += time;
    pair.second.second = pair.second.second + rt;

    TimePair &lastPair(m_lastCalls[id]);
    lastPair.first = time;
    lastPair.second = rt;

    TimePair &worstPair(m_worstCalls[id]);
    if (time > worstPair.first) {
        worstPair.first = time;
    }
    if (rt > worstPair.second) {
        worstPair.second = rt;
    }
#endif
}

void Profiles::dump() const
{
#ifndef NO_TIMING

    fprintf(stderr, "Profiling points:\n");

    fprintf(stderr, "\nBy name:\n");

    typedef std::set<const char *, std::less<std::string> > StringSet;

    StringSet profileNames;
    for (ProfileMap::const_iterator i = m_profiles.begin();
         i != m_profiles.end(); ++i) {
        profileNames.insert(i->first);
    }

    for (StringSet::const_iterator i = profileNames.begin();
         i != profileNames.end(); ++i) {

        ProfileMap::const_iterator j = m_profiles.find(*i);

        if (j == m_profiles.end()) continue;

        const ProfilePair &pp(j->second);

        fprintf(stderr, "%s(%d):\n", *i, pp.first);

        fprintf(stderr, "\tCPU:  \t%.9g ms/call \t[%d ms total]\n",
                (((double)pp.second.first * 1000.0 /
		  (double)pp.first) / CLOCKS_PER_SEC),
                int((pp.second.first * 1000.0) / CLOCKS_PER_SEC));

        fprintf(stderr, "\tReal: \t%s ms      \t[%s ms total]\n",
                ((pp.second.second / pp.first) * 1000).toString().c_str(),
                (pp.second.second * 1000).toString().c_str());

        WorstCallMap::const_iterator k = m_worstCalls.find(*i);
        if (k == m_worstCalls.end()) continue;
        
        const TimePair &wc(k->second);

        fprintf(stderr, "\tWorst:\t%s ms/call \t[%d ms CPU]\n",
                (wc.second * 1000).toString().c_str(),
                int((wc.first * 1000.0) / CLOCKS_PER_SEC));
    }

    typedef std::multimap<RealTime, const char *> TimeRMap;
    typedef std::multimap<int, const char *> IntRMap;
    
    TimeRMap totmap, avgmap, worstmap;
    IntRMap ncallmap;

    for (ProfileMap::const_iterator i = m_profiles.begin();
         i != m_profiles.end(); ++i) {
        totmap.insert(TimeRMap::value_type(i->second.second.second, i->first));
        avgmap.insert(TimeRMap::value_type(i->second.second.second /
                                           i->second.first, i->first));
        ncallmap.insert(IntRMap::value_type(i->second.first, i->first));
    }

    for (WorstCallMap::const_iterator i = m_worstCalls.begin();
         i != m_worstCalls.end(); ++i) {
        worstmap.insert(TimeRMap::value_type(i->second.second,
                                             i->first));
    }


    fprintf(stderr, "\nBy total:\n");
    for (TimeRMap::const_iterator i = totmap.end(); i != totmap.begin(); ) {
        --i;
        fprintf(stderr, "%-40s  %s ms\n", i->second,
                (i->first * 1000).toString().c_str());
    }

    fprintf(stderr, "\nBy average:\n");
    for (TimeRMap::const_iterator i = avgmap.end(); i != avgmap.begin(); ) {
        --i;
        fprintf(stderr, "%-40s  %s ms\n", i->second,
                (i->first * 1000).toString().c_str());
    }

    fprintf(stderr, "\nBy worst case:\n");
    for (TimeRMap::const_iterator i = worstmap.end(); i != worstmap.begin(); ) {
        --i;
        fprintf(stderr, "%-40s  %s ms\n", i->second,
                (i->first * 1000).toString().c_str());
    }

    fprintf(stderr, "\nBy number of calls:\n");
    for (IntRMap::const_iterator i = ncallmap.end(); i != ncallmap.begin(); ) {
        --i;
        fprintf(stderr, "%-40s  %d\n", i->second, i->first);
    }

#endif
}

#ifndef NO_TIMING    

Profiler::Profiler(const char* c, bool showOnDestruct) :
    m_c(c),
    m_showOnDestruct(showOnDestruct),
    m_ended(false)
{
    m_startCPU = clock();

    struct timeval tv;
    (void)gettimeofday(&tv, 0);
    m_startTime = RealTime::fromTimeval(tv);
}

void
Profiler::update() const
{
    clock_t elapsedCPU = clock() - m_startCPU;

    struct timeval tv;
    (void)gettimeofday(&tv, 0);
    RealTime elapsedTime = RealTime::fromTimeval(tv) - m_startTime;

    cerr << "Profiler : id = " << m_c
	 << " - elapsed so far = " << ((elapsedCPU * 1000) / CLOCKS_PER_SEC)
	 << "ms CPU, " << elapsedTime << " real" << endl;
}    

Profiler::~Profiler()
{
    if (!m_ended) end();
}

void
Profiler::end()
{
    clock_t elapsedCPU = clock() - m_startCPU;

    struct timeval tv;
    (void)gettimeofday(&tv, 0);
    RealTime elapsedTime = RealTime::fromTimeval(tv) - m_startTime;

    Profiles::getInstance()->accumulate(m_c, elapsedCPU, elapsedTime);

    if (m_showOnDestruct)
        cerr << "Profiler : id = " << m_c
             << " - elapsed = " << ((elapsedCPU * 1000) / CLOCKS_PER_SEC)
	     << "ms CPU, " << elapsedTime << " real" << endl;

    m_ended = true;
}
 
#endif

}

