/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

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

#include <qxml.h>

#include "NoteStyle.h"

#include <QCoreApplication>

namespace Rosegarden {

class NoteStyleFileReader : public QXmlDefaultHandler
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::NoteStyleFileReader)

public:
    NoteStyleFileReader(NoteStyleName name);

    typedef Rosegarden::Exception StyleFileReadFailed;
    
    NoteStyle *getStyle() { return m_style; }

    // Xml handler methods:

    virtual bool startElement
    (const QString& namespaceURI, const QString& localName,
     const QString& qName, const QXmlAttributes& atts);
    
private:
    bool setFromAttributes(Note::Type type, const QXmlAttributes &attributes);

    QString m_errorString;
    NoteStyle *m_style;
    bool m_haveNote;
};

}

#endif
