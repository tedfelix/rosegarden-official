/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
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
    typedef std::map<std::string, int> NameToIDMap;
    // Pointer for create on first use to avoid static init order fiasco.
    // Note: This is a deliberate memory leak since we cannot be sure
    //       who might access this as we are going down.
    NameToIDMap *a_nameToIDMap = nullptr;

    typedef std::map<int, std::string> IDToNameMap;
    // This map allows us to save space by not storing the name in every
    // copy of a PropertyName.  Used by getName().
    // Pointer for create on first use to avoid static init order fiasco.
    // Note: This is a deliberate memory leak since we cannot be sure
    //       who might access this as we are going down.
    IDToNameMap *a_idToNameMap = nullptr;

    int a_nextId = 0;

    // Get the existing ID for a name, or if not found, create
    // a new ID and add to the map.
    int a_getId(const std::string &name)
    {
        if (!a_nameToIDMap) {
            // Create on first use to avoid static init order fiasco.
            a_nameToIDMap = new NameToIDMap;
            a_idToNameMap = new IDToNameMap;
        }

        NameToIDMap::iterator idIter(a_nameToIDMap->find(name));
        // Found it?  Return it.
        if (idIter != a_nameToIDMap->end())
            return idIter->second;

        // Not found.  Create a new ID.

        const int newId = ++a_nextId;
        a_nameToIDMap->insert(NameToIDMap::value_type(name, newId));
        a_idToNameMap->insert(IDToNameMap::value_type(newId, name));
        return newId;
    }
}


PropertyName::PropertyName(const char *name)
{
    m_id = a_getId(name);
}

PropertyName::PropertyName(const std::string &name)
{
    m_id = a_getId(name);
}

PropertyName &PropertyName::operator=(const char *name)
{
    m_id = a_getId(name);

    return *this;
}

PropertyName &PropertyName::operator=(const std::string &name)
{
    m_id = a_getId(name);

    return *this;
}

std::string PropertyName::getName() const
{
    IDToNameMap::iterator i(a_idToNameMap->find(m_id));
    // Not found?  Return the empty string.
    if (i == a_idToNameMap->end())
        return "";

    return i->second;

#if 0
    // dump some informative data, even if we aren't in debug mode,
    // because this really shouldn't be happening
    std::cerr << "ERROR: PropertyName::getName: ID corrupted!\n";
    std::cerr << "PropertyName's internal ID is " << m_id << std::endl;
    std::cerr << "Reverse interns are ";

    i = a_idToNameMap->begin();
    if (i == a_idToNameMap->end()) {
        std::cerr << "(none)";
    } else {
        while (i != a_idToNameMap->end()) {
            if (i != a_idToNameMap->begin()) {
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
            "name's internal ID is corrupted -- see stderr for details");
#endif
}


}
