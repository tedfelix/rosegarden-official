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

#define RG_MODULE_STRING "[XMLReader]"
#define RG_NO_DEBUG_PRINT 1

#include "misc/Debug.h"
#include "document/io/XMLReader.h"
#include "document/io/XMLHandler.h"

#include <QXmlStreamReader>
#include <QFile>

namespace Rosegarden
{

XMLReader::XMLReader()
{
    m_handler = nullptr;
}
    
void XMLReader::setHandler(XMLHandler* handler)
{
    m_handler = handler;
}

bool XMLReader::parse(const QString& xmlString)
{
    if (! m_handler) return false;
    QXmlStreamReader xml;
    xml.addData(xmlString);

    return doParse(xml);
}

bool XMLReader::parse(QFile& xmlFile)
{
    if (!xmlFile.open(QFile::ReadOnly | QFile::Text)) {
        qWarning() << "XMLReader could not open file" << xmlFile.fileName();
        return false;
    }
    
    QXmlStreamReader xml;
    xml.setDevice(&xmlFile);

    return doParse(xml);
}

bool XMLReader::doParse(QXmlStreamReader& reader)
{
    bool ok = true;
    while (ok  &&  !reader.atEnd()) {
        QXmlStreamReader::TokenType token = reader.readNext();
        // Silence gcc compiler warnings due to the switches below not covering all cases, on purpose
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (token) {
        case QXmlStreamReader::StartDocument:
            {
                ok = m_handler->startDocument();
            }
            break;
        case QXmlStreamReader::EndDocument:
            {
                ok = m_handler->endDocument();
            }
            break;
        case QXmlStreamReader::StartElement:
            {
                ok = m_handler->startElement(reader.namespaceUri().toString(),
                                             reader.name().toString(),
                                             reader.qualifiedName().toString(),
                                             reader.attributes());
            }
            break;
        case QXmlStreamReader::EndElement:
            {
                ok = m_handler->endElement(reader.namespaceUri().toString(),
                                           reader.name().toString(),
                                           reader.qualifiedName().toString());
            }
            break;
        case QXmlStreamReader::Characters:
            {
                ok = m_handler->characters(reader.text().toString());
            }
            break;
        default:
            {
                RG_DEBUG << "defaut for token" << token;
            }
            break;
        }
#pragma GCC diagnostic pop
    }
    
    if (! ok) {
        qDebug() << m_handler->errorString();
    }
    if (reader.hasError()) {
        RG_DEBUG << "error";
        m_handler->fatalError(reader.lineNumber(), reader.columnNumber(),
                              reader.errorString());
    }
    return ok;
}
    
}
