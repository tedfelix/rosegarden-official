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

#ifndef _PROFILER_H_
#define _PROFILER_H_

#include <ctime>

#include "StringHash.h"


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

    void accumulate(const char* id, clock_t time);

    void dump();

    typedef hash_char<clock_t> profilesmap;
    

protected:
    Profiles();

    profilesmap m_profiles;

    static Profiles* m_instance;
};

class Profiler
{
public:
    Profiler(const char*, bool showOnDestruct=false);
    ~Profiler();

protected:
    const char* m_c;
    clock_t m_startTime;
    bool m_showOnDestruct;
};
 
}

#endif
