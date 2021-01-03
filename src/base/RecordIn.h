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

#ifndef RG_RECORDIN_H
#define RG_RECORDIN_H

#include "XmlExportable.h"

#include <string>

namespace Rosegarden
{


/// Audio record input of a sort that can be connected to.
struct RecordIn : public XmlExportable
{
    RecordIn() :
        mappedId(0)
    {
    }

    int mappedId;

    std::string toXmlString() const override
    {
        // We don't actually save these, as they have nothing persistent
        // in them.  The studio just remembers how many there should be.
        return "";
    }
};


}

#endif
