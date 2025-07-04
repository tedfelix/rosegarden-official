/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_XMLHANDLER_H
#define RG_XMLHANDLER_H

#include <QString>
#include <QXmlStreamAttributes>
#include <rosegardenprivate_export.h>

namespace Rosegarden
{

/**
 * The XMLHandler class provides a simple XML handler similar to the
 * SAX handler. It is used by the XMLReader class. This class was
 * introduced as preparation for QT6 in which the SAX classes are no
 * longer available.
 */

class ROSEGARDENPRIVATE_EXPORT XMLHandler
{
 public:
    virtual ~XMLHandler();
    /// The various handlers. These should be implemented by a derived class
    virtual bool characters(const QString &ch);
    virtual bool endDocument();
    virtual bool endElement(const QString &namespaceURI,
                            const QString &localName,
                            const QString &qName);
    virtual QString errorString() const;
    // unused virtual void setDocumentLocator(void *locator); // tbd
    virtual bool startDocument();
    virtual bool startElement(const QString &namespaceURI,
                              const QString &localName,
                              const QString &qName,
                              const QXmlStreamAttributes &atts);
    virtual bool fatalError(int lineNumber, int columnNumber,
                            const QString& msg);
};

}

#endif
