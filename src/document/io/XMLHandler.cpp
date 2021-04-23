/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "document/io/XMLHandler.h"

#define RG_MODULE_STRING "[XMLHandler]"
#define RG_NO_DEBUG_PRINT 1

#include "misc/Debug.h"

namespace Rosegarden
{

XMLHandler::~XMLHandler()
{
}

bool XMLHandler::characters(const QString&)
{
    return true;
}

bool XMLHandler::endDocument()
{
    return true;
}

bool XMLHandler::endElement(const QString &,
                            const QString &,
                            const QString &)
{
    return true;
}

QString XMLHandler::errorString() const
{
    return "";
}

void XMLHandler::setDocumentLocator(void *) // tbd
{
}
 
bool XMLHandler::startDocument()
{
    return true;
}

bool XMLHandler::startElement(const QString &,
                              const QString &,
                              const QString &,
                              const QXmlStreamAttributes &)
{
    return true;
}

bool XMLHandler::fatalError(int , int ,
                            const QString& )
{
    return true;
}

}
