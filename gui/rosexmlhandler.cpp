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
#include "Track.h"

#include "MidiDevice.h"
#include "AudioDevice.h"

#include <klocale.h>
#include <qtextstream.h>

using Rosegarden::Composition;
using Rosegarden::Studio;
using Rosegarden::Int;
using Rosegarden::String;
using Rosegarden::Segment;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::timeT;

using namespace Rosegarden::BaseProperties;


RoseXmlHandler::RoseXmlHandler(Composition &composition,
                               Studio &studio,
                               Rosegarden::AudioFileManager &audioFileManager)
    : m_composition(composition),
      m_studio(studio),
      m_audioFileManager(audioFileManager),
      m_currentSegment(0),
      m_currentEvent(0),
      m_currentTime(0),
      m_chordDuration(0),
      m_inChord(false),
      m_inGroup(false),
      m_inComposition(false),
      m_groupId(0),
      m_foundTempo(false),
      m_section(NoSection),
      m_device(0)
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
    m_composition.clearTracks();

    // and the loop
    //
    m_composition.setLoopStart(0);
    m_composition.setLoopEnd(0);

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

        // validate top level open

    } else if (lcName == "studio") {

        if (m_section != NoSection)
        {
            m_errorString = i18n("Found Studio in another section");
            return false;
        }

        m_section = InStudio;

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

    } else if (lcName == "composition") {

        if (m_section != NoSection)
        {
            m_errorString = i18n("Found Composition in another section");
            return false;
        }
        
        // set Segment
        m_section = InComposition;

        int track = -1, startTime = 0;
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


        // Set the loop
        //
        QString loopStartStr = atts.value("loopstart");
        QString loopEndStr = atts.value("loopend");

        if (loopStartStr && loopEndStr)
        {
            int loopStart = loopStartStr.toInt();
            int loopEnd = loopEndStr.toInt();

            m_composition.setLoopStart(loopStart);
            m_composition.setLoopEnd(loopEnd);
        }

    } else if (lcName == "track") {

        if (m_inComposition == false)
        {
            m_errorString = i18n("Track object found outside Composition");
            return false;
        }

        int id = -1;
        int position = -1;
        int instrument = -1;
        std::string label;
        bool muted;

        QString trackNbStr = atts.value("id");
        if (trackNbStr) {
            id = trackNbStr.toInt();
        }

        QString labelStr = atts.value("label");
        if (labelStr) {
            label = std::string(labelStr.data());
        }

        QString mutedStr = atts.value("muted");
        if (mutedStr) {
            if (mutedStr == "true")
                muted = true;
            else
                muted = false;
        }

        QString positionStr = atts.value("position");
        if (positionStr) {
            position = positionStr.toInt();
        }

        QString instrumentStr = atts.value("instrument");
        if (instrumentStr) {
            instrument = instrumentStr.toInt();
        }
       
        Rosegarden::Track *track = new Rosegarden::Track(id, muted,
                                                         label, position,
                                                         instrument);

        m_composition.addTrack(track);


    } else if (lcName == "segment") {

        if (m_section != NoSection)
        {
            m_errorString = i18n("Found Segment in another section");
            return false;
        }
        
        // set Segment
        m_section = InSegment;

        int track = -1, startTime = 0;
        QString trackNbStr = atts.value("track");
        if (trackNbStr) {
            track = trackNbStr.toInt();
        }

        QString startIdxStr = atts.value("start");
        if (startIdxStr) {
            startTime = startIdxStr.toInt();
        }
        
        QString segmentType = (atts.value("type")).lower();
        if (segmentType) {
            if (segmentType == "audio")
            {
                int audioFileID = atts.value("file").toInt();

                // check this file id exists on the AudioFileManager

                if(m_audioFileManager.fileExists(audioFileID) == false)
                {
                    m_errorString = i18n("Cannot find audio file reference");
                    return false;
                }

                // Create an Audio segment and add its reference
                //
                m_currentSegment = new Segment(Rosegarden::Segment::Audio);
                m_currentSegment->setAudioFileID(audioFileID);
            }
            else
            {
                // Create a (normal) internal Segment
                m_currentSegment = new Segment(Rosegarden::Segment::Internal);
            }

        }
        else // for the moment we default
            m_currentSegment = new Segment(Rosegarden::Segment::Internal);
    
        m_currentSegment->setTrack(track);
        m_currentSegment->setStartTime(startTime);
	m_currentTime = startTime;
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
        if(m_audioFileManager.insertFile(std::string(label.data()),
                                         std::string(file.data()),
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

        m_audioFileManager.addSearchPath(std::string(search.data()));

    } else if (lcName == "begin") {
        int marker = atts.value("marker").toInt();

        if (!m_currentSegment)
        {
            m_errorString = i18n("found sample begin marker outside segment");
            return false;
        }

        if (m_currentSegment->getType() != Rosegarden::Segment::Audio)
        {
            m_errorString = i18n("found sample begin marker in non audio segment");
            return false;
        }

        m_currentSegment->setAudioStartTime(marker);


    } else if (lcName == "end") {
        int marker = atts.value("marker").toInt();

        if (!m_currentSegment)
        {
            m_errorString = i18n("found sample end marker outside segment");
            return false;
        }

        if (m_currentSegment->getType() != Rosegarden::Segment::Audio)
        {
            m_errorString = i18n("found sample end marker in non audio segment");
            return false;
        }

        if (marker < m_currentSegment->getAudioStartTime())
        {
            m_errorString = i18n("audio end marker before audio start marker");
            return false;
        }

        m_currentSegment->setAudioEndTime(marker);
        m_currentSegment->setDuration(marker -
                               m_currentSegment->getAudioStartTime());

    } else if (lcName == "device") {

        if (m_section != InStudio)
        {
            m_errorString = i18n("Found Device outside Studio");
            return false;
        }

        QString type = (atts.value("type")).lower();

        if (type == "midi")
        {
            m_device = new Rosegarden::MidiDevice(atts.value("name").data());
            m_studio.addDevice(m_device);

            /*
            m_studio.addDevice(std::string(atts.value("name").data()),
                               Rosegarden::Device::Midi);
                               */
        }
        else if (type == "audio")
        {
            m_device = new Rosegarden::AudioDevice(atts.value("name").data());
            m_studio.addDevice(m_device);

            /*
            m_studio.addDevice(std::string(atts.value("name").data()),
                               Rosegarden::Device::Audio);
                               */
        }
        else
        {
            m_errorString = i18n("Found unknown Device type");
            return false;
        }

    } else if (lcName == "bank") {

        if (m_section != InStudio)
        {
            m_errorString = i18n("Found Bank outside Studio");
            return false;
        }

    } else if (lcName == "program") {

        if (m_section != InStudio)
        {
            m_errorString = i18n("Found Program outside Studio");
            return false;
        }

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
        m_section = NoSection;

    } else if (lcName == "bar-segment" || lcName == "tempo-segment") {
	
	m_currentSegment = 0;
    } else if (lcName == "composition") {
        m_inComposition = false;
        m_section = NoSection;

    } else if (lcName == "studio") {

        m_section = NoSection;
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


