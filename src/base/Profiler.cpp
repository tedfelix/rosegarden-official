/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[Profiler]"

#include "Profiler.h"

// Rosegarden
#include "misc/Debug.h"

// C++
#include <algorithm>
#include <set>

// C
#include <math.h>


namespace Rosegarden {


Profiles* Profiles::m_instance = nullptr;

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

    qDebug("----------------------------------------------------");
    qDebug("Profiling points:");
    qDebug(" ");
    qDebug("By name:");

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

        qDebug("%s(%d):", *i, pp.first);

        qDebug("    CPU:    %.9f ms/call  (%ld ms total)",
                (((double)pp.second.first * 1000.0 / (double)pp.first) / CLOCKS_PER_SEC),
                lround((pp.second.first * 1000.0) / CLOCKS_PER_SEC));

        qDebug("    Real:   %s ms  (%s ms total)",
                (pp.second.second / pp.first * 1000).toString().c_str(),
                (pp.second.second * 1000).toString().c_str());

        WorstCallMap::const_iterator k = m_worstCalls.find(*i);
        if (k == m_worstCalls.end()) continue;

        const TimePair &wc(k->second);

        qDebug("    Worst:  %s ms/call  (%d ms CPU)",
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

    // By Total
    qDebug(" ");
    qDebug("By total:");

    for (TimeRMap::const_iterator i = totmap.end(); i != totmap.begin(); ) {
        --i;
        qDebug("    %-40s  %s ms",
                i->second,
                (i->first * 1000).toString().c_str());
    }

    // By Average
    qDebug(" ");
    qDebug("By average:");

    for (TimeRMap::const_iterator i = avgmap.end(); i != avgmap.begin(); ) {
        --i;
        qDebug("    %-40s  %s ms",
                i->second,
                (i->first * 1000).toString().c_str());
    }

    // By Worst Case
    qDebug(" ");
    qDebug("By worst case:");

    for (TimeRMap::const_iterator i = worstmap.end(); i != worstmap.begin(); ) {
        --i;
        qDebug("    %-40s  %s ms",
                i->second,
                (i->first * 1000).toString().c_str());
    }

    // By Number of Calls
    qDebug(" ");
    qDebug("By number of calls:");

    for (IntRMap::const_iterator i = ncallmap.end(); i != ncallmap.begin(); ) {
        --i;
        qDebug("    %-40s  %d", i->second, i->first);
    }

#endif
}

#ifndef NO_TIMING

Profiler::Profiler(const char* name, bool showOnDestruct) :
    m_c(name),
    m_showOnDestruct(showOnDestruct),
    m_ended(false)
{
    m_startCPU = clock();

    struct timeval tv;
    (void)gettimeofday(&tv, nullptr);
    m_startTime = RealTime::fromTimeval(tv);
}

#if 0
void
Profiler::update() const
{
    clock_t elapsedCPU = clock() - m_startCPU;

    struct timeval tv;
    (void)gettimeofday(&tv, nullptr);
    RealTime elapsedTime = RealTime::fromTimeval(tv) - m_startTime;

    RG_DEBUG << "update() : id = " << m_c
        << " - elapsed so far = " << ((elapsedCPU * 1000) / CLOCKS_PER_SEC)
        << "ms CPU, " << elapsedTime << " real";
}
#endif

Profiler::~Profiler()
{
    if (!m_ended)
        end();
}

void
Profiler::end()
{
    clock_t elapsedCPU = clock() - m_startCPU;

    struct timeval tv;
    (void)gettimeofday(&tv, nullptr);
    RealTime elapsedTime = RealTime::fromTimeval(tv) - m_startTime;

    Profiles::getInstance()->accumulate(m_c, elapsedCPU, elapsedTime);

    if (m_showOnDestruct)
        RG_DEBUG << "end() : id = " << m_c
             << " - elapsed = " << ((elapsedCPU * 1000) / CLOCKS_PER_SEC)
             << "ms CPU, " << elapsedTime << " real";

    m_ended = true;
}

#endif

}
