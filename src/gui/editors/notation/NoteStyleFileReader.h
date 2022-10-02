/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_NOTESTYLEFILEREADER_H
#define RG_NOTESTYLEFILEREADER_H

#include "NoteStyle.h"
#include "document/io/XMLHandler.h"

#include <QCoreApplication>
#include <QSharedPointer>

namespace Rosegarden {

// cppcheck-suppress noConstructor
class NoteStyleFileReader : public XMLHandler
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::NoteStyleFileReader)

public:
    explicit NoteStyleFileReader(NoteStyleName name);

    typedef Rosegarden::Exception StyleFileReadFailed;

    QSharedPointer<NoteStyle> getStyle()  { return m_style; }

    // Xml handler methods:

    bool startElement(const QString& namespaceURI,
                      const QString& localName,
                      const QString& qName,
                      const QXmlStreamAttributes& attributes) override;

private:
    bool setFromAttributes(Note::Type type,
                           const QXmlStreamAttributes &attributes);

    QString m_errorString;
    QSharedPointer<NoteStyle> m_style;
    bool m_haveNote;
};

}

#endif
