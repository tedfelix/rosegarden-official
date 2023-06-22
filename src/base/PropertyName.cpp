/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2023 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "base/PropertyName.h"
#include "base/Exception.h"

#include <QtGlobal>  // For Q_ASSERT()

#include <iostream>
#include <map>


namespace Rosegarden 
{


namespace
{
    typedef std::map<std::string, int> NameToValueMap;
    // Pointer for create on first use to avoid static init order fiasco.
    // Note: This is a deliberate memory leak since we cannot be sure
    //       who might access this as we are going down.
    NameToValueMap *a_nameToValueMap = nullptr;

    typedef std::map<int, std::string> ValueToNameMap;
    // Pointer for create on first use to avoid static init order fiasco.
    // Note: This is a deliberate memory leak since we cannot be sure
    //       who might access this as we are going down.
    ValueToNameMap *a_valueToNameMap = nullptr;

    int a_nextValue = 0;

    // Get the existing hash value for a name, or if not found, create
    // a new hash value and add to the maps.
    int a_getValue(const std::string &s)
    {
        if (!a_nameToValueMap) {
            // Create on first use to avoid static init order fiasco.
            a_nameToValueMap = new NameToValueMap;
            a_valueToNameMap = new ValueToNameMap;
        }
    
        NameToValueMap::iterator i(a_nameToValueMap->find(s));

        if (i != a_nameToValueMap->end()) {
            return i->second;
        } else {
            int nv = ++a_nextValue;
            a_nameToValueMap->insert(NameToValueMap::value_type(s, nv));
            a_valueToNameMap->insert(ValueToNameMap::value_type(nv, s));
            return nv;
        }
    }
}


PropertyName::PropertyName(const char *cs)
{
    std::string s(cs);
    m_value = a_getValue(s);
}

PropertyName::PropertyName(const std::string &s) :
    m_value(a_getValue(s))
{
}

PropertyName &PropertyName::operator=(const char *cs)
{
    std::string s(cs);
    m_value = a_getValue(s);
    return *this;
}

PropertyName &PropertyName::operator=(const std::string &s)
{
    m_value = a_getValue(s);
    return *this;
}

std::string PropertyName::getName() const
{
    // ??? Why not cache the name in a member variable and get rid of
    //     a_valueToNameMap and this slow search?  And the exception and
    //     the error logging...  return m_name;
    ValueToNameMap::iterator i(a_valueToNameMap->find(m_value));
    if (i != a_valueToNameMap->end())
        return i->second;

    // dump some informative data, even if we aren't in debug mode,
    // because this really shouldn't be happening
    std::cerr << "ERROR: PropertyName::getName: value corrupted!\n";
    std::cerr << "PropertyName's internal value is " << m_value << std::endl;
    std::cerr << "Reverse interns are ";

    i = a_valueToNameMap->begin();
    if (i == a_valueToNameMap->end()) {
        std::cerr << "(none)";
    } else {
        while (i != a_valueToNameMap->end()) {
            if (i != a_valueToNameMap->begin()) {
                std::cerr << ", ";
            }
            std::cerr << i->first << "=" << i->second;
            ++i;
        }
    }

    std::cerr << std::endl;

    Q_ASSERT(0); // exit if debug is on
    throw Exception(
            "Serious problem in PropertyName::getName(): property "
            "name's internal value is corrupted -- see stderr for details");
}


}

