/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <cstdio>
#include <iostream>
#include <string>
#include "PropertyMap.h"
#include "XmlExportable.h"

namespace Rosegarden 
{
using std::string;

#ifdef PROPERTY_MAP_IS_HASH_MAP
PropertyMap::PropertyMap() :
    __HASH_NS::hash_map<PropertyName,
			PropertyStoreBase *,
			PropertyNameHash,
			PropertyNamesEqual>(50, PropertyNameHash())
{
    // nothing
}
#endif

PropertyMap::PropertyMap(const PropertyMap &pm) :

#ifdef PROPERTY_MAP_IS_HASH_MAP

    __HASH_NS::hash_map<PropertyName,
			PropertyStoreBase *,
			PropertyNameHash,
			PropertyNamesEqual>(50, PropertyNameHash())
#else

    std::map<PropertyName, PropertyStoreBase *>()

#endif

{
    for (const_iterator i = pm.begin(); i != pm.end(); ++i) {
	insert(PropertyPair(i->first, i->second->clone()));
    }
}

PropertyMap::~PropertyMap()
{
    for (iterator i = begin(); i != end(); ++i) delete i->second;
}    

void
PropertyMap::clear()
{
    for (iterator i = begin(); i != end(); ++i) delete i->second;
    erase(begin(), end());
}


// We could derive from XmlExportable and make this a virtual method
// overriding XmlExportable's pure virtual.  We don't, because this
// class has no other virtual methods and for such a core class we
// could do without the overhead (given that it wouldn't really gain
// us anything anyway).

string
PropertyMap::toXmlString() const
{
    string xml;

    for (const_iterator i = begin(); i != end(); ++i) {
	
	xml +=
	    "<property name=\"" + XmlExportable::encode(i->first.getName()) +
	    "\" " + i->second->getTypeName() +
	    "=\"" + XmlExportable::encode(i->second->unparse()) +
	    "\"/>";

    }

    return xml;
}

static bool propertyPairEqual(const PropertyPair &p1, const PropertyPair &p2)
{
    return p1.first == p2.first &&
            p1.second->getTypeName() == p2.second->getTypeName()
            && p1.second->unparse() == p2.second->unparse();
}

bool PropertyMap::operator==(const PropertyMap &other) const
{
    // The default std::map::operator== would compare pointers, provide our own predicate
    return size() == other.size()
       && std::equal(begin(), end(), other.begin(), propertyPairEqual);
}

}

