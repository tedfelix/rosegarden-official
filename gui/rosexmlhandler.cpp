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

RoseXmlHandler::RoseXmlHandler(EventList &events)
    : m_events(events)
{
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
RoseXmlHandler::startElement(const QString& namespaceURI,
                             const QString& localName,
                             const QString& qName, const QXmlAttributes& atts)
{
    QString lcName = qName.lower();

    if (lcName == "event") {
        m_events.push_back(new XMLStorableEvent(atts));
    } else if (lcName == "track") {
        // later
    } else {
        kdDebug(KDEBUG_AREA) << "Don't know how to parse this : " << qName << endl;
    }
    return true;
}

bool
RoseXmlHandler::endElement(const QString& namespaceURI,
                           const QString& localName, const QString& qName)
{
    return true;
}

bool
RoseXmlHandler::characters(const QString& ch)
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

