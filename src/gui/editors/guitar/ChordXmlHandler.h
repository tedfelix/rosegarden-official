/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#ifndef RG_CHORDXMLHANDLER_H
#define RG_CHORDXMLHANDLER_H

#include "Chord.h"
#include "ChordMap.h"

#include <qxml.h>


namespace Rosegarden
{

class ChordXmlHandler : public QXmlDefaultHandler
{
public:
	ChordXmlHandler(Guitar::ChordMap&);
    ~ChordXmlHandler() override;
    
    /// overloaded handler functions
    bool startDocument() override;
    bool startElement(const QString& namespaceURI,
                              const QString& localName,
                              const QString& qName,
                              const QXmlAttributes& atts) override;

    bool endElement(const QString& namespaceURI,
                            const QString& localName,
                            const QString& qName) override;

    bool characters(const QString& ch) override;

    bool endDocument () override;

    /// Return the error string set during the parsing (if any)
    QString errorString() const override { return m_errorString; }
    bool error(const QXmlParseException& exception) override;
    bool fatalError(const QXmlParseException& exception) override;

protected:

    bool parseFingering(const QString& ch);

    Guitar::Chord m_currentChord;
    QString m_currentRoot;
    QString m_errorString;
    bool m_inFingering;
    Guitar::ChordMap& m_chordMap;
};

}

#endif /*RG_CHORDXMLHANDLER_H*/
