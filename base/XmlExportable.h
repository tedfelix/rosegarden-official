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

#ifndef _XMLEXPORTABLE_H_
#define _XMLEXPORTABLE_H_

#include <string>

// [rwb]
//
//   Abstract base class that forces all derived classes
//   to implement the virtual toXmlString object.
//
//   Yes, this is similar to  the XmlStoreableEvent class
//   in gui/ but with hopes to be more general so that any
//   classes in base/ can go ahead and implement it.
//
//

namespace Rosegarden
{

class XmlExportable
{
public:
    XmlExportable() {;}
    virtual ~XmlExportable() {;}

    virtual std::string toXmlString() = 0;

    static std::string encode(const std::string &);
};

}

#endif // _XMLEXPORTABLE_H_

