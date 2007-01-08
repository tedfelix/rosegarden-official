/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "HydrogenLoader.h"

#include <qxml.h>
#include "base/Composition.h"
#include "base/PropertyName.h"
#include "base/Segment.h"
#include "base/Studio.h"
#include "gui/general/ProgressReporter.h"
#include "HydrogenXMLHandler.h"
#include <qfile.h>
#include <qobject.h>
#include <qstring.h>


namespace Rosegarden
{

HydrogenLoader::HydrogenLoader(Studio *studio,
                               QObject *parent, const char *name):
        ProgressReporter(parent, name),
        m_studio(studio)
{}

bool
HydrogenLoader::load(const QString& fileName, Composition &comp)
{
    m_composition = &comp;
    comp.clear();

    QFile file(fileName);
    if (!file.open(IO_ReadOnly)) {
        return false;
    }

    m_studio->unassignAllInstruments();

    HydrogenXMLHandler handler(m_composition);

    QXmlInputSource source(file);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);

    bool ok = reader.parse(source);

    return ok;
}

}
