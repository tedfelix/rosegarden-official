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

#include "rosestrings.h"
#include "rosedebug.h"
#include "rosexmlhandler.h"
#include "rosegardengui.h"
#include "xmlstorableevent.h"
#include "SegmentNotationHelper.h"
#include "BaseProperties.h"
#include "Track.h"

#include "MidiDevice.h"
#include "AudioDevice.h"
#include "MappedStudio.h"
#include "Instrument.h"
#include "widgets.h"
#include "rosestrings.h"
#include "dialogs.h"
#include "audiopluginmanager.h"
#include "studiocontrol.h"

#include <klocale.h>
#include <kmessagebox.h>

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

class XmlSubHandler
{
public:
    XmlSubHandler();
    virtual ~XmlSubHandler();
    
    virtual bool startElement(const QString& namespaceURI,
                              const QString& localName,
                              const QString& qName,
                              const QXmlAttributes& atts) = 0;

    /**
     * @param finished : if set to true on return, means that
     * the handler should be deleted
     */
    virtual bool endElement(const QString& namespaceURI,
                            const QString& localName,
                            const QString& qName,
                            bool& finished) = 0;

    virtual bool characters(const QString& ch) = 0;
};

XmlSubHandler::XmlSubHandler()
{
}

XmlSubHandler::~XmlSubHandler()
{
}

//----------------------------------------

class ConfigurationXmlSubHandler : public XmlSubHandler
{
public:
    ConfigurationXmlSubHandler(const QString &elementName,
			       Rosegarden::Configuration *configuration);
    
    virtual bool startElement(const QString& namespaceURI,
                              const QString& localName,
                              const QString& qName,
                              const QXmlAttributes& atts);

    virtual bool endElement(const QString& namespaceURI,
                            const QString& localName,
                            const QString& qName,
                            bool& finished);

    virtual bool characters(const QString& ch);

    //--------------- Data members ---------------------------------

    Rosegarden::Configuration *m_configuration;

    QString m_elementName;
    QString m_propertyName;
    QString m_propertyType;
};

ConfigurationXmlSubHandler::ConfigurationXmlSubHandler(const QString &elementName,
						       Rosegarden::Configuration *configuration)
    : m_configuration(configuration),
      m_elementName(elementName)
{
}

bool ConfigurationXmlSubHandler::startElement(const QString&, const QString&,
                                              const QString& lcName,
                                              const QXmlAttributes& atts)
{
    m_propertyName = lcName;
    m_propertyType = atts.value("type");

    if (m_propertyName == "property") {
	// handle alternative encoding for properties with arbitrary names
	m_propertyName = atts.value("name");
	QString value = atts.value("value");
	if (value) {
	    m_propertyType = "String";
	    m_configuration->set<String>(qstrtostr(m_propertyName),
					 qstrtostr(value));
	}
    }

    return true;
}

bool ConfigurationXmlSubHandler::characters(const QString& chars)
{
    QString ch = chars.stripWhiteSpace();
    // this method is also called on newlines - skip these cases
    if (ch.isEmpty()) return true;


    if (m_propertyType == "Int") {
        long i = ch.toInt();
        RG_DEBUG << "\"" << m_propertyName << "\" "
                 << "value = " << i << endl;
        m_configuration->set<Int>(qstrtostr(m_propertyName), i);

        return true;
    }
    
    if (m_propertyType == "RealTime") {
        Rosegarden::RealTime rt;
        int sepIdx = ch.find(',');
        
        rt.sec = ch.left(sepIdx).toInt();
        rt.usec = ch.mid(sepIdx + 1).toInt();

        RG_DEBUG << "\"" << m_propertyName << "\" "
                 << "sec = " << rt.sec << ", usec = " << rt.usec << endl;

        m_configuration->set<Rosegarden::RealTimeT>(qstrtostr(m_propertyName), rt);

        return true;
    }

    if (m_propertyType == "Bool") {
        QString chLc = ch.lower();
        
        bool b = (chLc == "true" ||
                  chLc == "1"    ||
                  chLc == "on");
        
        m_configuration->set<Rosegarden::Bool>(qstrtostr(m_propertyName), b);

        return true;
    }

    if (!m_propertyType ||
	m_propertyType == "String") {
        
        m_configuration->set<Rosegarden::String>(qstrtostr(m_propertyName),
						 qstrtostr(ch));

        return true;
    }
    

    return true;
}

bool
ConfigurationXmlSubHandler::endElement(const QString&,
                                       const QString&,
                                       const QString& lcName,
                                       bool& finished)
{
    m_propertyName = "";
    m_propertyType = "";
    finished = (lcName == m_elementName);
    return true;
}


//----------------------------------------



RoseXmlHandler::RoseXmlHandler(RosegardenGUIDoc *doc,
                               unsigned int elementCount)
    : ProgressReporter(0),
      m_doc(doc),
      m_currentSegment(0),
      m_currentEvent(0),
      m_currentTime(0),
      m_chordDuration(0),
      m_segmentEndMarkerTime(0),
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
      m_plugin(0),
      m_pluginId(0),
      m_totalElements(elementCount),
      m_elementsSoFar(0),
      m_subHandler(0),
      m_deprecation(false),
      m_cancelled(false)
{
}

RoseXmlHandler::~RoseXmlHandler()
{
    delete m_subHandler;
}

bool
RoseXmlHandler::startDocument()
{
    // Clear tracks
    //
    getComposition().clearTracks();

    // And the loop
    //
    getComposition().setLoopStart(0);
    getComposition().setLoopEnd(0);

    // All plugins
    //
    m_doc->clearAllPlugins();

    // reset state
    return true;
}

bool
RoseXmlHandler::startElement(const QString& namespaceURI,
                             const QString& localName,
                             const QString& qName, const QXmlAttributes& atts)
{
    // First check if user pressed cancel button on the progress
    // dialog
    //
    if (isOperationCancelled()) {
        // Ideally, we'd throw here, but at this point Qt is in the stack
        // and Qt is very often compiled without exception support.
        //
        m_cancelled = true;
        return false;
    }

    QString lcName = qName.lower();

    if (getSubHandler()) {
        return getSubHandler()->startElement(namespaceURI, localName, lcName, atts);
    }

    if (lcName == "event") {

//        RG_DEBUG << "RoseXmlHandler::startElement: found event, current time is " << m_currentTime << endl;

        if (m_currentEvent) {
            RG_DEBUG << "RoseXmlHandler::startElement: Warning: new event found at time " << m_currentTime << " before previous event has ended; previous event will be lost" << endl;
            delete m_currentEvent;
        }

        m_currentEvent = new XmlStorableEvent(atts, m_currentTime);

	if (m_currentEvent->has(BEAMED_GROUP_ID)) {
	    
	    // remap -- we want to ensure that the segment's nextId
	    // is always used (and incremented) in preference to the
	    // stored id
	    
	    if (!m_currentSegment) {
		m_errorString = i18n("Got grouped event outside of a segment");
		return false;
	    }
	    
	    long storedId = m_currentEvent->get<Int>(BEAMED_GROUP_ID);

	    if (m_groupIdMap.find(storedId) == m_groupIdMap.end()) {
		m_groupIdMap[storedId] = m_currentSegment->getNextId();
	    }
	    
	    m_currentEvent->set<Int>(BEAMED_GROUP_ID, m_groupIdMap[storedId]);

        } else if (m_inGroup) {
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

//            RG_DEBUG << "RoseXmlHandler::startElement: (we're not in a chord) " << endl;

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
            RG_DEBUG << "RoseXmlHandler::startElement: Warning: Found property outside of event at time " << m_currentTime << ", ignoring" << endl;
        } else {
            m_currentEvent->setPropertyFromAttributes(atts, true);
        }

    } else if (lcName == "nproperty") {
        
        if (!m_currentEvent) {
            RG_DEBUG << "RoseXmlHandler::startElement: Warning: Found nproperty outside of event at time " << m_currentTime << ", ignoring" << endl;
        } else {
            m_currentEvent->setPropertyFromAttributes(atts, false);
        }

    } else if (lcName == "chord") {

        m_inChord = true;

    } else if (lcName == "group") {

        if (!m_currentSegment) {
            m_errorString = i18n("Got group outside of a segment");
            return false;
        }
        
	m_deprecation = true;
	RG_DEBUG << "RoseXmlHandler::startElement: Warning: group element is deprecated, recommend re-saving file from this version of Rosegarden to assure your ability to re-load it in future versions" << endl;

        m_inGroup = true;
        m_groupId = m_currentSegment->getNextId();
        m_groupType = qstrtostr(atts.value("type"));

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
        getStudio().clearMidiBanksAndPrograms();
        m_section = InStudio; // set top level section

        // Get and set MIDI filters
        //
        QString thruStr = atts.value("thrufilter");

        if (thruStr)
            getStudio().setMIDIThruFilter(thruStr.toInt());

        QString recordStr = atts.value("recordfilter");

        if (recordStr)
            getStudio().setMIDIRecordFilter(recordStr.toInt());

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

	getComposition().addTimeSignature
	    (t, Rosegarden::TimeSignature(num, denom, common, hidden));

    } else if (lcName == "tempo") {

	timeT t = 0;
	QString timeStr = atts.value("time");
	if (timeStr) t = timeStr.toInt();
	
	int bph = 120 * 60; // arbitrary
	QString bphStr = atts.value("bph");
	if (bphStr) bph = bphStr.toInt();

	getComposition().addRawTempo(t, bph);

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

        getComposition().setRecordTrack(recordTrack);

        // Get and ste the position pointer
        //
        int position = 0;
        QString positionStr = atts.value("pointer");
        if (positionStr) {
            position = positionStr.toInt();
        }

        getComposition().setPosition(position);


        // Get and (eventually) set the tempo
        //
        double tempo = 120.0;
        QString tempoStr = atts.value("defaultTempo");
        if (tempoStr) {
            tempo = tempoStr.toDouble();
        }

        getComposition().setDefaultTempo(tempo);
        
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

            getComposition().setLoopStart(loopStart);
            getComposition().setLoopEnd(loopEnd);
        }

        QString selectedTrackStr = atts.value("selected");

        if (selectedTrackStr)
        {
            Rosegarden::TrackId selectedTrack =
                (Rosegarden::TrackId)selectedTrackStr.toInt();

            getComposition().setSelectedTrack(selectedTrack);
        }

        QString soloTrackStr = atts.value("solo");
        if (soloTrackStr)
        {
            if (soloTrackStr.toInt() == 1)
                getComposition().setSolo(true);
            else
                getComposition().setSolo(false);
        }

	QString copyrightStr = atts.value("copyright");
	if (copyrightStr)
	{
	    getComposition().setCopyrightNote(qstrtostr(copyrightStr));
	}

        QString startMarkerStr = atts.value("startMarker");
        QString endMarkerStr = atts.value("endMarker");

        if (startMarkerStr)
        {
            getComposition().setStartMarker(startMarkerStr.toInt());
        }

        if (endMarkerStr)
        {
            getComposition().setEndMarker(endMarkerStr.toInt());
        }

    } else if (lcName == "track") {

        if (m_section != InComposition)
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
        getComposition().addTrack(track);


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
                int audioFileId = atts.value("file").toInt();

                // check this file id exists on the AudioFileManager

                if(getAudioFileManager().fileExists(audioFileId) == false)
                {
                    // We don't report an error as this audio file might've
                    // been excluded deliberately as we could't actually
                    // find the audio file itself.
                    //
                    return true;
                }

                // Create an Audio segment and add its reference
                //
                m_currentSegment = new Segment(Rosegarden::Segment::Audio);
                m_currentSegment->setAudioFileId(audioFileId);
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
    
        QString repeatStr = atts.value("repeat");
        if (repeatStr.lower() == "true") {
            m_currentSegment->setRepeating(true);
        }

        QString delayStr = atts.value("delay");
        if (delayStr) {
	    RG_DEBUG << "Delay string is \"" << delayStr << "\"" << endl;
	    long delay = delayStr.toLong();
	    RG_DEBUG << "Delay is " << delay << endl;
	    m_currentSegment->setDelay(delay);
	}

	QString rtDelayuSec = atts.value("rtdelayusec");
	QString rtDelaySec = atts.value("rtdelaysec");
	if (rtDelaySec && rtDelayuSec) {
	    m_currentSegment->setRealTimeDelay
		(Rosegarden::RealTime(rtDelaySec.toLong(),
				      rtDelayuSec.toLong()));
	}

        QString transposeStr = atts.value("transpose");
        if (transposeStr) m_currentSegment->setTranspose(transposeStr.toInt());

	// fill in the label
        QString labelStr = atts.value("label");
        if (labelStr)
            m_currentSegment->setLabel(qstrtostr(labelStr));
        
        m_currentSegment->setTrack(track);
        m_currentSegment->setStartTime(startTime);
	m_currentTime = startTime;
        getComposition().addSegment(m_currentSegment);

	QString endMarkerStr = atts.value("endmarker");
	if (endMarkerStr) {
	    delete m_segmentEndMarkerTime;
	    m_segmentEndMarkerTime = new timeT(endMarkerStr.toInt());
	}

	m_groupIdMap.clear();

    } else if (lcName == "resync") {

	m_deprecation = true;
	RG_DEBUG << "RoseXmlHandler::startElement: Warning: resync element is deprecated, recommend re-saving file from this version of Rosegarden to assure your ability to re-load it in future versions" << endl;
	
	QString time(atts.value("time"));
	bool isNumeric;
	int numTime = time.toInt(&isNumeric);
	if (isNumeric) m_currentTime = numTime;

    } else if (lcName == "audio") {

        if (m_section != InAudioFiles)
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
        if(getAudioFileManager().insertFile(qstrtostr(label),
                                         qstrtostr(file),
                                         id.toInt()) == false)
        {
            // Freeze the progress dialog
            CurrentProgressDialog::freeze();

            // Create a locate file dialog - give it the file name
            // and the AudioFileManager path that we've already
            // tried.  If we manually locate the file then we reset
            // the audiofilepath to the new value and see if this
            // helps us locate the rest of the files.
            //

            QString newFilename = "";
            QString newPath = "";
            
            do
            {

                FileLocateDialog *fL =
                    new FileLocateDialog((RosegardenGUIApp *)m_doc->parent(),
                        file,
                        QString(getAudioFileManager().getAudioPath().c_str()));

                if(fL->exec() == QDialog::Accepted)
                {
                    newFilename = fL->getFilename();
                    newPath = fL->getDirectory();
                }
                else
                {
                    // just skip the file
                    break;
                }

            } while(getAudioFileManager().insertFile(qstrtostr(label),
                                            qstrtostr(newFilename),
                                            id.toInt()) == false);

            if (newPath != "")
                getAudioFileManager().setAudioPath(qstrtostr(newPath));

            getAudioFileManager().print();

            // Restore progress dialog's normal state
            CurrentProgressDialog::thaw();

        }
        
    } else if (lcName == "audiopath") {

        if (m_section != InAudioFiles)
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

        getAudioFileManager().setAudioPath(qstrtostr(search));

    } else if (lcName == "begin") {
        float marker = atts.value("index").toFloat();

        if (!m_currentSegment)
        {
            // Don't fail - as this segment could be defunct if we
            // skipped loading the audio file
            //
            return true;
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
            // Don't fail - as this segment could be defunct if we
            // skipped loading the audio file
            //
            return true;
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

        // Ensure we set end time according to correct RealTime end of Segment
        //
        Rosegarden::RealTime realEndTime = getComposition().
                getElapsedRealTime(m_currentSegment->getStartTime()) +
                m_currentSegment->getAudioEndTime() -
                m_currentSegment->getAudioStartTime();

        timeT absEnd = getComposition().getElapsedTimeForRealTime(realEndTime);
	m_currentSegment->setEndTime(absEnd);

    } else if (lcName == "device") {

        if (m_section != InStudio)
        {
            m_errorString = i18n("Found Device outside Studio");
            return false;
        }

        QString type = (atts.value("type")).lower();
        QString idString = atts.value("id");
        QString nameStr = atts.value("name");
            
        if (idString.isNull())
        {
            m_errorString = i18n("No ID on Device tag");
            return false;
        }
        int id = idString.toInt();

        if (type == "midi")
        {
            m_device = getStudio().getDevice(id);

            if (m_device && m_device->getType() == Rosegarden::Device::Midi) {
		if (nameStr && nameStr != "") {
		    m_device->setName(qstrtostr(nameStr));
		}
	    } else if (nameStr && nameStr != "") {
		//!!! add device
	    }
        }
        else if (type == "audio")
        {
            m_device = getStudio().getDevice(id);

            if (m_device && m_device->getType() == Rosegarden::Device::Audio)
                m_device->setName(qstrtostr(nameStr));
        }
        else
        {
            m_errorString = i18n("Found unknown Device type");
            return false;
        }

    } else if (lcName == "librarian") {

        // The contact details for the maintainer of the banks/programs
        // information.
        //
        if (m_device && m_device->getType() == Rosegarden::Device::Midi)
        {
            QString name = atts.value("name");
            QString email = atts.value("email");

            dynamic_cast<Rosegarden::MidiDevice*>(m_device)->
                setLibrarian(qstrtostr(name), qstrtostr(email));
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

    } else if (lcName == "reverb") {

        if (m_section != InInstrument)
        {
            m_errorString = i18n("Found Reverb outside Instrument");
            return false;
        }

        Rosegarden::MidiByte value = atts.value("value").toInt();

        if (m_instrument)
            m_instrument->setReverb(value);


    } else if (lcName == "chorus") {

        if (m_section != InInstrument)
        {
            m_errorString = i18n("Found Chorus outside Instrument");
            return false;
        }

        Rosegarden::MidiByte value = atts.value("value").toInt();

        if (m_instrument)
            m_instrument->setChorus(value);

    } else if (lcName == "filter") {

        if (m_section != InInstrument)
        {
            m_errorString = i18n("Found Filter outside Instrument");
            return false;
        }

        Rosegarden::MidiByte value = atts.value("value").toInt();

        if (m_instrument)
            m_instrument->setFilter(value);


    } else if (lcName == "resonance") {

        if (m_section != InInstrument)
        {
            m_errorString = i18n("Found Resonance outside Instrument");
            return false;
        }

        Rosegarden::MidiByte value = atts.value("value").toInt();

        if (m_instrument)
            m_instrument->setResonance(value);


    } else if (lcName == "attack") {

        if (m_section != InInstrument)
        {
            m_errorString = i18n("Found Attack outside Instrument");
            return false;
        }

        Rosegarden::MidiByte value = atts.value("value").toInt();

        if (m_instrument)
            m_instrument->setAttack(value);

    } else if (lcName == "release") {

        if (m_section != InInstrument)
        {
            m_errorString = i18n("Found Release outside Instrument");
            return false;
        }

        Rosegarden::MidiByte value = atts.value("value").toInt();

        if (m_instrument)
            m_instrument->setRelease(value);

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

        // keep "velocity" so we're backwards compatible
    } else if (lcName == "velocity" || lcName == "volume") {

        if (m_section != InInstrument)
        {
            m_errorString = i18n("Found Volume outside Instrument");
            return false;
        }

        Rosegarden::MidiByte value = atts.value("value").toInt();

        if (m_instrument)
        {
            m_instrument->setVelocity(value);
            m_instrument->setSendVelocity(true);
        }

#ifdef HAVE_LADSPA

    } else if (lcName == "plugin") {

        if (m_section != InInstrument)
        {
            m_errorString = i18n("Found Plugin outside Instrument");
            return false;
        }

        // get details
        int position = atts.value("position").toInt();
        unsigned long id = atts.value("id").toULong();
        QString bpStr = atts.value("bypassed");
        bool bypassed = false;

        if (bpStr.lower() == "true")
            bypassed = true;

        // Check that ID exists
        //
        Rosegarden::AudioPlugin *plugin = 0;
        if (getAudioPluginManager())
            plugin = getAudioPluginManager()->getPluginByUniqueId(id);

        // If we find the plugin all is well and good but if
        // we don't we just skip it.
        //
#ifdef HAVE_LADSPA
	
        if (plugin)
        {
            m_plugin = m_instrument->getPlugin(position);
            m_plugin->setAssigned(true);
            m_plugin->setBypass(bypassed);

            // Creating the Plugin creates the ports too
            //
            Rosegarden::MappedObjectId mappedId =
                Rosegarden::StudioControl::createStudioObject(
                        Rosegarden::MappedObject::LADSPAPlugin);

            m_plugin->setMappedId(mappedId);
            m_plugin->setId(id);

            Rosegarden::StudioControl::setStudioObjectProperty
                (mappedId,
                 Rosegarden::MappedObject::Instrument,
                 m_instrument->getId());

            Rosegarden::StudioControl::setStudioObjectProperty
                (mappedId,
                 Rosegarden::MappedObject::Position,
                 Rosegarden::MappedObjectValue(position));

            // Setting the id also sets up the plugin so that it's
            // ready to run.
            // 
            Rosegarden::StudioControl::setStudioObjectProperty
                (mappedId,
                 Rosegarden::MappedLADSPAPlugin::UniqueId,
                 Rosegarden::MappedObjectValue(id));

            m_section = InPlugin;
        }
        else
        {
            m_errorString = i18n("Can't find Plugin");
            return false;
        }

#endif

    } else if (lcName == "port") {

        if (m_section != InPlugin)
        {
            m_errorString = i18n("Found Port outside Plugin");
            return false;
        }
        unsigned long portId = atts.value("id").toULong();
        Rosegarden::MappedObjectValue value = atts.value("value").toFloat();

        if (m_plugin)
        {
            // Set the port at the sequencer and in the AudioPluginInstance
            //
            Rosegarden::StudioControl::setStudioPluginPort
                (m_plugin->getMappedId(),
                 portId,
                 value);

            m_plugin->addPort(portId, value);
        }
        else
        {
            m_errorString = i18n("No Plugin object found for Port");
            return false;
        }

#endif // HAVE_LADSPA

    } else if (lcName == "metronome") {

        if (m_section != InStudio)
        {
            m_errorString = i18n("Found Metronome outside Studio");
            return false;
        }

        // Only create if we have a device and we don't already
        // have a metronome.
        //
        if (m_device && getStudio().getMetronome() == 0)
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
        Rosegarden::Instrument *instrument = getStudio().getInstrumentById(id);

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

        m_section = InAudioFiles;


    } else if (lcName == "configuration") {

        setSubHandler(new ConfigurationXmlSubHandler
		      (lcName, &m_doc->getConfiguration()));

    } else if (lcName == "metadata") {
	
	if (m_section != InComposition) {
            m_errorString = i18n("Found Metadata outside Composition");
            return false;
        }

        setSubHandler(new ConfigurationXmlSubHandler
		      (lcName, &getComposition().getMetadata()));

    } else {
        RG_DEBUG << "RoseXmlHandler::startElement : Don't know how to parse this : " << qName << endl;
    }

    return true;
}

bool
RoseXmlHandler::endElement(const QString& namespaceURI,
                           const QString& localName,
                           const QString& qName)
{
    if (getSubHandler()) {
        bool finished;
        bool res = getSubHandler()->endElement(namespaceURI, localName, qName.lower(), finished);
        if (finished) setSubHandler(0);
        return res;
    }

    // Set percentage done
    //
    if ((m_totalElements > m_elementsSoFar) &&
	(++m_elementsSoFar % 100 == 0)) {

        emit setProgress(int(double(m_elementsSoFar) / double(m_totalElements) * 100.0));
        kapp->processEvents();
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

	if (m_currentSegment && m_segmentEndMarkerTime) {
	    m_currentSegment->setEndMarkerTime(*m_segmentEndMarkerTime);
	    delete m_segmentEndMarkerTime;
	    m_segmentEndMarkerTime = 0;
	}

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

    } else if (lcName == "plugin") {

        m_section = InInstrument;
        m_plugin = 0;
        m_pluginId = 0;

    } else if (lcName == "device") {

        m_device = 0;
    } else if (lcName == "audiofiles") {

        m_section = NoSection;

    }


    return true;
}

bool
RoseXmlHandler::characters(const QString& s)
{
    if (m_subHandler)
        return m_subHandler->characters(s);

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
    if (m_foundTempo == false) getComposition().setDefaultTempo(120.0);

    return true;
}


void
RoseXmlHandler::setSubHandler(XmlSubHandler* sh)
{
    delete m_subHandler;
    m_subHandler = sh;
}
