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

#include "rosestrings.h"
#include "rosedebug.h"
#include "rosexmlhandler.h"
#include "xmlstorableevent.h"
#include "SegmentNotationHelper.h"
#include "BaseProperties.h"
#include "Track.h"

#include "MidiDevice.h"
#include "AudioDevice.h"
#include "Instrument.h"
#include "widgets.h"

#include <klocale.h>
#include <qtextstream.h>

using Rosegarden::Composition;
using Rosegarden::Studio;
using Rosegarden::Int;
using Rosegarden::String;
using Rosegarden::Segment;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::timeT;
using Rosegarden::Device;
using Rosegarden::DeviceList;
using Rosegarden::DeviceListIterator;

using namespace Rosegarden::BaseProperties;


RoseXmlHandler::RoseXmlHandler(Composition &composition,
                               Studio &studio,
                               Rosegarden::AudioFileManager &audioFileManager,
                               unsigned int elementCount,
                               Rosegarden::Progress *progress)
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
      m_device(0),
      m_msb(0),
      m_lsb(0),
      m_instrument(0),
      m_totalElements(elementCount),
      m_elementsSoFar(0),
      m_progress(progress)
{
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
            m_currentEvent->set<Int>(BEAMED_GROUP_ID, m_groupId);
            m_currentEvent->set<String>(BEAMED_GROUP_TYPE, m_groupType);
	    if (m_groupType == GROUP_TYPE_TUPLED) {
		m_currentEvent->set<Int>
		    (BEAMED_GROUP_TUPLET_BASE, m_groupTupletBase);
		m_currentEvent->set<Int>
		    (BEAMED_GROUP_TUPLED_COUNT, m_groupTupledCount);
		m_currentEvent->set<Int>
		    (BEAMED_GROUP_UNTUPLED_COUNT, m_groupUntupledCount);
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
	    m_groupTupletBase = atts.value("base").toInt();
	    m_groupTupledCount = atts.value("tupled").toInt();
	    m_groupUntupledCount = atts.value("untupled").toInt();
	}

    } else if (lcName == "rosegarden-data") {

        // validate top level open

    } else if (lcName == "studio") {

        if (m_section != NoSection)
        {
            m_errorString = i18n("Found Studio in another section");
            return false;
        }

        // In the Studio we clear down everything apart from Devices and
        // Instruments before we reload.  Instruments are derived from
        // the Sequencer, the bank/program information is loaded from
        // the file we're currently examining.
        //
        //
        m_studio.clearMidiBanksAndPrograms();
        m_section = InStudio; // set top level section

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

	bool common = false;
	QString commonStr = atts.value("common");
	if (commonStr) common = (commonStr == "true");

	bool hidden = false;
	QString hiddenStr = atts.value("hidden");
	if (hiddenStr) hidden = (hiddenStr == "true");

	m_composition.addTimeSignature
	    (t, Rosegarden::TimeSignature(num, denom, common, hidden));

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
        double tempo = 120.0;
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

        QString selectedTrackStr = atts.value("selected");

        if (selectedTrackStr)
        {
            Rosegarden::TrackId selectedTrack =
                (Rosegarden::TrackId)selectedTrackStr.toInt();

            m_composition.setSelectedTrack(selectedTrack);
        }

        QString soloTrackStr = atts.value("solo");
        if (soloTrackStr)
        {
            if (soloTrackStr.toInt() == 1)
                m_composition.setSolo(true);
            else
                m_composition.setSolo(false);
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
        bool muted = false;

        QString trackNbStr = atts.value("id");
        if (trackNbStr) {
            id = trackNbStr.toInt();
        }

        QString labelStr = atts.value("label");
        if (labelStr) {
            label = qstrtostr(labelStr);
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
       
        Rosegarden::Track *track = new Rosegarden::Track(id,
                                                         instrument,
                                                         position,
                                                         label,
                                                         muted);
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
                m_currentSegment->setStartTime(startTime);
            }
            else
            {
                // Create a (normal) internal Segment
                m_currentSegment = new Segment(Rosegarden::Segment::Internal);
            }

        }
        else // for the moment we default
            m_currentSegment = new Segment(Rosegarden::Segment::Internal);
    
        // fill in the label
        QString labelStr = atts.value("label");
        if (labelStr)
            m_currentSegment->setLabel(qstrtostr(labelStr));
        
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

        if (m_section != InAudio)
        {
            m_errorString = i18n("Audio object found outside Audio section");
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
        if(m_audioFileManager.insertFile(qstrtostr(label),
                                         qstrtostr(file),
                                         id.toInt()) == false)
        {
            m_errorString = i18n("Couldn't find audio file " + file);
            return false;
        }
        
    } else if (lcName == "audiopath") {

        if (m_section != InAudio)
        {
            m_errorString = i18n("Audiopath object found outside AudioFiles section");
            return false;
        }

        QString search(atts.value("value"));

        if (search.isEmpty())
        {
            m_errorString = i18n("Audiopath has no value");
            return false;
        }

        m_audioFileManager.setAudioPath(qstrtostr(search));

    } else if (lcName == "audiolastaddpath") {
        if (m_section != InAudio)
        {
            m_errorString = i18n("AudioLastAddPath object found outside AudioFiles section");
            return false;
        }

        QString path(atts.value("value"));

        if (path.isEmpty())
        {
            m_errorString = i18n("AudioLastAddPath has no value");
        }

        m_audioFileManager.setLastAddPath(qstrtostr(path));

    } else if (lcName == "begin") {
        float marker = atts.value("index").toFloat();

        if (!m_currentSegment)
        {
            m_errorString = i18n("found audio begin index outside segment");
            return false;
        }

        if (m_currentSegment->getType() != Rosegarden::Segment::Audio)
        {
            m_errorString = i18n("found audio begin index in non audio segment");
            return false;
        }

        // convert to RealTime from float
        int sec = (int)marker;
        int usec = (int)((marker - ((float)sec)) * 1000000.0);
        m_currentSegment->setAudioStartTime(Rosegarden::RealTime(sec, usec));


    } else if (lcName == "end") {
        float marker = atts.value("index").toFloat();

        if (!m_currentSegment)
        {
            m_errorString = i18n("found audio end index outside segment");
            return false;
        }

        if (m_currentSegment->getType() != Rosegarden::Segment::Audio)
        {
            m_errorString = i18n("found audio end index in non audio segment");
            return false;
        }

        int sec = (int)marker;
        int usec = (int)((marker - ((float)sec)) * 1000000.0);
        Rosegarden::RealTime markerTime(sec, usec);

        if (markerTime < m_currentSegment->getAudioStartTime())
        {
            m_errorString = i18n("audio end index before audio start marker");
            return false;
        }

        m_currentSegment->setAudioEndTime(markerTime);

        // convert both times independently and then set duration
        // according to difference.
        timeT absStart =
            m_composition.getElapsedTimeForRealTime(
                    m_currentSegment->getAudioStartTime());

        timeT absEnd = m_composition.getElapsedTimeForRealTime(markerTime);
#ifdef OLD_SEGMENT_API
        m_currentSegment->setDuration(absEnd - absStart);
#else
	m_currentSegment->setEndTime(absEnd);
#endif

    } else if (lcName == "device") {

        if (m_section != InStudio)
        {
            m_errorString = i18n("Found Device outside Studio");
            return false;
        }

        QString type = (atts.value("type")).lower();
        QString idString = atts.value("id");
            
        if (idString.isNull())
        {
            m_errorString = i18n("No ID on Device tag");
            return false;
        }
        int id = idString.toInt();

        if (type == "midi" || type == "audio")
        {
            m_device = m_studio.getDevice(id);
        }
        else
        {
            m_errorString = i18n("Found unknown Device type");
            return false;
        }

    } else if (lcName == "bank") {

        if (m_device) // only if we have a device
        {
            if (m_section != InStudio && m_section != InInstrument)
            {
                m_errorString = i18n("Found Bank outside Studio or Instrument");
                return false;
            }

            QString nameStr = atts.value("name");
            m_msb = (atts.value("msb")).toInt();
            m_lsb = (atts.value("lsb")).toInt();

            // To actually create a bank
            //
            if (m_section == InStudio)
            {
                // Create a new bank
                Rosegarden::MidiBank *bank = new Rosegarden::MidiBank();
                bank->msb = m_msb;
                bank->lsb = m_lsb;
                bank->name = qstrtostr(nameStr);
    
                if (m_device->getType() == Rosegarden::Device::Midi)
                {
                    // Insert the bank
                    //
                    dynamic_cast<Rosegarden::MidiDevice*>(m_device)->addBank(bank);
                }
            }
            else // otherwise we're referencing it in an instrument
            if (m_section == InInstrument)
            {
                if (m_instrument)
                {
                    m_instrument->setMSB(m_msb);
                    m_instrument->setLSB(m_lsb);
                    m_instrument->setSendBankSelect(true);
                }
            }
        }

    } else if (lcName == "program") {

        if (m_device) // only if we have a device
        {
            if (m_section == InStudio)
            {
                QString nameStr = (atts.value("name"));
                Rosegarden::MidiByte pc = atts.value("id").toInt();

                // Create a new program
                Rosegarden::MidiProgram *program = new Rosegarden::MidiProgram();
                program->name = qstrtostr(nameStr);
                program->program = pc;
                program->msb = m_msb;
                program->lsb = m_lsb;

                if (m_device->getType() == Rosegarden::Device::Midi)
                {
                    // Insert the program
                    //
                    dynamic_cast<Rosegarden::MidiDevice*>(m_device)->
                         addProgram(program);
                }

            }
            else if (m_section == InInstrument)
            {
                if (m_instrument)
                {
                    Rosegarden::MidiByte id = atts.value("id").toInt();
                    m_instrument->setProgramChange(id);
                    m_instrument->setSendProgramChange(true);
                }
            }
            else
            {
                m_errorString =
                    i18n("Found Program outside Studio and Instrument");
                return false;
            }
        }

    } else if (lcName == "pan") {

        if (m_section != InInstrument)
        {
            m_errorString = i18n("Found Pan outside Instrument");
            return false;
        }

        Rosegarden::MidiByte value = atts.value("value").toInt();

        if (m_instrument)
        {
            m_instrument->setPan(value);
            m_instrument->setSendPan(true);
        }

    } else if (lcName == "velocity") {

        if (m_section != InInstrument)
        {
            m_errorString = i18n("Found Pan outside Instrument");
            return false;
        }

        Rosegarden::MidiByte value = atts.value("value").toInt();

        if (m_instrument)
        {
            m_instrument->setVelocity(value);
            m_instrument->setSendVelocity(true);
        }

    } else if (lcName == "metronome") {

        if (m_section != InStudio)
        {
            m_errorString = i18n("Found Metronome outside Studio");
            return false;
        }

        // only create if we have a device
        if (m_device)
        {
            int msb = atts.value("msb").toInt();
            int lsb = atts.value("lsb").toInt();
            int program = atts.value("program").toInt();
            int pitch = atts.value("pitch").toInt();
            Rosegarden::InstrumentId instrument =
                atts.value("instrument").toInt();

            if (m_device->getType() == Rosegarden::Device::Midi)
            {
                // Modify metronome
                dynamic_cast<Rosegarden::MidiDevice*>(m_device)->
                        setMetronome(instrument, msb, lsb, program, pitch,
                           std::string("MIDI Metronome"));
            }
        }

    } else if (lcName == "instrument") {

        if (m_section != InStudio)
        {
            m_errorString = i18n("Found Instrument outside Studio");
            return false;
        }

        m_section = InInstrument;

        Rosegarden::InstrumentId id = atts.value("id").toInt();
        std::string stringType = qstrtostr(atts.value("type"));
        Rosegarden::Instrument::InstrumentType type;

        if (stringType == "midi")
            type = Rosegarden::Instrument::Midi;
        else if (stringType == "audio")
            type = Rosegarden::Instrument::Audio;
        else
        {
            m_errorString = i18n("Found unknown Instrument type");
            return false;
        }
            
        // Try and match an Instrument in the file with one in
        // our studio
        //
        Rosegarden::Instrument *instrument = m_studio.getInstrumentById(id);

        // If we've got an instrument and the types match then
        // we use it from now on.
        //
        if (instrument && instrument->getType() == type)
        {
            m_instrument = instrument;

            // We can also get the channel from this tag
            //
            Rosegarden::MidiByte channel =
                    (Rosegarden::MidiByte)atts.value("channel").toInt();
            m_instrument->setMidiChannel(channel);
        }

    } else if (lcName == "audiofiles") {

        if (m_section != NoSection)
        {
            m_errorString = i18n("Found AudioFiles inside another section");
            return false;
        }

        m_section = InAudio;


    } else {
        kdDebug(KDEBUG_AREA) << "RoseXmlHandler::startElement : Don't know how to parse this : " << qName << endl;
    }

    return true;
}

bool
RoseXmlHandler::endElement(const QString& /*namespaceURI*/,
                           const QString& /*localName*/, const QString& qName)
{
    // Set percentage done
    //
    if (m_progress &&
	(m_totalElements > m_elementsSoFar) &&
	(++m_elementsSoFar % 50 == 0)) {
	m_progress->setCompleted
	    (int(double(m_elementsSoFar) / double(m_totalElements) * 100.0));
	m_progress->processEvents();
    }

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
    } else if (lcName == "instrument") {

        m_section = InStudio;
        m_instrument = 0;

    } else if (lcName == "device") {

        m_device = 0;
    } else if (lcName == "audiofiles") {

        m_section = NoSection;

    }


    return true;
}

bool
RoseXmlHandler::characters(const QString& string)
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
    m_errorString = QString("%1 at line %2, column %3")
	.arg(exception.message())
	.arg(exception.lineNumber())
	.arg(exception.columnNumber());
    return QXmlDefaultHandler::error( exception );
}

bool
RoseXmlHandler::fatalError(const QXmlParseException& exception)
{
    m_errorString = QString("%1 at line %2, column %3")
	.arg(exception.message())
	.arg(exception.lineNumber())
	.arg(exception.columnNumber());
    return QXmlDefaultHandler::fatalError( exception );
}

bool
RoseXmlHandler::endDocument()
{
  if (m_foundTempo == false)
    m_composition.setDefaultTempo(120.0);

  return true;
}


