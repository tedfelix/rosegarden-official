/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ChordXmlHandler.h"
#include "misc/Debug.h"

namespace Rosegarden
{

ChordXmlHandler::ChordXmlHandler(Guitar::ChordMap& map)
    : ProgressReporter(0),
      m_chordMap(map)
{
}

ChordXmlHandler::~ChordXmlHandler()
{
}

bool ChordXmlHandler::startDocument()
{
    // nothing to do ?
    return true;
}

bool ChordXmlHandler::startElement(const QString& /* namespaceURI */,
                                   const QString& /* localName */,
                                   const QString& qName,
                                   const QXmlAttributes& atts)
{
    QString lcName = qName.toLower();

    if (lcName == "chordset") {
        // start new chord set
        m_currentRoot = atts.value("root").trimmed();

    } else if (lcName == "chord") {
        
        m_currentChord = Guitar::Chord(m_currentRoot);
        
        if (atts.index("ext") >= 0)
            m_currentChord.setExt(atts.value("ext").trimmed());

        if (atts.index("user") >= 0) {
            QString userVal = atts.value("user").trimmed().toLower();
            bool res = (userVal == "yes" || userVal == "1" || userVal == "true");
            m_currentChord.setUserChord(res);
        } else {
            m_currentChord.setUserChord(false);
        }

    } else if (lcName == "fingering") {
        m_inFingering = true;
    }
    
    return true;
}

bool ChordXmlHandler::endElement(const QString& /* namespaceURI */,
                                 const QString& /* localName */,
                                 const QString& qName)
{
    QString lcName = qName.toLower();

    if (lcName == "fingering") {

        m_inFingering = false;
        m_chordMap.insert(m_currentChord);
        NOTATION_DEBUG << "ChordXmlHandler::endElement (fingering) : adding chord " << m_currentChord << endl;            

    } else if (lcName == "chord") {
        
        // adding is done after each parsing of fingering
        //
//        m_chordMap.insert(m_currentChord);

    }
    
    return true;
}

bool ChordXmlHandler::characters(const QString& ch)
{
    QString ch2 = ch.simplified();
    
    if (!ch2.isEmpty() && m_inFingering) {
        if (!parseFingering(ch2))
            return false;        
    }

    return true;
}

bool ChordXmlHandler::endDocument()
{
    // m_chordMap is now a complete and unchanged copy of the data in the
    // file, so it does not need to be saved.
    m_chordMap.clearNeedSave();

    return true;
}

bool ChordXmlHandler::parseFingering(const QString& ch) {
    
    QString errString;
    
    Guitar::Fingering fingering = Guitar::Fingering::parseFingering(ch, errString);
    
    if (m_errorString.isEmpty()) {
        NOTATION_DEBUG << "ChordXmlHandler::parseFingering : fingering " << ch << endl;
        m_currentChord.setFingering(fingering);
        return true;    
    } else {
        m_errorString = errString;
        return false;
    }
}

bool
ChordXmlHandler::error(const QXmlParseException& exception)
{
    m_errorString = QString("%1 at line %2, column %3")
                    .arg(exception.message())
                    .arg(exception.lineNumber())
                    .arg(exception.columnNumber());
    return QXmlDefaultHandler::error( exception );
}

bool
ChordXmlHandler::fatalError(const QXmlParseException& exception)
{
    m_errorString = QString("%1 at line %2, column %3")
                    .arg(exception.message())
                    .arg(exception.lineNumber())
                    .arg(exception.columnNumber());
    return QXmlDefaultHandler::fatalError( exception );
}


}
