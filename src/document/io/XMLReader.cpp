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

#include "document/io/XMLReader.h"
#include "document/io/XMLHandler.h"
#include <QXmlStreamReader>

#define RG_MODULE_STRING "[XMLReader]"
#define RG_NO_DEBUG_PRINT 1

#include "misc/Debug.h"

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
    
    bool ok = true;
    while (!xml.atEnd()) {
        QXmlStreamReader::TokenType token = xml.readNext();
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
                ok = m_handler->startElement(xml.namespaceUri().toString(),
                                             xml.name().toString(),
                                             xml.qualifiedName().toString(),
                                             xml.attributes());
            }
            break;
        case QXmlStreamReader::EndElement:
            {
                ok = m_handler->endElement(xml.namespaceUri().toString(),
                                           xml.name().toString(),
                                           xml.qualifiedName().toString());
            }
            break;
        case QXmlStreamReader::Characters:
            {
                ok = m_handler->characters(xml.text().toString());
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
  
  if (xml.hasError()) {
      RG_DEBUG << "error";
      m_handler->fatalError(xml.lineNumber(), xml.columnNumber(),
                            xml.errorString());
  }

  return ok;
  
}

}
