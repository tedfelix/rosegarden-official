// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.2
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

#include <cstdio>
#include <iostream>
#include "PropertyMap.h"

namespace Rosegarden 
{
using std::string;

string
PropertyMap::toXmlString()
{
    string xml;

    for (const_iterator i = begin(); i != end(); ++i) {
	
	xml +=
	    "<property name=\"" + encode(i->first.getName()) +
	    "\" " + i->second->getTypeName() +
	    "=\"" + encode(i->second->unparse()) +
	    "\"/>";

    }

    return xml;
}

}

