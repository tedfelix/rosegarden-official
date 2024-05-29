/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "MusicXMLLoader.h"

#include "base/Composition.h"
#include "base/Studio.h"
#include "document/RosegardenDocument.h"
#include "document/io/MusicXMLXMLHandler.h"
#include "document/io/XMLReader.h"

#include <QFile>
#include <QObject>
#include <QString>


namespace Rosegarden
{


bool
MusicXMLLoader::load(
        const QString& fileName, RosegardenDocument *doc)
{
    doc->getComposition().clear();

    // Make sure the file can be opened.
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        m_message = QObject::tr("Can't open file '%1'").arg(fileName);
        return false;
    }
    file.close();

    doc->getStudio().unassignAllInstruments();

    MusicXMLXMLHandler handler(doc);

    XMLReader reader;
    reader.setHandler(&handler);

    bool ok = reader.parse(file);
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
