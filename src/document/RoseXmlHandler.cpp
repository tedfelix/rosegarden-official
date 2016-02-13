/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[RoseXmlHandler]"

#include "RoseXmlHandler.h"

#include "sound/Midi.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/AudioLevel.h"
#include "base/AudioPluginInstance.h"
#include "base/BaseProperties.h"
#include "base/Colour.h"
#include "base/ColourMap.h"
#include "base/Composition.h"
#include "base/ControlParameter.h"
#include "base/Device.h"
#include "base/Instrument.h"
#include "base/Marker.h"
#include "base/MidiDevice.h"
#include "base/SoftSynthDevice.h"
#include "base/MidiProgram.h"
#include "base/MidiTypes.h"
#include "base/NotationTypes.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "base/SegmentLinker.h"
#include "base/Studio.h"
#include "base/Track.h"
#include "base/TriggerSegment.h"
#include "base/Profiler.h"
#include "gui/application/RosegardenMainWindow.h"
#include "sequencer/RosegardenSequencer.h"
#include "gui/dialogs/FileLocateDialog.h"
#include "gui/general/ProgressReporter.h"
#include "gui/widgets/StartupLogo.h"
#include "gui/studio/AudioPlugin.h"
#include "gui/studio/AudioPluginManager.h"
#include "gui/widgets/CurrentProgressDialog.h"
#include "gui/widgets/ProgressDialog.h"
#include "RosegardenDocument.h"
#include "sound/AudioFileManager.h"
#include "XmlStorableEvent.h"
#include "XmlSubHandler.h"

#include <QMessageBox>
#include <QByteArray>
#include <QDataStream>
#include <QDialog>
#include <QFileInfo>
#include <QString>
#include <QStringList>

namespace Rosegarden
{

using namespace BaseProperties;

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
        if (!value.isEmpty()) {
            m_propertyType = "String";
            m_configuration->set<String>(qstrtostr(m_propertyName),
                                         qstrtostr(value));
        }
    }

    return true;
}

bool ConfigurationXmlSubHandler::characters(const QString& chars)
{
    QString ch = chars.trimmed();
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
        int sepIdx = ch.indexOf(',');
        
        rt.sec = ch.left(sepIdx).toInt();
        rt.nsec = ch.mid(sepIdx + 1).toInt();

        RG_DEBUG << "\"" << m_propertyName << "\" "
                 << "sec = " << rt.sec << ", nsec = " << rt.nsec << endl;

        m_configuration->set<Rosegarden::RealTimeT>(qstrtostr(m_propertyName), rt);

        return true;
    }

    if (m_propertyType == "Bool") {
        QString chLc = ch.toLower();
        
        bool b = (chLc == "true" ||
                  chLc == "1"    ||
                  chLc == "on");
        
        m_configuration->set<Rosegarden::Bool>(qstrtostr(m_propertyName), b);

        return true;
    }

    if (m_propertyType.isEmpty() ||
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



RoseXmlHandler::RoseXmlHandler(RosegardenDocument *doc,
                               unsigned int elementCount,
                               bool createNewDevicesWhenNeeded) :
    ProgressReporter(0),
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
    m_deviceRunningId(Device::NO_DEVICE),
    m_deviceInstrumentBase(MidiInstrumentBase),
    m_deviceReadInstrumentBase(0),
    m_sendBankSelect(false),
    m_msb(0),
    m_lsb(0),
    m_instrument(0),
    m_plugin(0),
    m_pluginInBuss(false),
    m_colourMap(0),
    m_keyMapping(0),
    m_pluginId(0),
    m_totalElements(elementCount),
    m_elementsSoFar(0),
    m_subHandler(0),
    m_deprecation(false),
    m_createDevices(createNewDevicesWhenNeeded),
    m_haveControls(false),
    m_cancelled(false),
    m_skipAllAudio(false),
    m_hasActiveAudio(false)

{}

RoseXmlHandler::~RoseXmlHandler()
{
    delete m_subHandler;
}

Composition &
RoseXmlHandler::getComposition()
{
    return m_doc->getComposition();
}

Studio &
RoseXmlHandler::getStudio()
{
    return m_doc->getStudio();
}

AudioFileManager &
RoseXmlHandler::getAudioFileManager()
{
    return m_doc->getAudioFileManager();
}

AudioPluginManager *
RoseXmlHandler::getAudioPluginManager()
{
    return m_doc->getPluginManager();
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
    // First check if user pressed cancel button on the value()
    // dialog
    //
    if (isOperationCancelled()) {
        // Ideally, we'd throw here, but at this point Qt is in the stack
        // and Qt is very often compiled without exception support.
        //
        m_cancelled = true;
        return false;
    }

    QString lcName = qName.toLower();

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
                m_errorString = "Got grouped event outside of a segment";
                return false;
            }

            long storedId = m_currentEvent->get
                            <Int>(BEAMED_GROUP_ID);

            if (m_groupIdMap.find(storedId) == m_groupIdMap.end()) {
                m_groupIdMap[storedId] = m_currentSegment->getNextId();
            }

            m_currentEvent->set
            <Int>(BEAMED_GROUP_ID, m_groupIdMap[storedId]);

        } else if (m_inGroup) {
            m_currentEvent->set
            <Int>(BEAMED_GROUP_ID, m_groupId);
            m_currentEvent->set
            <String>(BEAMED_GROUP_TYPE, m_groupType);
            if (m_groupType == GROUP_TYPE_TUPLED) {
                m_currentEvent->set
                <Int>
                (BEAMED_GROUP_TUPLET_BASE, m_groupTupletBase);
                m_currentEvent->set
                <Int>
                (BEAMED_GROUP_TUPLED_COUNT, m_groupTupledCount);
                m_currentEvent->set
                <Int>
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
            m_errorString = "Got group outside of a segment";
            return false;
        }

        if (!m_deprecation)
            std::cerr << "WARNING: This Rosegarden file uses the deprecated element \"group\".  We recommend re-saving the file from this version of Rosegarden to assure your ability to re-load it in future versions" << std::endl;
        m_deprecation = true;

        m_inGroup = true;
        m_groupId = m_currentSegment->getNextId();
        m_groupType = qstrtostr(atts.value("type"));

        if (m_groupType == GROUP_TYPE_TUPLED) {
            m_groupTupletBase = atts.value("base").toInt();
            m_groupTupledCount = atts.value("tupled").toInt();
            m_groupUntupledCount = atts.value("untupled").toInt();
        }

    } else if (lcName == "rosegarden-data") {

        // FILE FORMAT VERSIONING -- see comments in
        // RosegardenDocument.cpp.  We only care about major and minor
        // here, not point.

        QString version = atts.value("version");
        QString smajor = atts.value("format-version-major");
        QString sminor = atts.value("format-version-minor");

//        std::cerr << "\n\n\nRosegarden file version = \"" << version << "\"\n\n\n" << std::endl;

        if (!smajor.isEmpty()) {

            int major = smajor.toInt();
            int minor = sminor.toInt();

            if (major > RosegardenDocument::FILE_FORMAT_VERSION_MAJOR) {
                m_errorString = tr("This file was written by Rosegarden %1, and it uses\na different file format that cannot be read by this version.").arg(version);
                return false;
            }

            if (major == RosegardenDocument::FILE_FORMAT_VERSION_MAJOR &&
                    minor > RosegardenDocument::FILE_FORMAT_VERSION_MINOR) {

                CurrentProgressDialog::freeze();
                StartupLogo::hideIfStillThere();

                QMessageBox::information(0, tr("Rosegarden"), tr("This file was written by Rosegarden %1, which is more recent than this version.\nThere may be some incompatibilities with the file format.").arg(version));

                CurrentProgressDialog::thaw();
            }
        }

    } else if (lcName == "studio") {

        if (m_section != NoSection) {
            m_errorString = "Found Studio in another section";
            return false;
        }

        // In the Studio we clear down everything apart from Devices and
        // Instruments before we reload.  Instruments are derived from
        // the Sequencer, the bank/program information is loaded from
        // the file we're currently examining.
        //
        getStudio().clearMidiBanksAndPrograms();
        getStudio().clearBusses();
        getStudio().clearRecordIns();

        m_section = InStudio; // set top level section

        // Get and set MIDI filters
        //
        QString thruStr = atts.value("thrufilter");

        if (!thruStr.isEmpty())
            getStudio().setMIDIThruFilter(thruStr.toInt());

        QString recordStr = atts.value("recordfilter");

        if (!recordStr.isEmpty())
            getStudio().setMIDIRecordFilter(recordStr.toInt());

        QString inputStr = atts.value("audioinputpairs");

        if (!inputStr.isEmpty()) {
            int inputs = inputStr.toInt();
            if (inputs < 1)
                inputs = 1; // we simply don't permit no inputs
            while (int(getStudio().getRecordIns().size()) < inputs) {
                getStudio().addRecordIn(new RecordIn());
            }
        }

        QString mixerStr = atts.value("mixerdisplayoptions");

        if (!mixerStr.isEmpty()) {
            unsigned int mixer = mixerStr.toUInt();
            getStudio().setMixerDisplayOptions(mixer);
        }

        QString metronomeStr = atts.value("metronomedevice");

        if (!metronomeStr.isEmpty()) {
            DeviceId metronome = metronomeStr.toUInt();
            getStudio().setMetronomeDevice(metronome);
        }

    } else if (lcName == "timesignature") {

        if (m_inComposition == false) {
            m_errorString = "TimeSignature object found outside Composition";
            return false;
        }

        timeT t = 0;
        QString timeStr = atts.value("time");
        if (!timeStr.isEmpty())
            t = timeStr.toInt();

        int num = 4;
        QString numStr = atts.value("numerator");
        if (!numStr.isEmpty())
            num = numStr.toInt();

        int denom = 4;
        QString denomStr = atts.value("denominator");
        if (!denomStr.isEmpty())
            denom = denomStr.toInt();

        bool common = false;
        QString commonStr = atts.value("common");
        if (!commonStr.isEmpty())
            common = (commonStr == "true");

        bool hidden = false;
        QString hiddenStr = atts.value("hidden");
        if (!hiddenStr.isEmpty())
            hidden = (hiddenStr == "true");

        bool hiddenBars = false;
        QString hiddenBarsStr = atts.value("hiddenbars");
        if (!hiddenBarsStr.isEmpty())
            hiddenBars = (hiddenBarsStr == "true");

        getComposition().addTimeSignature
        (t, TimeSignature(num, denom, common, hidden, hiddenBars));

    } else if (lcName == "tempo") {

        timeT t = 0;
        QString timeStr = atts.value("time");
        if (!timeStr.isEmpty())
            t = timeStr.toInt();

        tempoT tempo = Composition::getTempoForQpm(120.0);
        QString tempoStr = atts.value("tempo");
        QString targetStr = atts.value("target");
        QString bphStr = atts.value("bph");
        if (!tempoStr.isEmpty()) {
            tempo = tempoStr.toInt();
        } else if (!bphStr.isEmpty()) {
            tempo = Composition::getTempoForQpm
                    (double(bphStr.toInt()) / 60.0);
        }

        if (!targetStr.isEmpty()) {
            getComposition().addTempoAtTime(t, tempo, targetStr.toInt());
        } else {
            getComposition().addTempoAtTime(t, tempo);
        }

    } else if (lcName == "composition") {

        if (m_section != NoSection) {
            m_errorString = "Found Composition in another section";
            return false;
        }

        // set Segment
        m_section = InComposition;

        // Get and set the record track
        //
        QString recordStr = atts.value("recordtrack");
        if (!recordStr.isEmpty()) {
            getComposition().setTrackRecording(recordStr.toInt(), true);
        }

        QString recordPlStr = atts.value("recordtracks");
        if (!recordPlStr.isEmpty()) {
            RG_DEBUG << "Record tracks: " << recordPlStr << endl;
            QStringList recordList = recordPlStr.split(',');
            for (QStringList::iterator i = recordList.begin();
                    i != recordList.end(); ++i) {
                RG_DEBUG << "Record track: " << (*i).toInt() << endl;
                getComposition().setTrackRecording((*i).toInt(), true);
            }
        }

        // Get and set the position pointer
        //
        int position = 0;
        QString positionStr = atts.value("pointer");
        if (!positionStr.isEmpty()) {
            position = positionStr.toInt();
        }

        getComposition().setPosition(position);


        // Get and (eventually) set the default tempo.
        // We prefer the new compositionDefaultTempo over the
        // older defaultTempo.
        //
        QString tempoStr = atts.value("compositionDefaultTempo");
        if (!tempoStr.isEmpty()) {
            tempoT tempo = tempoT(tempoStr.toInt());
            getComposition().setCompositionDefaultTempo(tempo);
        } else {
            tempoStr = atts.value("defaultTempo");
            if (!tempoStr.isEmpty()) {
                double tempo = qstrtodouble(tempoStr);
                getComposition().setCompositionDefaultTempo
                (Composition::getTempoForQpm(tempo));
            }
        }

        // set the composition flag
        m_inComposition = true;


        // Set the loop
        //
        QString loopStartStr = atts.value("loopstart");
        QString loopEndStr = atts.value("loopend");

        if (!loopStartStr.isEmpty() && !loopEndStr.isEmpty()) {
            int loopStart = loopStartStr.toInt();
            int loopEnd = loopEndStr.toInt();

            getComposition().setLoopStart(loopStart);
            getComposition().setLoopEnd(loopEnd);
        }

        QString selectedTrackStr = atts.value("selected");

        if (!selectedTrackStr.isEmpty()) {
            TrackId selectedTrack =
                (TrackId)selectedTrackStr.toInt();

            getComposition().setSelectedTrack(selectedTrack);
        }

        QString soloTrackStr = atts.value("solo");
        if (!soloTrackStr.isEmpty()) {
            if (soloTrackStr.toInt() == 1)
                getComposition().setSolo(true);
            else
                getComposition().setSolo(false);
        }


        QString playMetStr = atts.value("playmetronome");
        if (!playMetStr.isEmpty()) {
            if (playMetStr.toInt())
                getComposition().setPlayMetronome(true);
            else
                getComposition().setPlayMetronome(false);
        }

        QString recMetStr = atts.value("recordmetronome");
        if (!recMetStr.isEmpty()) {
            if (recMetStr.toInt())
                getComposition().setRecordMetronome(true);
            else
                getComposition().setRecordMetronome(false);
        }

        QString nextTriggerIdStr = atts.value("nexttriggerid");
        if (!nextTriggerIdStr.isEmpty()) {
            getComposition().setNextTriggerSegmentId(nextTriggerIdStr.toInt());
        }

        QString copyrightStr = atts.value("copyright");
        if (!copyrightStr.isEmpty()) {
            getComposition().setCopyrightNote(qstrtostr(copyrightStr));
        }

        QString startMarkerStr = atts.value("startMarker");
        QString endMarkerStr = atts.value("endMarker");

        if (!startMarkerStr.isEmpty()) {
            getComposition().setStartMarker(startMarkerStr.toInt());
        }

        if (!endMarkerStr.isEmpty()) {
            getComposition().setEndMarker(endMarkerStr.toInt());
        }

        QString autoExpand = atts.value("autoExpand");
        if (!autoExpand.isEmpty()) {
            if (autoExpand.toInt() == 1)
                getComposition().setAutoExpand(true);
            else
                getComposition().setAutoExpand(false);
        }

        QString panLawStr = atts.value("panlaw");
        if (!panLawStr.isEmpty()) {
            int panLaw = panLawStr.toInt();
            AudioLevel::setPanLaw(panLaw);
        } else {
            // Since no "panlaw" was found in this tag, apply the default.
            AudioLevel::setPanLaw(0);
        }

    } else if (lcName == "track") {

        if (m_section != InComposition) {
            m_errorString = "Track object found outside Composition";
            return false;
        }

        int id = -1;
        int position = -1;
        int instrument = -1;
        std::string label;
        bool muted = false;

        QString trackNbStr = atts.value("id");
        if (!trackNbStr.isEmpty()) {
            id = trackNbStr.toInt();
        }

        QString labelStr = atts.value("label");
        if (!labelStr.isEmpty()) {
            label = qstrtostr(labelStr);
        }

        QString mutedStr = atts.value("muted");
        if (!mutedStr.isEmpty()) {
            if (mutedStr == "true")
                muted = true;
            else
                muted = false;
        }

        QString positionStr = atts.value("position");
        if (!positionStr.isEmpty()) {
            position = positionStr.toInt();
        }

        QString instrumentStr = atts.value("instrument");
        if (!instrumentStr.isEmpty()) {
            instrument = instrumentStr.toInt();
        }

        Track *track = new Track(id,
                                 instrument,
                                 position,
                                 label,
                                 muted);

        QString shortLabelStr = atts.value("shortLabel");
        if (!shortLabelStr.isEmpty()) {
            track->setShortLabel(shortLabelStr.toStdString());
        }

        // track properties affecting newly created segments are initialized
        // to default values in the ctor, so they don't need to be initialized
        // here
        
        QString presetLabelStr = atts.value("defaultLabel");
        if (!labelStr.isEmpty()) {
            track->setPresetLabel( qstrtostr(presetLabelStr) );
        }
    
        QString clefStr = atts.value("defaultClef");
        if (!clefStr.isEmpty()) {
            track->setClef(clefStr.toInt());
        }

        QString transposeStr = atts.value("defaultTranspose");
        if (!transposeStr.isEmpty()) {
            track->setTranspose(transposeStr.toInt());
        }

        QString colorStr = atts.value("defaultColour");
        if (!colorStr.isEmpty()) {
            track->setColor(colorStr.toInt());
        }

        QString highplayStr = atts.value("defaultHighestPlayable");
        if (!highplayStr.isEmpty()) {
            track->setHighestPlayable(highplayStr.toInt());
        }

        QString lowplayStr = atts.value("defaultLowestPlayable");
        if (!lowplayStr.isEmpty()) {
            track->setLowestPlayable(lowplayStr.toInt());
        }

        QString staffSizeStr = atts.value("staffSize");
        if (!staffSizeStr.isEmpty()) {
            track->setStaffSize(staffSizeStr.toInt());
        }

        QString staffBracketStr = atts.value("staffBracket");
        if (!staffBracketStr.isEmpty()) {
            track->setStaffBracket(staffBracketStr.toInt());
        }

        QString inputDeviceStr = atts.value("inputDevice");
        if (!inputDeviceStr.isEmpty()) {
            track->setMidiInputDevice(inputDeviceStr.toUInt());
        }

        QString inputChannelStr = atts.value("inputChannel");
        if (!inputChannelStr.isEmpty()) {
            track->setMidiInputChannel(inputChannelStr.toInt());
        }

        // If the composition tag had this track set to record, make sure
        // it is armed.
        if (getComposition().isTrackRecording(id)) {
            track->setArmed(true);
        }

        getComposition().addTrack(track);

        std::vector<TrackId> trackIds;
        trackIds.push_back(track->getId());
        getComposition().notifyTracksAdded(trackIds);


    } else if (lcName == "segment") {

        if (m_section != NoSection) {
            m_errorString = "Found Segment in another section";
            return false;
        }

        // set Segment
        if(lcName == "segment") {
            m_section = InSegment;
        }

        int track = -1, startTime = 0;
        unsigned int colourindex = 0;
        QString trackNbStr = atts.value("track");
        if (!trackNbStr.isEmpty()) {
            track = trackNbStr.toInt();
        }

        QString startIdxStr = atts.value("start");
        if (!startIdxStr.isEmpty()) {
            startTime = startIdxStr.toInt();
        }

        QString segmentType = (atts.value("type")).toLower();
        if (!segmentType.isEmpty()) {
            if (segmentType == "audio") {
                int audioFileId = atts.value("file").toInt();

                // check this file id exists on the AudioFileManager

                if (getAudioFileManager().fileExists(audioFileId) == false) {
                    // We don't report an error as this audio file might've
                    // been excluded deliberately as we could't actually
                    // find the audio file itself.
                    //
                    return true;
                }

                // Create an Audio segment and add its reference
                //
                m_currentSegment = new Segment(Segment::Audio);
                m_currentSegment->setAudioFileId(audioFileId);
                m_currentSegment->setStartTime(startTime);
            } else {
                // Create a (normal) internal Segment
                m_currentSegment = new Segment(Segment::Internal);
            }

        } else {
            // for the moment we default
            m_currentSegment = new Segment(Segment::Internal);
        }

        QString repeatStr = atts.value("repeat");
        if (repeatStr.toLower() == "true") {
            m_currentSegment->setRepeating(true);
        }

        QString delayStr = atts.value("delay");
        if (!delayStr.isEmpty()) {
            RG_DEBUG << "Delay string is \"" << delayStr << "\"" << endl;
            long delay = delayStr.toLong();
            RG_DEBUG << "Delay is " << delay << endl;
            m_currentSegment->setDelay(delay);
        }

        QString rtDelaynSec = atts.value("rtdelaynsec");
        QString rtDelayuSec = atts.value("rtdelayusec");
        QString rtDelaySec = atts.value("rtdelaysec");
        if ( (!rtDelaySec.isEmpty()) && ((!rtDelaynSec.isEmpty()) || (!rtDelayuSec.isEmpty())) ){
            if (!rtDelaynSec.isEmpty()) {
                m_currentSegment->setRealTimeDelay(
                            RealTime(rtDelaySec.toInt(),
                                          rtDelaynSec.toInt()));
            } else {
                m_currentSegment->setRealTimeDelay
                (RealTime(rtDelaySec.toInt(),
                          rtDelayuSec.toInt() * 1000));
            }
        }

        QString transposeStr = atts.value("transpose");
        if (!transposeStr.isEmpty())
            m_currentSegment->setTranspose(transposeStr.toInt());

        // fill in the label
        QString labelStr = atts.value("label");
        if (!labelStr.isEmpty())
            m_currentSegment->setLabel(qstrtostr(labelStr));

        m_currentSegment->setTrack(track);
        //m_currentSegment->setStartTime(startTime);

        QString colourIndStr = atts.value("colourindex");
        if (!colourIndStr.isEmpty()) {
            colourindex = colourIndStr.toInt();
        }

        m_currentSegment->setColourIndex(colourindex);

        QString snapGridSizeStr = atts.value("snapgridsize");
        if (!snapGridSizeStr.isEmpty()) {
            m_currentSegment->setSnapGridSize(snapGridSizeStr.toInt());
        }

        QString viewFeaturesStr = atts.value("viewfeatures");
        if (!viewFeaturesStr.isEmpty()) {
            m_currentSegment->setViewFeatures(viewFeaturesStr.toInt());
        }

        m_currentTime = startTime;

        QString triggerIdStr = atts.value("triggerid");
        QString triggerPitchStr = atts.value("triggerbasepitch");
        QString triggerVelocityStr = atts.value("triggerbasevelocity");
        QString triggerRetuneStr = atts.value("triggerretune");
        QString triggerAdjustTimeStr = atts.value("triggeradjusttimes");
        
        QString linkerIdStr = atts.value("linkerid");

        QString linkerChgKeyStr = atts.value("linkertransposechangekey");
        QString linkerStepsStr = atts.value("linkertransposesteps");
        QString linkerSemitonesStr = atts.value("linkertransposesemitones");
        QString linkerTransBackStr =
                             atts.value("linkertransposesegmentback");

        if (!triggerIdStr.isEmpty()) {
            int pitch = -1;
            if (!triggerPitchStr.isEmpty())
                pitch = triggerPitchStr.toInt();
            int velocity = -1;
            if (!triggerVelocityStr.isEmpty())
                velocity = triggerVelocityStr.toInt();
            TriggerSegmentRec *rec =
                getComposition().addTriggerSegment(m_currentSegment,
                                                   triggerIdStr.toInt(),
                                                   pitch, velocity);
            if (rec) {
                if (!triggerRetuneStr.isEmpty())
                    rec->setDefaultRetune(triggerRetuneStr.toLower() == "true");
                if (!triggerAdjustTimeStr.isEmpty())
                    rec->setDefaultTimeAdjust(qstrtostr(triggerAdjustTimeStr));
            }
            m_currentSegment->setStartTimeDataMember(startTime);
        } else {
            if (!linkerIdStr.isEmpty()) {
                SegmentLinker *linker = 0;                                  
                int linkId = linkerIdStr.toInt();
                SegmentLinkerMap::iterator linkerItr = 
                                                  m_segmentLinkers.find(linkId);
                if (linkerItr == m_segmentLinkers.end()) {
                    //need to create a SegmentLinker with this id
                    linker = new SegmentLinker(linkId);
                    m_segmentLinkers[linkId] = linker;
                } else {
                    linker = linkerItr->second;
                }
                Segment::LinkTransposeParams params(
                                        linkerChgKeyStr.toLower()=="true",
                                        linkerStepsStr.toInt(),
                                        linkerSemitonesStr.toInt(),
                                        linkerTransBackStr.toLower()=="true");
                linker->addLinkedSegment(m_currentSegment);
                m_currentSegment->setLinkTransposeParams(params);
            }
            getComposition().addSegment(m_currentSegment);
            getComposition().setSegmentStartTime(m_currentSegment, startTime);
        }

        QString endMarkerStr = atts.value("endmarker");
        if (!endMarkerStr.isEmpty()) {
            delete m_segmentEndMarkerTime;
            m_segmentEndMarkerTime = new timeT(endMarkerStr.toInt());
        }

        m_groupIdMap.clear();

    } else if (lcName == "gui") {

        if (m_section != InSegment) {
            m_errorString = "Found GUI element outside Segment";
            return false;
        }

    } else if (lcName == "controller") {

        if (m_section != InSegment) {
            m_errorString = "Found Controller element outside Segment";
            return false;
        }

        QString type = atts.value("type");
        //RG_DEBUG << "RoseXmlHandler::startElement - controller type = " << type << endl;

        if (type == strtoqstr(PitchBend::EventType))
            m_currentSegment->addEventRuler(PitchBend::EventType);
        else if (type == strtoqstr(Controller::EventType)) {
            QString value = atts.value("value");

            if (value != "")
                m_currentSegment->addEventRuler(Controller::EventType, value.toInt());
        }

    } else if (lcName == "resync") {

        if (!m_deprecation)
            std::cerr << "WARNING: This Rosegarden file uses the deprecated element \"resync\".  We recommend re-saving the file from this version of Rosegarden to assure your ability to re-load it in future versions" << std::endl;
        m_deprecation = true;

        QString time(atts.value("time"));
        bool isNumeric;
        int numTime = time.toInt(&isNumeric);
        if (isNumeric)
            m_currentTime = numTime;

    } else if (lcName == "audio") {

        if (m_section != InAudioFiles) {
            m_errorString = "Audio object found outside Audio section";
            return false;
        }

        if (m_skipAllAudio) {
            std::cout << "SKIPPING audio file" << std::endl;
            return true;
        }

        QString id(atts.value("id"));
        QString file(atts.value("file"));
        QString label(atts.value("label"));

        if (id.isEmpty() || file.isEmpty() || label.isEmpty()) {
            m_errorString = "Audio object has empty parameters";
            return false;
        }

        m_hasActiveAudio = true;

        // attempt to insert file into AudioFileManager
        // (this checks the integrity of the file at the
        // same time)
        //
        if (getAudioFileManager().insertFile(qstrtostr(label),
                                             file,
                                             id.toInt()) == false) {

            // We failed to find the audio file.  First we'll try to
            // set the audio file path to "a sensible default", and if
            // this still doesn't work, we show a locate-file dialog.
            // In earlier times, RG used to use the last file path
            // that the KDE file dialog had associated with an audio
            // file for its sensible default, but we no longer have
            // that facility.  More recent code dropped the sensible
            // default and went straight to showing a directory
            // location dialog to ask the user to set the audio file
            // path -- but without any explanation of what the dialog
            // is for, this behaviour is just baffling.  And we're
            // about to show a file-locate dialog if this fails
            // anyway.  So instead let's just start by looking in the
            // same place as the .rg file.

            QString docPath = m_doc->getAbsFilePath();
            QString dirPath = QFileInfo(docPath).path();
            getAudioFileManager().setAudioPath(dirPath);

            RG_DEBUG << "Attempting to find audio file " << file
                     << " in path " << dirPath << endl;

            if (getAudioFileManager().insertFile(qstrtostr(label),
                                                 file, id.toInt()) == false) {

                // Freeze the progress dialog
                CurrentProgressDialog::freeze();

                // Hide splash screen if present on startup
                StartupLogo::hideIfStillThere();

                // Create a locate file dialog - give it the file name
                // and the AudioFileManager path that we've already
                // tried.  If we manually locate the file then we reset
                // the audiofilepath to the new value and see if this
                // helps us locate the rest of the files.
                //

                QString newFilename = "";
                QString newPath = "";

                do {

                    FileLocateDialog fL((QWidget *)m_doc->parent(),
                                        file,
                                        getAudioFileManager().getAudioPath());
                    int result = fL.exec();

                    if (result == QDialog::Accepted) {
                        newFilename = fL.getFilename();
                        newPath = fL.getDirectory();
                    } else if (result == QDialog::Rejected) {
                        // just skip the file
                        break;
                    } else {
                        // don't process any more audio files
                        m_skipAllAudio = true;
                        CurrentProgressDialog::thaw();
                        return true;
                    }


                } while (getAudioFileManager().insertFile(qstrtostr(label),
                         newFilename,
                         id.toInt()) == false);

                if (newPath != "") {
                    getAudioFileManager().setAudioPath(newPath);
                    // Set a document post-modify flag
                    //m_doc->setModified(true);
                }

                getAudioFileManager().print();

                // Restore progress dialog's normal state
                CurrentProgressDialog::thaw();
            } else {
                // AudioPath is modified so set a document post modify flag
                //
                //m_doc->setModified(true);
            }

        }

    } else if (lcName == "audiopath") {

        if (m_section != InAudioFiles) {
            m_errorString = "Audiopath object found outside AudioFiles section";
            return false;
        }

        QString search(atts.value("value"));

        if (search.isEmpty()) {
            m_errorString = "Audiopath has no value";
            return false;
        }

        if (!search.startsWith("/") && !search.startsWith("~")) {
            QString docPath = m_doc->getAbsFilePath();
            QString dirPath = QFileInfo(docPath).path();
            if (QFileInfo(dirPath).exists()) {
                search = dirPath + "/" + search;
            }
        }

        getAudioFileManager().setAudioPath(search);

    } else if (lcName == "begin") {

        double marker = qstrtodouble(atts.value("index"));

        if (!m_currentSegment) {
            // Don't fail - as this segment could be defunct if we
            // skipped loading the audio file
            //
            return true;
        }

        if (m_currentSegment->getType() != Segment::Audio) {
            m_errorString = "Found audio begin index in non audio segment";
            return false;
        }

        // convert to RealTime from float
        int sec = (int)marker;
        int usec = (int)((marker - ((double)sec)) * 1000000.0);
        m_currentSegment->setAudioStartTime(RealTime(sec, usec * 1000));


    } else if (lcName == "end") {

        double marker = qstrtodouble(atts.value("index"));

        if (!m_currentSegment) {
            // Don't fail - as this segment could be defunct if we
            // skipped loading the audio file
            //
            return true;
        }

        if (m_currentSegment->getType() != Segment::Audio) {
            m_errorString = "found audio end index in non audio segment";
            return false;
        }

        int sec = (int)marker;
        int usec = (int)((marker - ((double)sec)) * 1000000.0);
        RealTime markerTime(sec, usec * 1000);

        if (markerTime < m_currentSegment->getAudioStartTime()) {
            m_errorString = "Audio end index before audio start marker";
            return false;
        }

        m_currentSegment->setAudioEndTime(markerTime);

        // Ensure we set end time according to correct RealTime end of Segment
        //
        RealTime realEndTime = getComposition().
                               getElapsedRealTime(m_currentSegment->getStartTime()) +
                               m_currentSegment->getAudioEndTime() -
                               m_currentSegment->getAudioStartTime();

        timeT absEnd = getComposition().getElapsedTimeForRealTime(realEndTime);
        m_currentSegment->setEndTime(absEnd);

    } else if (lcName == "fadein") {

        if (!m_currentSegment) {
            // Don't fail - as this segment could be defunct if we
            // skipped loading the audio file
            //
            return true;
        }

        if (m_currentSegment->getType() != Segment::Audio) {
            m_errorString = "found fade in time in non audio segment";
            return false;
        }

        double marker = qstrtodouble(atts.value("time"));
        int sec = (int)marker;
        int usec = (int)((marker - ((double)sec)) * 1000000.0);
        RealTime markerTime(sec, usec * 1000);

        m_currentSegment->setFadeInTime(markerTime);
        m_currentSegment->setAutoFade(true);


    } else if (lcName == "fadeout") {

        if (!m_currentSegment) {
            // Don't fail - as this segment could be defunct if we
            // skipped loading the audio file
            //
            return true;
        }

        if (m_currentSegment->getType() != Segment::Audio) {
            m_errorString = "found fade out time in non audio segment";
            return false;
        }

        double marker = qstrtodouble(atts.value("time"));
        int sec = (int)marker;
        int usec = (int)((marker - ((double)sec)) * 1000000.0);
        RealTime markerTime(sec, usec * 1000);

        m_currentSegment->setFadeOutTime(markerTime);
        m_currentSegment->setAutoFade(true);

    } else if (lcName == "device") {

        if (m_section != InStudio) {
            m_errorString = "Found Device outside Studio";
            return false;
        }

        m_haveControls = false;

        QString type = (atts.value("type")).toLower();
        QString idString = atts.value("id");
        QString nameStr = atts.value("name");

        if (idString.isNull()) {
            m_errorString = "No ID on Device tag";
            return false;
        }
        
        //int id = idString.toInt();

        if (type == "midi") {
            QString direction = atts.value("direction").toLower();

            if (direction.isNull() ||
                direction == "" ||
                direction == "play") { // ignore inputs

                if (!nameStr.isEmpty()) {
                    addMIDIDevice(nameStr, m_createDevices, "play"); // also sets m_device
                }
            }
            
            
            if (direction == "record") {
                if (m_device) {
                    if (!nameStr.isEmpty()) {
                        m_device->setName(qstrtostr(nameStr));
                    }
                } else if (!nameStr.isEmpty()) {
                    addMIDIDevice(nameStr, m_createDevices, "record"); // also sets m_device
                }
            }

            QString connection = atts.value("connection");
            if ((m_createDevices) && (m_device) &&
                !connection.isNull() && (!connection.isEmpty()) ) {
                setMIDIDeviceConnection(connection);
            }

            if (m_createDevices)
                setMIDIDeviceName(nameStr);

            QString vstr = atts.value("variation").toLower();
            MidiDevice::VariationType variation =
                MidiDevice::NoVariations;
            if (!vstr.isNull()) {
                if (vstr == "lsb") {
                    variation = MidiDevice::VariationFromLSB;
                } else if (vstr == "msb") {
                    variation = MidiDevice::VariationFromMSB;
                } else if (vstr == "") {
                    variation = MidiDevice::NoVariations;
                }
            }
            MidiDevice *md = dynamic_cast<MidiDevice *>(m_device);
            if (md) {
                md->setVariationType(variation);
            }
        } else if (type == "softsynth") {
            m_device = getStudio().getSoftSynthDevice();
            if (m_device && m_device->getType() == Device::SoftSynth) {
                m_device->setName(qstrtostr(nameStr));
                m_deviceRunningId = m_device->getId();
                m_deviceInstrumentBase = SoftSynthInstrumentBase;
                m_deviceReadInstrumentBase = 0;
            }
        } else if (type == "audio") {
            m_device = getStudio().getAudioDevice();
            if (m_device && m_device->getType() == Device::Audio) {
                m_device->setName(qstrtostr(nameStr));
                m_deviceRunningId = m_device->getId();
                m_deviceInstrumentBase = AudioInstrumentBase;
                m_deviceReadInstrumentBase = 0;
            }
        } else {
            m_errorString = "Found unknown Device type";
            return false;
        }

    } else if (lcName == "librarian") {

        // The contact details for the maintainer of the banks/programs
        // information.
        //
        if (m_device && m_device->getType() == Device::Midi) {
            QString name = atts.value("name");
            QString email = atts.value("email");

            dynamic_cast<MidiDevice*>(m_device)->
            setLibrarian(qstrtostr(name), qstrtostr(email));
        }

    } else if (lcName == "bank") {

        if (m_device) // only if we have a device
        {
            if (m_section != InStudio && m_section != InInstrument)
            {
                m_errorString = "Found Bank outside Studio or Instrument";
                return false;
            }

            QString nameStr = atts.value("name");
            m_percussion = (atts.value("percussion").toLower() == "true");
            m_msb = (atts.value("msb")).toInt();
            m_lsb = (atts.value("lsb")).toInt();
            
            // Must account for file format <1.6.0
            // which would return an empty string.
            //
            // File formats <1.6.0 assume the existence of bank tag implies
            // send bank select should be set to true.
            m_sendBankSelect = !(atts.value("send").toLower() == "false");

            // To actually create a bank
            //
            if (m_section == InStudio)
            {
                // Create a new bank
                MidiBank bank(m_percussion,
                              m_msb,
                              m_lsb,
                              qstrtostr(nameStr));

                if (m_device->getType() == Device::Midi) {
                    // Insert the bank
                    //
                    dynamic_cast<MidiDevice*>(m_device)->addBank(bank);
                }
            } else // otherwise we're referencing it in an instrument
                if (m_section == InInstrument)
                {
                    if (m_instrument) {
                        m_instrument->setPercussion(m_percussion);
                        m_instrument->setMSB(m_msb);
                        m_instrument->setLSB(m_lsb);
                        m_instrument->setSendBankSelect(m_sendBankSelect);
                    }
                }
        }

    } else if (lcName == "program") {

        if (m_device) // only if we have a device
        {
            if (m_section == InStudio)
            {
                QString nameStr = (atts.value("name"));
                MidiByte pc = atts.value("id").toInt();
                QString keyMappingStr = (atts.value("keymapping"));

                // Create a new program
                bool k = !keyMappingStr.isEmpty();

                MidiProgram program
                (MidiBank(m_percussion,
                          m_msb,
                          m_lsb),
                 pc,
                 qstrtostr(nameStr),
                 k ? qstrtostr(keyMappingStr) : "");

                if (m_device->getType() == Device::Midi) {
                    // Insert the program
                    //
                    dynamic_cast<MidiDevice*>(m_device)->
                    addProgram(program);
                }

            } else if (m_section == InInstrument)
            {
                if (m_instrument) {
                    // Must account for file format <1.6.0
                    // which would return an empty string.
                    //
                    // File formats <1.6.0 assume the existence of bank tag implies
                    // send program change should be set to true.
                    bool sendProgramChange = !(atts.value("send").toLower() == "false");

                    MidiByte id = atts.value("id").toInt();
                    m_instrument->setProgramChange(id);
                    m_instrument->setSendProgramChange(sendProgramChange);
                }
            } else
            {
                m_errorString = "Found Program outside Studio and Instrument";
                return false;
            }
        }

    } else if (lcName == "keymapping") {

        if (m_section == InInstrument) {
            RG_DEBUG << "Old-style keymapping in instrument found, ignoring" << endl;
        } else {

            if (m_section != InStudio) {
                m_errorString = "Found Keymapping outside Studio";
                return false;
            }

            if (m_device && (m_device->getType() == Device::Midi)) {
                QString name = atts.value("name");
                m_keyMapping = new MidiKeyMapping(qstrtostr(name));
                m_keyNameMap.clear();
            }
        }

    } else if (lcName == "key") {

        if (m_keyMapping) {
            QString numStr = atts.value("number");
            QString namStr = atts.value("name");
            if (!numStr.isEmpty() && !namStr.isEmpty()) {
                m_keyNameMap[numStr.toInt()] = qstrtostr(namStr);
            }
        }

    } else if (lcName == "controls") {

        // Only clear down the controllers list if we have found some controllers in the RG file
        //
        if (m_device) {
            dynamic_cast<MidiDevice*>(m_device)->clearControlList();
        }

        m_haveControls = true;

    } else if (lcName == "control") {

        if (m_section != InStudio) {
            m_errorString = "Found ControlParameter outside Studio";
            return false;
        }

        if (!m_device) {
            //!!! ach no, we can't give this warning -- we might be in a <device> elt
            // but have no sequencer support, for example.  we need a separate m_inDevice
            // flag
            //        m_deprecation = true;
            //        std::cerr << "WARNING: This Rosegarden file uses a deprecated control parameter structure.  We recommend re-saving the file from this version of Rosegarden to assure your ability to re-load it in future versions" << std::endl;

        } else if (m_device->getType() == Device::Midi) {

            if (!m_haveControls) {
                m_errorString = "Found ControlParameter outside Controls block";
                return false;
            }

            QString name = atts.value("name");
            QString type = atts.value("type");
            QString descr = atts.value("description");
            QString min = atts.value("min");
            QString max = atts.value("max");
            QString def = atts.value("default");
            QString conVal = atts.value("controllervalue");
            QString colour = atts.value("colourindex");
            QString ipbPosition = atts.value("ipbposition");

            ControlParameter con(qstrtostr(name),
                                 qstrtostr(type),
                                 qstrtostr(descr),
                                 min.toInt(),
                                 max.toInt(),
                                 def.toInt(),
                                 MidiByte(conVal.toInt()),
                                 colour.toInt(),
                                 ipbPosition.toInt());

            // !!! Not clear whether this should propagate to
            // instruments.  Conservatively keeping the original
            // semantics, which may not be right.
            dynamic_cast<MidiDevice*>(m_device)->
                addControlParameter(con, true);
        }

    } else if (lcName == "reverb") { // deprecated but we still read 'em

        if (!m_deprecation)
            std::cerr << "WARNING: This Rosegarden file uses the deprecated element \"reverb\" (now replaced by a control parameter).  We recommend re-saving the file from this version of Rosegarden to assure your ability to re-load it in future versions" << std::endl;
        m_deprecation = true;

        if (m_section != InInstrument) {
            m_errorString = "Found Reverb outside Instrument";
            return false;
        }

        MidiByte value = atts.value("value").toInt();

        if (m_instrument)
            m_instrument->setControllerValue(MIDI_CONTROLLER_REVERB, value);


    } else if (lcName == "chorus") { // deprecated but we still read 'em

        if (!m_deprecation)
            std::cerr << "WARNING: This Rosegarden file uses the deprecated element \"chorus\" (now replaced by a control parameter).  We recommend re-saving the file from this version of Rosegarden to assure your ability to re-load it in future versions" << std::endl;
        m_deprecation = true;

        if (m_section != InInstrument) {
            m_errorString = "Found Chorus outside Instrument";
            return false;
        }

        MidiByte value = atts.value("value").toInt();

        if (m_instrument)
            m_instrument->setControllerValue(MIDI_CONTROLLER_CHORUS, value);

    } else if (lcName == "filter") { // deprecated but we still read 'em

        if (!m_deprecation)
            std::cerr << "WARNING: This Rosegarden file uses the deprecated element \"filter\" (now replaced by a control parameter).  We recommend re-saving the file from this version of Rosegarden to assure your ability to re-load it in future versions" << std::endl;
        m_deprecation = true;

        if (m_section != InInstrument) {
            m_errorString = "Found Filter outside Instrument";
            return false;
        }

        MidiByte value = atts.value("value").toInt();

        if (m_instrument)
            m_instrument->setControllerValue(MIDI_CONTROLLER_FILTER, value);


    } else if (lcName == "resonance") { // deprecated but we still read 'em

        if (!m_deprecation)
            std::cerr << "WARNING: This Rosegarden file uses the deprecated element \"resonance\" (now replaced by a control parameter).  We recommend re-saving the file from this version of Rosegarden to assure your ability to re-load it in future versions" << std::endl;
        m_deprecation = true;

        if (m_section != InInstrument) {
            m_errorString = "Found Resonance outside Instrument";
            return false;
        }

        MidiByte value = atts.value("value").toInt();

        if (m_instrument)
            m_instrument->setControllerValue(MIDI_CONTROLLER_RESONANCE, value);


    } else if (lcName == "attack") { // deprecated but we still read 'em

        if (!m_deprecation)
            std::cerr << "WARNING: This Rosegarden file uses the deprecated element \"attack\" (now replaced by a control parameter).  We recommend re-saving the file from this version of Rosegarden to assure your ability to re-load it in future versions" << std::endl;
        m_deprecation = true;

        if (m_section != InInstrument) {
            m_errorString = "Found Attack outside Instrument";
            return false;
        }

        MidiByte value = atts.value("value").toInt();

        if (m_instrument)
            m_instrument->setControllerValue(MIDI_CONTROLLER_ATTACK, value);

    } else if (lcName == "release") { // deprecated but we still read 'em

        if (!m_deprecation)
            std::cerr << "WARNING: This Rosegarden file uses the deprecated element \"release\" (now replaced by a control parameter).  We recommend re-saving the file from this version of Rosegarden to assure your ability to re-load it in future versions" << std::endl;
        m_deprecation = true;

        if (m_section != InInstrument) {
            m_errorString = "Found Release outside Instrument";
            return false;
        }

        MidiByte value = atts.value("value").toInt();

        if (m_instrument)
            m_instrument->setControllerValue(MIDI_CONTROLLER_RELEASE, value);

    } else if (lcName == "pan") {

        if (m_section != InInstrument && m_section != InBuss) {
            m_errorString = "Found Pan outside Instrument or Buss";
            return false;
        }

        MidiByte value = atts.value("value").toInt();

        if (m_section == InInstrument) {
            if (m_instrument) {
                m_instrument->setControllerValue(MIDI_CONTROLLER_PAN, value);
                m_instrument->setSendPan(true);
            }
        } else if (m_section == InBuss) {
            if (m_buss) {
                m_buss->setPan(value);
            }
        }

        // keep "velocity" so we're backwards compatible
    } else if (lcName == "velocity" || lcName == "volume") {

        if (lcName == "velocity") {
            if (!m_deprecation)
                std::cerr << "WARNING: This Rosegarden file uses the deprecated element \"velocity\" for an overall MIDI instrument level (now replaced by \"volume\").  We recommend re-saving the file from this version of Rosegarden to assure your ability to re-load it in future versions" << std::endl;
            m_deprecation = true;
        }

        if (m_section != InInstrument) {
            m_errorString = "Found Volume outside Instrument";
            return false;
        }

        MidiByte value = atts.value("value").toInt();

        if (m_instrument) {
            if (m_instrument->getType() == Instrument::Audio ||
                m_instrument->getType() == Instrument::SoftSynth) {
                // Backward compatibility: "volume" was in a 0-127
                // range and we now store "level" (float dB) instead.
                // Note that we have no such compatibility for
                // "recordLevel", whose range has changed silently.
                if (!m_deprecation)
                    std::cerr << "WARNING: This Rosegarden file uses the deprecated element \"volume\" for an audio instrument (now replaced by \"level\").  We recommend re-saving the file from this version of Rosegarden to assure your ability to re-load it in future versions" << std::endl;
                m_deprecation = true;
                m_instrument->setLevel
                    (AudioLevel::multiplier_to_dB(float(value) / 100.0));
            } else {
                m_instrument->setControllerValue(MIDI_CONTROLLER_VOLUME, value);
                m_instrument->setSendVolume(true);
            }
        }

    } else if (lcName == "level") {

        if (m_section != InBuss &&
                (m_section != InInstrument ||
                 (m_instrument &&
                  m_instrument->getType() != Instrument::Audio &&
                  m_instrument->getType() != Instrument::SoftSynth))) {
            m_errorString = "Found Level outside (audio) Instrument or Buss";
            return false;
        }

        double value = qstrtodouble(atts.value("value"));

        if (m_section == InBuss) {
            if (m_buss)
                m_buss->setLevel(value);
        } else {
            if (m_instrument)
                m_instrument->setLevel(value);
        }

    } else if (lcName == "controlchange") {

        if (m_section != InInstrument) {
            m_errorString = "Found ControlChange outside Instrument";
            return false;
        }

        MidiByte type = atts.value("type").toInt();
        MidiByte value = atts.value("value").toInt();

        if (m_instrument) {
            m_instrument->setControllerValue(type, value);
        }

    } else if (lcName == "plugin" || lcName == "synth") {

        PluginContainer *container = 0;

        if (m_section == InInstrument) {
//            std::cerr << "Found plugin in instrument" << std::endl;
            container = m_instrument;
            m_pluginInBuss = false;
        } else if (m_section == InBuss) {
//            std::cerr << "Found plugin in buss" << std::endl;
            container = m_buss;
            m_pluginInBuss = true;
        } else {
            m_errorString = "Found Plugin outside Instrument or Buss";
            return false;
        }

        // Despite being InInstrument or InBuss we might not actually
        // have a valid one.
        //
        if (container) {

//            std::cerr << "Have container" << std::endl;

            emit setOperationName(tr("Loading plugins..."));
            qApp->processEvents(QEventLoop::AllEvents, 100);

            // Get the details
            int position;
            if (lcName == "synth") {
                position = Instrument::SYNTH_PLUGIN_POSITION;
            } else {
                position = atts.value("position").toInt();
            }

            bool bypassed = false;
            QString bpStr = atts.value("bypassed");
            if (bpStr.toLower() == "true")
                bypassed = true;

            std::string program = "";
            QString progStr = atts.value("program");
            if (!progStr.isEmpty()) {
                program = qstrtostr(progStr);
            }

            // Plugins are identified by a structured identifier
            // string, but we will accept a LADSPA UniqueId if there's
            // no identifier, for backward compatibility

            QString identifier = atts.value("identifier");

            AudioPlugin *plugin = 0;
            AudioPluginManager *apm = getAudioPluginManager();

            if ( identifier.isEmpty() ) {
                QString q = atts.value("id");
                if (!q.isEmpty()) {
                    unsigned long id = atts.value("id").toULong();
                    if (apm)
                        plugin = apm->getPluginByUniqueId(id);
                }
            } else {
                if (apm)
                    plugin = apm->getPluginByIdentifier(identifier);
            }

            std::cerr << "Plugin identifier " << identifier << " -> plugin " << plugin << std::endl;

            // If we find the plugin all is well and good but if
            // we don't we just skip it.
            //
            if (plugin) {
                m_plugin = container->getPlugin(position);
                if (!m_plugin) {
                    RG_DEBUG << "WARNING: RoseXmlHandler: instrument/buss "
                    << container->getId() << " has no plugin position "
                    << position << endl;
                } else {
                    m_plugin->setAssigned(true);
                    m_plugin->setBypass(bypassed);
                    m_plugin->setIdentifier( qstrtostr( plugin->getIdentifier() ) );
                    std::cerr << "set identifier to plugin at position " << position << " of container " << container->getId() << std::endl;
                    if (program != "") {
                        m_plugin->setProgram(program);
                    }
                }
            } else {
                // we shouldn't be halting import of the RG file just because
                // we can't match a plugin
                //
                QString q = atts.value("id");

                if (!identifier.isEmpty()) {
                    RG_DEBUG << "WARNING: RoseXmlHandler: plugin " << identifier << " not found" << endl;
                    m_pluginsNotFound.insert(identifier);
                } else if (!q.isEmpty()) {
                    RG_DEBUG << "WARNING: RoseXmlHandler: plugin uid " << atts.value("id") << " not found" << endl;
                } else {
                    m_errorString = "No plugin identifier or uid specified";
                    return false;
                }
            }
        } else { // no instrument

            if (lcName == "synth") {
                QString identifier = atts.value("identifier");
                if (!identifier.isEmpty()) {
                    RG_DEBUG << "WARNING: RoseXmlHandler: no instrument for plugin " << identifier << endl;
                    m_pluginsNotFound.insert(identifier);
                }
            }
        }

        m_section = InPlugin;

    } else if (lcName == "port") {

        if (m_section != InPlugin) {
            m_errorString = "Found Port outside Plugin";
            return false;
        }
        unsigned long portId = atts.value("id").toULong();
        double value = qstrtodouble(atts.value("value"));

        QString changed = atts.value("changed");
        bool changedSinceProgram = (changed == "true");

        if (m_plugin) {
            m_plugin->addPort(portId, value);
            if (changedSinceProgram) {
                PluginPortInstance *ppi = m_plugin->getPort(portId);
                if (ppi)
                    ppi->changedSinceProgramChange = true;
            }
        }

    } else if (lcName == "configure") {

        if (m_section != InPlugin) {
            m_errorString = "Found Configure outside Plugin";
            return false;
        }

        QString key = atts.value("key");
        QString value = atts.value("value");

        if (m_plugin) {
            m_plugin->setConfigurationValue(qstrtostr(key), qstrtostr(value));
        }

    } else if (lcName == "metronome") {

        if (m_section != InStudio) {
            m_errorString = "Found Metronome outside Studio";
            return false;
        }

        // Only create if we have a device
        //
        if (m_device &&
            (m_device->getType() == Device::Midi ||
             m_device->getType() == Device::SoftSynth)) {

            // We will map this to an "actual" instrument ID at the
            // end, when we do the same for the track->instrument
            // references
            InstrumentId instrument = atts.value("instrument").toInt();

            MidiMetronome metronome(instrument);

            QString q = atts.value("barpitch");
            if (!q.isEmpty())
                metronome.setBarPitch(atts.value("barpitch").toInt());

            q = atts.value("beatpitch");    
            if (!q.isEmpty())
                metronome.setBeatPitch(atts.value("beatpitch").toInt());
            
            q = atts.value("subbeatpitch");
            if (!q.isEmpty())
                metronome.setSubBeatPitch(atts.value("subbeatpitch").toInt());

            q = atts.value("depth");
            if (!q.isEmpty())
                metronome.setDepth(atts.value("depth").toInt());

            q = atts.value("barvelocity");
            if (!q.isEmpty())
                metronome.setBarVelocity(atts.value("barvelocity").toInt());

            q = atts.value("beatvelocity");
            if (!q.isEmpty())
                metronome.setBeatVelocity(atts.value("beatvelocity").toInt());

            q = atts.value("subbeatvelocity");
            if (!q.isEmpty())
                metronome.setSubBeatVelocity(atts.value("subbeatvelocity").toInt());

            MidiDevice *md = dynamic_cast<MidiDevice *>(m_device);
            if (md) md->setMetronome(metronome);

            SoftSynthDevice *ssd = dynamic_cast<SoftSynthDevice *>(m_device);
            if (ssd) ssd->setMetronome(metronome);
        }

    } else if (lcName == "instrument") {

        if (m_section != InStudio) {
            m_errorString = "Found Instrument outside Studio";
            return false;
        }

        m_section = InInstrument;

        InstrumentId id = mapToActualInstrument(atts.value("id").toInt());

        std::string stringType = qstrtostr(atts.value("type"));
        Instrument::InstrumentType type;

        if (stringType == "midi")
            type = Instrument::Midi;
        else if (stringType == "audio")
            type = Instrument::Audio;
        else if (stringType == "softsynth")
            type = Instrument::SoftSynth;
        else {
            m_errorString = "Found unknown Instrument type";
            return false;
        }

        // Try and match an Instrument in the file with one in
        // our studio
        //
        Instrument *instrument = getStudio().getInstrumentById(id);

        RG_DEBUG << "Found Instrument in document: mapped actual id " << id << " to instrument " << instrument << endl;

        // If we've got an instrument and the types match then
        // we use it from now on.
        //
        if (instrument && instrument->getType() == type) {
            m_instrument = instrument;

            // Synth and Audio instruments always have the channel set to 2.
            // Preserve this.
            MidiByte channel = (MidiByte)atts.value("channel").toInt();

            m_instrument->setNaturalChannel(channel);

            if (type == Instrument::Midi)
                {
                    QString valueText = atts.value("fixed");
                    // Despite appearances this is not comparing
                    // addresses, it tests for empty QString.
                    if ("" == valueText) {
                        // Old file with no "fixed" attribute.  There is no
                        // information one way or the other, so default all
                        // channels to fixed.
                        //
                        // NB. We used to default to fixed only for percussion
                        // instruments, but several long-time users experienced
                        // annoying problems with this behavior when loading
                        // old files.
                        m_instrument->setFixedChannel();

                    } else if (valueText.toLower() == "true") {
                        // Fixed attribute is set.
                        m_instrument->setFixedChannel();
                    } 
            }
        }

    } else if (lcName == "buss") {

        if (m_section != InStudio) {
            m_errorString = "Found Buss outside Studio";
            return false;
        }

        m_section = InBuss;

        BussId id = atts.value("id").toInt();
        Buss *buss = getStudio().getBussById(id);

        // If we've got a buss then we use it from now on.
        //
        if (buss) {
            m_buss = buss;
        } else {
            m_buss = new Buss(id);
            getStudio().addBuss(m_buss);
        }

    } else if (lcName == "audiofiles") {

        if (m_section != NoSection) {
            m_errorString = "Found AudioFiles inside another section";
            return false;
        }

        m_section = InAudioFiles;

        int rate = atts.value("expectedRate").toInt();
        if (rate) {
            getAudioFileManager().setExpectedSampleRate(rate);
        }

    } else if (lcName == "configuration") {

        setSubHandler(new ConfigurationXmlSubHandler
                      (lcName, &m_doc->getConfiguration()));

    } else if (lcName == "metadata") {

        if (m_section != InComposition) {
            m_errorString = "Found Metadata outside Composition";
            return false;
        }

        setSubHandler(new ConfigurationXmlSubHandler
                      (lcName, &getComposition().getMetadata()));

    } else if (lcName == "recordlevel") {

        if (m_section != InInstrument) {
            m_errorString = "Found recordLevel outside Instrument";
            return false;
        }

        double value = qstrtodouble(atts.value("value"));

        // if the value retrieved is greater than (say) 15 then we
        // must have an old-style 0-127 value instead of a shiny new
        // dB value, so convert it
        if (value > 15.0) {
            value = AudioLevel::multiplier_to_dB(value / 100);
        }

        if (m_instrument)
            m_instrument->setRecordLevel(value);

    } else if (lcName == "alias") {

        if (m_section != InInstrument) {
            m_errorString = "Found alias outside Instrument";
            return false;
        }
        if (m_instrument) {
            QString alias = atts.value("value");
            m_instrument->setAlias(alias.toStdString());
        }
            
    } else if (lcName == "audioinput") {

        if (m_section != InInstrument) {
            m_errorString = "Found audioInput outside Instrument";
            return false;
        }

        int value = atts.value("value").toInt();
        int channel = atts.value("channel").toInt();

        QString type = atts.value("type");
        if (!type.isEmpty()) {
            if (type.toLower() == "buss") {
                if (m_instrument)
                    m_instrument->setAudioInputToBuss(value, channel);
            } else if (type.toLower() == "record") {
                if (m_instrument)
                    m_instrument->setAudioInputToRecord(value, channel);
            }
        }

    } else if (lcName == "audiooutput") {

        if (m_section != InInstrument) {
            m_errorString = "Found audioOutput outside Instrument";
            return false;
        }

        int value = atts.value("value").toInt();
        if (m_instrument)
            m_instrument->setAudioOutput(value);

    } else if (lcName == "appearance") {

        m_section = InAppearance;

    } else if (lcName == "colourmap") {

        if (m_section == InAppearance) {
            QString mapName = atts.value("name");
            m_inColourMap = true;
            if (mapName == "segmentmap") {
                m_colourMap = &m_doc->getComposition().getSegmentColourMap();
            } else
                if (mapName == "generalmap") {
                    m_colourMap = &m_doc->getComposition().getGeneralColourMap();
                } else { // This will change later once we get more of the Appearance code sorted out
                    RG_DEBUG << "RoseXmlHandler::startElement : Found colourmap with unknown name\n";
                }
        } else {
            m_errorString = "Found colourmap outside Appearance";
            return false;
        }

    } else if (lcName == "colourpair") {

        if (m_inColourMap && m_colourMap) {
            unsigned int id = atts.value("id").toInt();
            QString name = atts.value("name");
            unsigned int red = atts.value("red").toInt();
            unsigned int blue = atts.value("blue").toInt();
            unsigned int green = atts.value("green").toInt();
            Colour colour(red, green, blue);
            m_colourMap->addItem(colour, qstrtostr(name), id);
        } else {
            m_errorString = "Found colourpair outside ColourMap";
            return false;
        }

    } else if (lcName == "markers") {

        if (!m_inComposition) {
            m_errorString = "Found Markers outside Composition";
            return false;
        }

        // clear down any markers
        getComposition().clearMarkers();

    } else if (lcName == "marker") {
        if (!m_inComposition) {
            m_errorString = "Found Marker outside Composition";
            return false;
        }
        int time = atts.value("time").toInt();
        QString name = atts.value("name");
        QString descr = atts.value("description");

        Marker *marker =
            new Marker(time,
                       qstrtostr(name),
                       qstrtostr(descr));

        getComposition().addMarker(marker);
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
        bool res = getSubHandler()->endElement(namespaceURI, localName, qName.toLower(), finished);
        if (finished)
            setSubHandler(0);
        return res;
    }

    // Set percentage done
    //
    if ((m_totalElements > m_elementsSoFar) &&
        (++m_elementsSoFar % 300 == 0)) {

        Profiler profiler("RoseXmlHandler::endElement: emit progress");

//        std::cout << "emitting setValue(" << int(double(m_elementsSoFar) / double(m_totalElements) * 100.0) << ")" << std::endl;

        emit setValue(int(double(m_elementsSoFar) / double(m_totalElements) * 100.0));
        qApp->processEvents(QEventLoop::AllEvents, 100);
    }

    QString lcName = qName.toLower();

    if (lcName == "rosegarden-data") {

        Composition &comp = getComposition();

        // Remap all the instrument IDs in track and metronome objects
        // from "file" to "actual" IDs.  See discussion in
        // mapToActualInstrument() below.

        for (Composition::trackcontainer::iterator i = comp.getTracks().begin();
             i != comp.getTracks().end(); ++i) {
            InstrumentId iid = i->second->getInstrument();
            InstrumentId aid = mapToActualInstrument(iid);
            RG_DEBUG << "RoseXmlHandler: mapping instrument " << iid
                     << " to " << aid << " for track " << i->first << endl;
            i->second->setInstrument(aid);
        }

        Studio &studio = getStudio();
        for (DeviceList::iterator i = studio.getDevices()->begin();
             i != studio.getDevices()->end(); ++i) {
            MidiMetronome mm(0);
            MidiDevice *md = dynamic_cast<MidiDevice *>(*i);
            SoftSynthDevice *sd = dynamic_cast<SoftSynthDevice *>(*i);
            if (md && md->getMetronome()) mm = *md->getMetronome();
            else if (sd && sd->getMetronome()) mm = *sd->getMetronome();
            else continue;
            InstrumentId iid = mm.getInstrument();
            InstrumentId aid = mapToActualInstrument(iid);
            RG_DEBUG << "RoseXmlHandler: mapping instrument " << iid
                     << " to " << aid << " for metronome" << endl;
            if (md) md->setMetronome(mm);
            else if (sd) sd->setMetronome(mm);
        }

        comp.updateTriggerSegmentReferences();

    } else if (lcName == "event") {

        if (m_currentSegment && m_currentEvent) {
            m_currentSegment->insert(m_currentEvent);
            m_currentEvent = 0;
        } else if (!m_currentSegment && m_currentEvent) {
            m_errorString = "Got event outside of a Segment";
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

            // If the segment is zero or negative duration
            if (m_currentSegment->getEndMarkerTime() <= 
                    m_currentSegment->getStartTime()) {
                // Make it stick out so the user can take care of it.
                m_currentSegment->setEndMarkerTime(
                    m_currentSegment->getStartTime() + 
                        Note(Note::Shortest).getDuration());
            }

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

    } else if (lcName == "buss") {

        m_section = InStudio;
        m_buss = 0;

    } else if (lcName == "instrument") {

        m_section = InStudio;
        m_instrument = 0;

    } else if (lcName == "plugin") {

        if (m_pluginInBuss) {
            m_section = InBuss;
        } else {
            m_section = InInstrument;
        }
        m_plugin = 0;
        m_pluginId = 0;

    } else if (lcName == "device") {

        m_device = 0;

    } else if (lcName == "keymapping") {

        if (m_section == InStudio) {
            if (m_keyMapping) {
                if (!m_keyNameMap.empty()) {
                    MidiDevice *md = dynamic_cast<MidiDevice *>
                                     (m_device);
                    if (md) {
                        m_keyMapping->setMap(m_keyNameMap);
                        md->addKeyMapping(*m_keyMapping);
                    }
                }
                m_keyMapping = 0;
            }
        }

    } else if (lcName == "audiofiles") {

        m_section = NoSection;

    } else if (lcName == "appearance") {

        m_section = NoSection;

    } else if (lcName == "colourmap") {
        m_inColourMap = false;
        m_colourMap = 0;
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
RoseXmlHandler::errorString() const
{
    return m_errorString;
}

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
    if (m_foundTempo == false) {
        getComposition().setCompositionDefaultTempo
        (Composition::getTempoForQpm(120.0));
    }

    return true;
}

void
RoseXmlHandler::setSubHandler(XmlSubHandler* sh)
{
    delete m_subHandler;
    m_subHandler = sh;
}

void
RoseXmlHandler::addMIDIDevice(QString name, bool createAtSequencer, QString dir)
{
    /**
    *   params:
    *   QString name           : device name
    *   bool createAtSequencer : normally true
    *   QString dir            : direction "play" or "record"
    **/
    
    unsigned int deviceId = 0;
    
    MidiDevice::DeviceDirection devDir;
    
    if (dir == "play") {
        devDir = MidiDevice::Play;
    } else if (dir == "record") {
        devDir = MidiDevice::Record;
    } else {
        std::cerr << "Error: Device direction \"" << dir
                  << "\" invalid in RoseXmlHandler::addMIDIDevice()" << std::endl;
        return;
    }

    InstrumentId instrumentBase;
    deviceId = getStudio().getSpareDeviceId(instrumentBase);

    if (createAtSequencer) {
        if (!RosegardenSequencer::getInstance()->
            addDevice(Device::Midi, deviceId, instrumentBase, devDir)) {
            SEQMAN_DEBUG << "RoseXmlHandler::addMIDIDevice - "
                         << "sequencer addDevice failed" << endl;
            return;
        }

        SEQMAN_DEBUG << "RoseXmlHandler::addMIDIDevice - "
                     << " added device " << deviceId
                     << " with instrument base " << instrumentBase
                     << " at sequencer" << endl;
    }

    getStudio().addDevice(qstrtostr(name), deviceId,
                          instrumentBase, Device::Midi);
    m_device = getStudio().getDevice(deviceId);
    if (m_device) {
        MidiDevice *md = dynamic_cast<MidiDevice *>(m_device);
        if (md) md->setDirection(devDir);
    }

    SEQMAN_DEBUG << "RoseXmlHandler::addMIDIDevice - "
                 << " added device " << deviceId
                 << " with instrument base " << instrumentBase
                 << " in studio" << endl;

    m_deviceRunningId = deviceId;
    m_deviceInstrumentBase = instrumentBase;
    m_deviceReadInstrumentBase = 0;
}

InstrumentId
RoseXmlHandler::mapToActualInstrument(InstrumentId oldId)
{
    /*
      When we read a device from the file, we want to be able to add
      it to the studio and the sequencer immediately.  But to do so,
      we (now) need to be able to provide a base instrument number for
      the device.  We can make up a plausible one (we know what type
      of device it is, and that's the main determining factor) but we
      can't know for sure whether it's the same one as used in the
      file until we have continued and read the first instrument
      definition in the device.

      Device and instrument numbers in the file have always been
      problematic in other ways as well.  Rosegarden actually never
      used the device numbers loaded from the file (we always made up
      a new device number for each device as we read it) and it was
      theoretically possible for the instrument definitions to end up
      attached to different devices from the ones they were hung onto
      in the file (because of the way we sometimes re-used instruments
      that already existed in the studio).

      Our new approach is to assume that we will _never_ believe the
      device or instrument numbers found in the file, except for the
      purposes of resolving internal references within the file.  (The
      main examples of these are track -> instrument mappings outside
      of the studio definition, and metronome -> instrument mappings
      within a device.)  Instead, we assign a new device ID to each
      device based on its type and the available spare IDs; we assign
      a new instrument ID to each instrument based on the device type;
      and we maintain a map from "file" to "actual" instrument IDs as
      we go along, using this to resolve the track -> instrument and
      metronome -> instrument references at the end of the file (see
      the endElement method for rosegarden-data element).
    */

    /*
      This function is first called for any given instrument when
      first reading that instrument's definition in the context of a
      device definition.  So we know that if the instrument is not in
      the map already, then m_deviceInstrumentBase must contain a
      valid instrument base ID that was set when we added the device.

      To map from the number we just read, we subtract
      m_deviceReadInstrumentBase (if it exists; otherwise we set it as
      this is the first instrument for this device) and add
      m_deviceInstrumentBase.
    */

    if (m_actualInstrumentIdMap.find(oldId) != m_actualInstrumentIdMap.end()) {
        return m_actualInstrumentIdMap[oldId];
    }

    InstrumentId id = oldId;

    // here be dark wartortles

    if (m_deviceReadInstrumentBase == 0 || id < m_deviceReadInstrumentBase) {
        m_deviceReadInstrumentBase = id;
    }
    id = id - m_deviceReadInstrumentBase;
    id = id + m_deviceInstrumentBase;

    RG_DEBUG << "RoseXmlHandler::mapToActualInstrument: instrument " << oldId
             << ", dev read base " << m_deviceReadInstrumentBase
             << ", dev base " << m_deviceInstrumentBase << " -> " << id << endl;

    m_actualInstrumentIdMap[oldId] = id;

    return id;
}

void
RoseXmlHandler::skipToNextPlayDevice()
{
    SEQMAN_DEBUG << "RoseXmlHandler::skipToNextPlayDevice; m_deviceRunningId is " << m_deviceRunningId << endl;

    for (DeviceList::iterator i = getStudio().getDevices()->begin();
            i != getStudio().getDevices()->end(); ++i) {

        MidiDevice *md = dynamic_cast<MidiDevice *>(*i);

        if (md && md->getDirection() == MidiDevice::Play) {
            if (m_deviceRunningId == Device::NO_DEVICE ||
                md->getId() > m_deviceRunningId) {

                SEQMAN_DEBUG << "RoseXmlHandler::skipToNextPlayDevice: found next device: id " << md->getId() << endl;

                m_device = md;
                m_deviceRunningId = md->getId();
                return ;
            }
        }
    }

    SEQMAN_DEBUG << "RoseXmlHandler::skipToNextPlayDevice: fresh out of devices" << endl;

    m_device = 0;
}

void
RoseXmlHandler::setMIDIDeviceConnection(QString connection)
{
    SEQMAN_DEBUG << "RoseXmlHandler::setMIDIDeviceConnection(" << connection << ")" << endl;

    MidiDevice *md = dynamic_cast<MidiDevice *>(m_device);
    if (!md) return;

    RosegardenSequencer::getInstance()->setPlausibleConnection
        (md->getId(), connection);
    // We sync connection now, otherwise we'll confuse
    // MidiDevice::setConnection.
    md->setConnection(qstrtostr(connection));
}

void
RoseXmlHandler::setMIDIDeviceName(QString name)
{
    SEQMAN_DEBUG << "RoseXmlHandler::setMIDIDeviceName(" << name << ")" << endl;

    MidiDevice *md = dynamic_cast<MidiDevice *>(m_device);
    if (!md) return;

    RosegardenSequencer::getInstance()->renameDevice
        (md->getId(), name);
}

}
