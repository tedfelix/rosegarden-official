// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
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

#include "rosedebug.h"
#include "rosexmlhandler.h"
#include "xmlstorableevent.h"
#include "SegmentNotationHelper.h"
#include "BaseProperties.h"
#include "Instrument.h"
#include "Track.h"

#include <klocale.h>
#include <qtextstream.h>

using Rosegarden::Composition;
using Rosegarden::Int;
using Rosegarden::String;
using Rosegarden::Segment;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::timeT;

using namespace Rosegarden::BaseProperties;


RoseXmlHandler::RoseXmlHandler(Composition &composition,
                               Rosegarden::AudioFileManager &audioFileManager)
    : m_composition(composition),
      m_audioFileManager(audioFileManager),
      m_currentSegment(0),
      m_currentEvent(0),
      m_currentTime(0),
      m_chordDuration(0),
      m_inChord(false),
      m_inGroup(false),
      m_inComposition(false),
      m_groupId(0),
      m_foundTempo(false)
{
//     kdDebug(KDEBUG_AREA) << "RoseXmlHandler() : composition size : "
//                          << m_composition.getNbSegments()
//                          << " addr : " << &m_composition
//                          << endl;
}

RoseXmlHandler::~RoseXmlHandler()
{
}

bool
RoseXmlHandler::startDocument()
{
    // clear things before we load 'em
    //
    m_composition.clearTracks();
    m_composition.clearInstruments();

    // reset state
    return true;
}

bool
RoseXmlHandler::startElement(const QString& /*namespaceURI*/,
                             const QString& /*localName*/,
                             const QString& qName, const QXmlAttributes& atts)
{
    QString lcName = qName.lower();

    if (lcName == "event") {

//        kdDebug(KDEBUG_AREA) << "RoseXmlHandler::startElement: found event, current time is " << m_currentTime << endl;

        if (m_currentEvent) {
            kdDebug(KDEBUG_AREA) << "RoseXmlHandler::startElement: Warning: new event found at time " << m_currentTime << " before previous event has ended; previous event will be lost" << endl;
            delete m_currentEvent;
        }

        m_currentEvent = new XmlStorableEvent(atts, m_currentTime);

        if (m_inGroup) {
            m_currentEvent->setMaybe<Int>(BEAMED_GROUP_ID, m_groupId);
            m_currentEvent->setMaybe<String>(BEAMED_GROUP_TYPE, m_groupType);
	    if (m_groupType == GROUP_TYPE_TUPLED) {
		m_currentEvent->set<Int>
		    (BEAMED_GROUP_TUPLED_LENGTH, m_groupTupledLength);
		m_currentEvent->set<Int>
		    (BEAMED_GROUP_TUPLED_COUNT, m_groupTupledCount);
		m_currentEvent->set<Int>
		    (BEAMED_GROUP_UNTUPLED_LENGTH, m_groupUntupledLength);
	    }
        }

	timeT duration = m_currentEvent->getDuration();
        
        if (!m_inChord) {

            m_currentTime = m_currentEvent->getAbsoluteTime() + duration;

//            kdDebug(KDEBUG_AREA) << "RoseXmlHandler::startElement: (we're not in a chord) " << endl;

        } else if (duration != 0) {

            // set chord duration to the duration of the shortest
            // element with a non-null duration (if no such elements,
            // leave it as 0).

	    if (m_chordDuration == 0 || duration < m_chordDuration) {
		m_chordDuration = duration;
	    }
        }

    } else if (lcName == "property") {
        
        if (!m_currentEvent) {
            kdDebug(KDEBUG_AREA) << "RoseXmlHandler::startElement: Warning: Found property outside of event at time " << m_currentTime << ", ignoring" << endl;
        } else {
            m_currentEvent->setPropertiesFromAttributes(atts);
        }

    } else if (lcName == "chord") {

        m_inChord = true;

    } else if (lcName == "group") {

        if (!m_currentSegment) {
            m_errorString = i18n("Got group outside of a segment");
            return false;
        }
        
        m_inGroup = true;
        m_groupId = m_currentSegment->getNextId();
        m_groupType = atts.value("type");

	if (m_groupType == GROUP_TYPE_TUPLED) {
	    m_groupTupledLength = atts.value("length").toInt();
	    m_groupTupledCount = atts.value("count").toInt();
	    m_groupUntupledLength = atts.value("untupled").toInt();
	}

    } else if (lcName == "rosegarden-data") {
        // set to some state which says it's ok to parse the rest

    } else if (lcName == "timesignature") {

        if (m_inComposition == false)
        {
            m_errorString = i18n("TimeSignature object found outside Composition");
            return false;
        }

	timeT t = 0;
	QString timeStr = atts.value("time");
	if (timeStr) t = timeStr.toInt();

	int num = 4;
	QString numStr = atts.value("numerator");
	if (numStr) num = numStr.toInt();

	int denom = 4;
	QString denomStr = atts.value("denominator");
	if (denomStr) denom = denomStr.toInt();

	m_composition.addTimeSignature
	    (t, Rosegarden::TimeSignature(num, denom));

    } else if (lcName == "tempo") {

	timeT t = 0;
	QString timeStr = atts.value("time");
	if (timeStr) t = timeStr.toInt();
	
	int bph = 120 * 60; // arbitrary
	QString bphStr = atts.value("bph");
	if (bphStr) bph = bphStr.toInt();

	m_composition.addRawTempo(t, bph);

    } else if (lcName == "instrument") {

        if (m_inComposition == false)
        {
            m_errorString = i18n("Instrument object found outside Composition");
            return false;
        }

        int id = -1;
        string name;
        Rosegarden::Instrument::InstrumentType it;

        QString instrNbStr = atts.value("id");
        if (instrNbStr) {
            id = instrNbStr.toInt();
        }

        QString nameStr = atts.value("name");
        if (nameStr) {
            name = string(nameStr.data());
        }

        QString instrTypeStr = atts.value("type");
        if (instrTypeStr) {
            if (instrTypeStr == "midi")
                it = Rosegarden::Instrument::Midi;
            else if (instrTypeStr == "audio")
                it = Rosegarden::Instrument::Audio;
            else
                it = Rosegarden::Instrument::Midi;
        }
        

        // Create and insert the Instrument
        //
        //
        Rosegarden::Instrument *instrument =
                new Rosegarden::Instrument(id, it, name);

        m_composition.addInstrument(instrument);

    } else if (lcName == "composition") {

        // Get and set the record track
        //
        int recordTrack = -1;
        QString recordStr = atts.value("recordtrack");

        if (recordStr) {
            recordTrack = recordStr.toInt();
        }

        m_composition.setRecordTrack(recordTrack);

        // Get and ste the position pointer
        //
        int position = 0;
        QString positionStr = atts.value("pointer");
        if (positionStr) {
            position = positionStr.toInt();
        }

        m_composition.setPosition(position);


        // Get and (eventually) set the tempo
        //
        double tempo;
        QString tempoStr = atts.value("defaultTempo");
        if (tempoStr) {
            tempo = tempoStr.toDouble();
        }

        m_composition.setDefaultTempo(tempo);
        
        // set the composition flag
        m_inComposition = true;

    } else if (lcName == "track") {

        if (m_inComposition == false)
        {
            m_errorString = i18n("Track object found outside Composition");
            return false;
        }

        int id = -1;
        int position = -1;
        int instrument = -1;
        string label;
        bool muted;
        Rosegarden::Track::TrackType tt;

        QString trackNbStr = atts.value("id");
        if (trackNbStr) {
            id = trackNbStr.toInt();
        }

        QString labelStr = atts.value("label");
        if (labelStr) {
            label = string(labelStr.data());
        }

        QString mutedStr = atts.value("muted");
        if (mutedStr) {
            if (mutedStr == "true")
                muted = true;
            else
                muted = false;
        }

        QString trackTypeStr = atts.value("type");
        if (trackTypeStr) {
            if (trackTypeStr == "midi")
                tt = Rosegarden::Track::Midi;
            else if (trackTypeStr == "audio")
                tt = Rosegarden::Track::Audio;
            else // default
                tt = Rosegarden::Track::Midi;
        }

        QString positionStr = atts.value("position");
        if (positionStr) {
            position = positionStr.toInt();
        }

        QString instrumentStr = atts.value("instrument");
        if (instrumentStr) {
            instrument = instrumentStr.toInt();
        }
       
        Rosegarden::Track *track = new Rosegarden::Track(id, muted, tt,
                                                         label, position,
                                                         instrument);

        m_composition.addTrack(track);


    } else if (lcName == "segment") {

        int track = -1, startIndex = 0;
        QString trackNbStr = atts.value("track");
        if (trackNbStr) {
            track = trackNbStr.toInt();
        }

        QString startIdxStr = atts.value("start");
        if (startIdxStr) {
            startIndex = startIdxStr.toInt();
        }
        
        Rosegarden::Segment::SegmentType sT;

        QString segmentType = (atts.value("type")).lower();
        if (segmentType) {
            if (segmentType == "audio")
                sT = Rosegarden::Segment::Audio;
            else
                sT = Rosegarden::Segment::Internal;
        }
    
        m_currentSegment = new Segment(sT);
        m_currentSegment->setTrack(track);
        m_currentSegment->setStartIndex(startIndex);
	m_currentTime = startIndex;

        m_composition.addSegment(m_currentSegment);
    
    } else if (lcName == "resync") {
	
	QString time(atts.value("time"));
	bool isNumeric;
	int numTime = time.toInt(&isNumeric);
	if (isNumeric) m_currentTime = numTime;

    } else if (lcName == "audio") {

        if (m_inComposition == false)
        {
            m_errorString = i18n("Audio object found outside Composition");
            return false;
        }

	QString id(atts.value("id"));
        QString file(atts.value("file"));
        QString label(atts.value("label"));

        if (id.isEmpty() || file.isEmpty() || label.isEmpty())
        {
            m_errorString = i18n("Audio object has empty parameters");
            return false;
        }

        // attempt to insert file into AudioFileManager
        // (this checks the integrity of the file at the
        // same time)
        //
        if(m_audioFileManager.insertFile(string(label.data()),
                                         string(file.data()),
                                         id.toInt()) == false)
        {
            m_errorString = i18n("Couldn't find audio file " + file);
            return false;
        }
        
    } else if (lcName == "audiopath") {

        if (m_inComposition == false)
        {
            m_errorString = i18n("Audiopath object found outside Composition");
            return false;
        }

        QString search(atts.value("search"));

        if (search.isEmpty())
        {
            m_errorString = i18n("Audiopath has no search parameter");
            return false;
        }

        m_audioFileManager.addSearchPath(string(search.data()));

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

    if (lcName == "event") {

        if (m_currentSegment && m_currentEvent) {
            m_currentSegment->insert(m_currentEvent);
            m_currentEvent = 0;
        } else if (!m_currentSegment && m_currentEvent) {
            m_errorString = i18n("Got event outside of a Segment");
            return false;
        }
        
    } else if (lcName == "chord") {

        m_currentTime += m_chordDuration;
        m_inChord = false;
        m_chordDuration = 0;

    } else if (lcName == "group") {

        m_inGroup = false;

    } else if (lcName == "segment") {

        m_currentSegment = 0;

    } else if (lcName == "bar-segment" || lcName == "tempo-segment") {
	
	m_currentSegment = 0;
    } else if (lcName == "composition") {
        m_inComposition = false;
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
    return m_errorString;
}

// Not used yet
bool
RoseXmlHandler::error(const QXmlParseException& exception)
{
    return QXmlDefaultHandler::error( exception );
}

bool
RoseXmlHandler::fatalError(const QXmlParseException& exception)
{
    return QXmlDefaultHandler::fatalError( exception );
}

bool
RoseXmlHandler::endDocument()
{
  if (m_foundTempo == false)
    m_composition.setDefaultTempo(120.0);

  return true;
}


