// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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
#include "AudioFileManager.h"
#include "Studio.h"
#include "rosegardenguidoc.h"
#include "progressreporter.h"

class XmlStorableEvent;
class XmlSubHandler;

/**
 * Handler for the Rosegarden XML format
 */
class RoseXmlHandler : public ProgressReporter, public QXmlDefaultHandler
{
public:

    typedef enum
    {
        NoSection,
        InComposition,
        InSegment,
        InStudio,
        InInstrument,
        InAudioFiles,
        InPlugin
    } RosegardenFileSection;

    /**
     * Construct a new RoseXmlHandler which will put the data extracted
     * from the XML file into the specified composition
     */
    RoseXmlHandler(RosegardenGUIDoc *doc,
                   unsigned int elementCount,
		   bool createNewDevicesWhenNeeded);

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

    bool isDeprecated() { return m_deprecation; }

    bool isCancelled() { return m_cancelled; }

    /// Return the error string set during the parsing (if any)
    QString errorString();

    bool error(const QXmlParseException& exception);
    bool fatalError(const QXmlParseException& exception);

protected:

    // just for convenience
    //
    Rosegarden::Composition& getComposition()
        { return m_doc->getComposition(); }
    Rosegarden::Studio& getStudio()
        { return m_doc->getStudio(); }
    Rosegarden::AudioFileManager& getAudioFileManager()
        { return m_doc->getAudioFileManager(); }
    Rosegarden::AudioPluginManager* getAudioPluginManager()
        { return m_doc->getPluginManager(); }

    void setSubHandler(XmlSubHandler* sh);
    XmlSubHandler* getSubHandler() { return m_subHandler; }

    void addMIDIDevice(QString name);
                                         
    //--------------- Data members ---------------------------------

    RosegardenGUIDoc    *m_doc;
    Rosegarden::Segment *m_currentSegment;
    XmlStorableEvent    *m_currentEvent;

    Rosegarden::timeT m_currentTime;
    Rosegarden::timeT m_chordDuration;
    Rosegarden::timeT *m_segmentEndMarkerTime;

    bool m_inChord;
    bool m_inGroup;
    bool m_inComposition;
    std::string m_groupType;
    int m_groupId;
    int m_groupTupletBase;
    int m_groupTupledCount;
    int m_groupUntupledCount;
    std::map<long, long> m_groupIdMap;

    bool m_foundTempo;

    QString m_errorString;

    RosegardenFileSection             m_section;
    Rosegarden::Device               *m_device;
    Rosegarden::MidiByte              m_msb;
    Rosegarden::MidiByte              m_lsb;
    Rosegarden::Instrument           *m_instrument;
    Rosegarden::AudioPluginInstance  *m_plugin;
    unsigned int                      m_pluginId;
    unsigned int                      m_totalElements;
    unsigned int                      m_elementsSoFar;

    XmlSubHandler                    *m_subHandler;
    bool		              m_deprecation;
    bool                              m_createDevices;
    bool                              m_cancelled;
};

#endif
