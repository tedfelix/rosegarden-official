
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "rosedebug.h"
#include "rosexmlhandler.h"
#include "xmlstorableevent.h"

RoseXmlHandler::RoseXmlHandler(Composition &composition)
    : m_composition(composition),
      m_currentTrack(0),
      m_currentTime(0),
      m_chordDuration(0),
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

        kdDebug(KDEBUG_AREA) << "RoseXmlHandler::startElement: found event, current time is " << m_currentTime << endl;

        XMLStorableEvent *newEvent = new XMLStorableEvent(atts);
        newEvent->setAbsoluteTime(m_currentTime);
        
        if (!m_inChord) {

            m_currentTime += newEvent->getDuration();

            kdDebug(KDEBUG_AREA) << "RoseXmlHandler::startElement: (we're not in a chord) " << endl;

        } else if (m_chordDuration == 0 &&
                   newEvent->getDuration() != 0) {

            // set chord duration to the duration of the 1st element
            // with a non-null duration (if no such elements, leave it
            // to 0).

            m_chordDuration = newEvent->getDuration();
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

    if (lcName == "chord") {
        m_currentTime += m_chordDuration;
        m_inChord = false;
        m_chordDuration = 0;
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

