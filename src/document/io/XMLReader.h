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

#ifndef RG_XMLREADER_H
#define RG_XMLREADER_H

class QFile;
class QXmlStreamReader;

#include <QString>
#include <rosegardenprivate_export.h>

namespace Rosegarden
{

class XMLHandler;

/**
 * The XMLReader class provides a simple XML reader using the qt
 * stream base XML classes. It implements a SAX like interface. This
 * class was introduced as preparation for QT6 in which the SAX
 * classes (QXmlSimpleReader and QXmlDefaultHandler) are deprecated.
 */
class ROSEGARDENPRIVATE_EXPORT XMLReader
{
 public:
    XMLReader();

    /// set the (SAX like) handler for processing the XML elements
    void setHandler(XMLHandler* handler);

    /// Parse the give string containing XML
    bool parse(const QString& xmlString);

    /// parse the XML file
    bool parse(QFile& xmlFile);
    
 private:
    XMLHandler* m_handler;

    /// do the work
    bool doParse(QXmlStreamReader& reader);
};
 
}

#endif
