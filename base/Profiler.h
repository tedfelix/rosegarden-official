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

#ifndef _PROFILER_H_
#define _PROFILER_H_

#include <string>
#include <iostream>

#include <ext/hash_map>

#include <sys/times.h>
#include <unistd.h>

namespace Rosegarden 
{

/**
 * Profiling classes
 */

// Silly stuff to declare a hash<char*,clock_t>
//
struct StringEq
{
    bool operator()(const char* a, const char* b) const
    {
        return (strcmp(a, b) == 0);
    }
};

struct StringHash
{
    size_t operator() (const char* s) const {
        int l = strlen(s);
        if (l == 0) return 0;
        if (l == 1) return s[0];
        if (l > 1) return s[0] + s[1];
    }
};

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

    typedef std::hash_map<const char*, clock_t, StringHash, StringEq> profilesmap;
    

protected:
    Profiles();

    profilesmap m_profiles;

    static Profiles* m_instance;
};

class Profiler
{
public:
    Profiler(const char*);
    ~Profiler();

protected:
    const char* m_c;
    clock_t m_startTime;

    static struct tms m_spare;
};
 
}

#endif
