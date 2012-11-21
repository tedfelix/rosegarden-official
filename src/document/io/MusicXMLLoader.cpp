/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "MusicXMLLoader.h"

#include <qxml.h>
#include "base/Composition.h"
#include "base/PropertyName.h"
#include "base/Segment.h"
#include "base/Studio.h"
#include "gui/general/ProgressReporter.h"
#include "document/io/MusicXMLXMLHandler.h"
#include <QFile>
#include <QObject>
#include <QString>


namespace Rosegarden
{

MusicXMLLoader::MusicXMLLoader(Studio *studio,
                               QObject *parent, const char */* name */):
        ProgressReporter(parent),
        m_studio(studio)
{}

bool
MusicXMLLoader::load(const QString& fileName, Composition &comp, Studio &studio)
{
    m_composition = &comp;
    m_studio = &studio;
    comp.clear();

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    m_studio->unassignAllInstruments();

    MusicXMLXMLHandler handler(m_composition, m_studio);

    QXmlInputSource source(&file);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);

    bool ok = reader.parse(source);
    if (!ok)
        m_message = handler.errorString();

    return ok;
}

QString
MusicXMLLoader::errorMessage() const
{
    return m_message;
}

}
