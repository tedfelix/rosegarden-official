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


#include "HydrogenLoader.h"

#include <qxml.h>
#include "base/Composition.h"
#include "base/PropertyName.h"
#include "base/Segment.h"
#include "base/Studio.h"
#include "HydrogenXMLHandler.h"
#include <QFile>
#include <QObject>
#include <QString>


namespace Rosegarden
{

HydrogenLoader::HydrogenLoader(Studio *studio):
        m_studio(studio)
{}

bool
HydrogenLoader::load(const QString& fileName, Composition &comp)
{
    m_composition = &comp;
    comp.clear();

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    m_studio->unassignAllInstruments();

    HydrogenXMLHandler handler(m_composition);

    QXmlInputSource source(&file);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);

    bool ok = reader.parse(source);

    return ok;
}

}
