// -*- c-basic-offset: 4 -*-

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

#include <cstdio>
#include <iostream>
#include <string>
#include "PropertyMap.h"
#include "XmlExportable.h"

namespace Rosegarden 
{
using std::string;

PropertyMap::PropertyMap(const PropertyMap &pm) :
    __HASH_NS::hash_map<PropertyName,
			PropertyStoreBase *,
			PropertyNameHash,
			PropertyNamesEqual>()
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
PropertyMap::toXmlString()
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

}

