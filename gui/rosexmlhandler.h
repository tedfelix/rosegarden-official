// -*- c-basic-offset: 4 -*-

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

#ifndef ROSEXMLHANDLER_H
#define ROSEXMLHANDLER_H

#include <qxml.h>
#include "Composition.h"
#include "Event.h"

class XmlStorableEvent;

/**
 * Handler for the Rosegarden XML format
 */
class RoseXmlHandler : public QXmlDefaultHandler
{
public:

    /**
     * Construct a new RoseXmlHandler which will put the data extracted
     * from the XML file into the specified composition
     */
    RoseXmlHandler(Rosegarden::Composition &composition);

    virtual ~RoseXmlHandler();

    /// return the error protocol if parsing failed
    QString errorProtocol();

    /// overloaded handler functions
    virtual bool startDocument();
    virtual bool startElement(const QString& namespaceURI,
                              const QString& localName,
                              const QString& qName,
                              const QXmlAttributes& atts);

    virtual bool endElement(const QString& namespaceURI,
                            const QString& localName,
                            const QString& qName);

    virtual bool characters(const QString& ch);

    virtual bool endDocument (); // [rwb] - for tempo element catch

    /// Return the error string set during the parsing (if any)
    QString errorString();

    bool error(const QXmlParseException& exception);
    bool fatalError(const QXmlParseException& exception);


protected:
    //--------------- Data members ---------------------------------

    Rosegarden::Composition &m_composition;
    Rosegarden::Segment *m_currentSegment;
    Rosegarden::Composition::ReferenceSegment *m_currentReferenceSegment;
    XmlStorableEvent *m_currentEvent;

    Rosegarden::timeT m_currentTime;
    Rosegarden::timeT m_chordDuration;

    bool m_inChord;
    bool m_inGroup;
    std::string m_groupType;
    int m_groupId;
    int m_groupTupledLength;
    int m_groupTupledCount;
    int m_groupUntupledLength;

    bool m_foundTempo;

    QString m_errorString;
};

#endif
