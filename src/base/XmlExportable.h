/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_XMLEXPORTABLE_H
#define RG_XMLEXPORTABLE_H

#include <string>

namespace Rosegarden
{


/**
 * Abstract base class that forces all derived classes
 * to implement the virtual toXmlString object.
 *
 * Yes, this is similar to the XmlStoreableEvent class
 * in gui/ but with hopes to be more general so that any
 * classes in base/ can go ahead and implement it.
 *
 * \author rwb
 */
class XmlExportable
{
public:
    XmlExportable() {;}
    virtual ~XmlExportable() {;}

    virtual std::string toXmlString() const = 0;

    /// Escape any xml special characters and validate utf8.
    static std::string encode(const std::string &);
};


}

#endif // RG_XMLEXPORTABLE_H
