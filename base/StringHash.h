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

#ifndef _STRING_HASH_H_
#define _STRING_HASH_H_

#include <string>

#if (__GNUC__ < 3)

#include <hash_map>
#define __HASH_NS std

#else

#include <ext/hash_map>
#if (__GNUC_MINOR__ >= 1)
#define __HASH_NS __gnu_cxx
#else
#define __HASH_NS std
#endif

#endif

namespace Rosegarden 
{

struct eqstring {
    bool operator() (const std::string &s1, const std::string &s2) const {
        return s1 == s2;
    }
};

struct hashstring {
    static __HASH_NS::hash<const char *> _H;
    size_t operator() (const std::string &s) const { return _H(s.c_str()); }
};

template <typename T>
class hash_char : public __HASH_NS::hash_map<const char*, T>
{
};

template <typename T>
class hash_string : public __HASH_NS::hash_map<std::string, T, hashstring, eqstring>
{
};


// #if !defined(__GNUC__) || __GNUC__ < 3
// __HASH_NS::hash<const char *> hashstring::_H;
// #endif

 
}


#endif

