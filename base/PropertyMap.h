/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#ifndef _PROPERTY_MAP_H_
#define _PROPERTY_MAP_H_

#include "Property.h"
#include "PropertyName.h"

#ifdef PROPERTY_MAP_IS_HASH_MAP

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

#else

#include <map>

#endif

namespace Rosegarden {

#ifdef PROPERTY_MAP_IS_HASH_MAP

class PropertyMap : public __HASH_NS::hash_map<PropertyName,
					       PropertyStoreBase *,
					       PropertyNameHash,
					       PropertyNamesEqual>

#else

class PropertyMap : public std::map<PropertyName, PropertyStoreBase *>

#endif

{
public:
    PropertyMap()
#ifdef PROPERTY_MAP_IS_HASH_MAP
	;
#else
	{ }
#endif
    PropertyMap(const PropertyMap &pm);

    ~PropertyMap();
    
    void clear();
    
    std::string toXmlString();

private:
    PropertyMap &operator=(const PropertyMap &); // not provided
};

typedef PropertyMap::value_type PropertyPair;

}

#endif
