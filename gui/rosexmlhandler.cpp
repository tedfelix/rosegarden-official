/***************************************************************************
                          rosexmlhandler.cpp  -  description
                             -------------------
    begin                : Sat Aug 12 2000
    copyright            : (C) 2000 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "rosedebug.h"
#include "rosexmlhandler.h"
#include "xmlstorableevent.h"

RoseXmlHandler::RoseXmlHandler(Composition &composition)
    : m_composition(composition),
      m_currentTrack(0),
      m_currentTime(0),
      m_groupDuration(0),
      m_inChord(false)
{
//     kdDebug(KDEBUG_AREA) << "RoseXmlHandler() : composition size : "
//                          << m_composition.getNbTracks()
//                          << " addr : " << &m_composition
//                          << endl;
}

RoseXmlHandler::~RoseXmlHandler()
{
}

bool
RoseXmlHandler::startDocument()
{
    // reset state
    return true;
}

bool
RoseXmlHandler::startElement(const QString& /*namespaceURI*/,
                             const QString& /*localName*/,
                             const QString& qName, const QXmlAttributes& atts)
{
    QString lcName = qName.lower();

    if (lcName == "rosegarden-data") {
        // set to some state which says it's ok to parse the rest

    } else if (lcName == "track") {
        m_currentTime = 0;
        int trackNb = -1;
        QString trackNbStr = atts.value("nb");
        if (trackNbStr) {
            trackNb = trackNbStr.toInt();
//             kdDebug(KDEBUG_AREA) << "RoseXmlHandler::startElement : track nb = "
//                                  << trackNb << endl;
        }
        
        m_currentTrack = new Track;
        bool rc = true;
        
//         kdDebug(KDEBUG_AREA) << "RoseXmlHandler::startElement : composition size is now "
//                              << m_composition.getNbTracks()
//                              << endl;
        if (trackNb > 0)
            rc = m_composition.addTrack(m_currentTrack, trackNb);
        else
            rc = m_composition.addTrack(m_currentTrack);
    
        if (!rc)
            kdDebug(KDEBUG_AREA) << "RoseXmlHandler::startElement : addTrack failed"
                                 << endl;
        else
            kdDebug(KDEBUG_AREA) << "RoseXmlHandler::startElement : addTrack succeeded - composition size is now "
                                 << m_composition.getNbTracks()
                                 << endl;

    } else if (lcName == "event") {

        XMLStorableEvent *newEvent = new XMLStorableEvent(atts);
        newEvent->setAbsoluteTime(m_currentTime);
        
        if (!m_inChord) {

            m_currentTime += newEvent->duration();

        } else if (m_groupDuration == 0 &&
                   newEvent->duration() != 0) {

            // set group duration to the duration of the 1st element
            // with a non-null duration (if no such elements, leave it
            // to 0).

            m_groupDuration = newEvent->duration();
        }
        
        m_currentTrack->insert(newEvent);

    } else if (lcName == "chord") {

        m_inChord = true;
        
    } else {
        kdDebug(KDEBUG_AREA) << "RoseXmlHandler::startElement : Don't know how to parse this : " << qName << endl;
    }

    return true;
}

bool
RoseXmlHandler::endElement(const QString& /*namespaceURI*/,
                           const QString& /*localName*/, const QString& qName)
{
    QString lcName = qName.lower();

    if (lcName == "group") {
        m_currentTime += m_groupDuration;
        m_inChord = false;
        m_groupDuration = 0;
    }

    return true;
}

bool
RoseXmlHandler::characters(const QString&)
{
    return true;
}

QString
RoseXmlHandler::errorString()
{
    return "The document is not in the Rosegarden XML format";
}


bool
RoseXmlHandler::fatalError(const QXmlParseException& exception)
{
    return QXmlDefaultHandler::fatalError( exception );
}

