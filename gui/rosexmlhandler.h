/***************************************************************************
                          rosexmlhandler.h  -  description
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

#ifndef ROSEXMLHANDLER_H
#define ROSEXMLHANDLER_H

#include <qxml.h>
#include "Event.h"

/**Handler for the Rosegarden XML format
 *
 *@author Guillaume Laurent, Chris Cannam, Rich Bown
 */

class RoseXmlHandler : public QXmlDefaultHandler  {
public:
    RoseXmlHandler(Composition &composition);
    virtual ~RoseXmlHandler();

    // return the error protocol if parsing failed
    QString errorProtocol();

    // overloaded handler functions
    virtual bool startDocument();
    virtual bool startElement(const QString& namespaceURI,
                              const QString& localName,
                              const QString& qName,
                              const QXmlAttributes& atts);

    virtual bool endElement(const QString& namespaceURI,
                            const QString& localName,
                            const QString& qName);

    virtual bool characters(const QString& ch);

    QString errorString();

    bool fatalError(const QXmlParseException& exception);

protected:
    Composition &m_composition;
    Track *m_currentTrack;

    Event::timeT m_currentTime;

    Event::timeT m_groupDuration;
    bool m_inGroup;
};

#endif
