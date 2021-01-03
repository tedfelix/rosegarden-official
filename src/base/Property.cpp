/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */


/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Property.h"
#include <cstdio>
#include <cstdlib>
#include <string>

namespace Rosegarden 
{
using std::string;

string
PropertyDefn<Int>::typeName()
{
    return "Int";
}    

PropertyDefn<Int>::basic_type
PropertyDefn<Int>::parse(string s)
{
    return atoi(s.c_str());
}

string
PropertyDefn<Int>::unparse(PropertyDefn<Int>::basic_type i)
{
    static char buffer[20]; sprintf(buffer, "%ld", i);
    return buffer;
}

string
PropertyDefn<String>::typeName()
{
    return "String";
}    

PropertyDefn<String>::basic_type
PropertyDefn<String>::parse(string s)
{
    return s;
}

string
PropertyDefn<String>::unparse(PropertyDefn<String>::basic_type i)
{
    return i;
}

string
PropertyDefn<Bool>::typeName()
{
    return "Bool";
}    

PropertyDefn<Bool>::basic_type
PropertyDefn<Bool>::parse(string s)
{
    return s == "true";
}

string
PropertyDefn<Bool>::unparse(PropertyDefn<Bool>::basic_type i)
{
    return (i ? "true" : "false");
}

string
PropertyDefn<RealTimeT>::typeName()
{
    return "RealTimeT";
}    

PropertyDefn<RealTimeT>::basic_type
PropertyDefn<RealTimeT>::parse(string s)
{
    string sec = s.substr(0, s.find('/')),
          nsec = s.substr(s.find('/') + 1);

    return RealTime(atoi(sec.c_str()), atoi(nsec.c_str()));
}

string
PropertyDefn<RealTimeT>::unparse(PropertyDefn<RealTimeT>::basic_type i)
{
    static char buffer[256]; sprintf(buffer, "%d/%d", i.sec, i.nsec);
    return buffer;
}

PropertyStoreBase::~PropertyStoreBase()
{
}

template <>
size_t
PropertyStore<Int>::getStorageSize() const
{
    return sizeof(*this);
}

template <>
size_t
PropertyStore<String>::getStorageSize() const
{
    return sizeof(*this) + m_data.size();
}

template <>
size_t
PropertyStore<Bool>::getStorageSize() const
{
    return sizeof(*this);
}

template <>
size_t
PropertyStore<RealTimeT>::getStorageSize() const
{
    return sizeof(*this);
}
 
}

