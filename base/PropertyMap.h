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

#ifndef _PROPERTY_MAP_H_
#define _PROPERTY_MAP_H_

#include "Property.h"
#include "PropertyName.h"

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

namespace Rosegarden {

class PropertyMap : public __HASH_NS::hash_map<PropertyName,
					       PropertyStoreBase *,
					       PropertyNameHash,
					       PropertyNamesEqual>
{
public:
    ~PropertyMap() {
	clear();
    }
    
    void clear() {
	for (iterator i = begin(); i != end(); ++i) delete i->second;
	erase(begin(), end());
    }
};

typedef PropertyMap::value_type PropertyPair;

}

#endif
