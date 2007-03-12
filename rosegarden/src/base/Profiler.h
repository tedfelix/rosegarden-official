// -*- c-basic-offset: 4 -*-


/*
    Rosegarden
    A sequencer and musical notation editor.

    This program is Copyright 2000-2007
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

#ifndef _PROFILER_H_
#define _PROFILER_H_

#include <ctime>
#include <sys/time.h>
#include <map>

#include "RealTime.h"


namespace Rosegarden 
{

/**
 * Profiling classes
 */

/**
 * The class holding all profiling data
 *
 * This class is a singleton
 */
class Profiles
{
public:
    static Profiles* getInstance();
    ~Profiles();

    void accumulate(const char* id, clock_t time, RealTime rt);
    void dump();

protected:
    Profiles();

    typedef std::pair<clock_t, RealTime> TimePair;
    typedef std::pair<int, TimePair> ProfilePair;
    typedef std::map<const char *, ProfilePair> ProfileMap;
    typedef std::map<const char *, TimePair> LastCallMap;
    ProfileMap m_profiles;
    LastCallMap m_lastCalls;

    static Profiles* m_instance;
};

class Profiler
{
public:
    Profiler(const char*, bool showOnDestruct = false);
    ~Profiler();

    void update();

protected:
    const char* m_c;
    clock_t m_startCPU;
    RealTime m_startTime;
    bool m_showOnDestruct;
};
 
}

#endif
