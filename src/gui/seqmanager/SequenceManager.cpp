/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "SequenceManager.h"

#include "sound/Midi.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "document/ConfigGroups.h"
#include "base/Composition.h"
#include "base/Device.h"
#include "base/Exception.h"
#include "base/Instrument.h"
#include "base/MidiProgram.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "base/Studio.h"
#include "base/Track.h"
#include "base/TriggerSegment.h"
#include "CompositionMmapper.h"
#include "document/RosegardenGUIDoc.h"
#include "document/MultiViewCommandHistory.h"
#include "gui/application/RosegardenApplication.h"
#include "gui/application/RosegardenGUIApp.h"
#include "gui/application/RosegardenGUIView.h"
#include "gui/dialogs/AudioManagerDialog.h"
#include "gui/dialogs/CountdownDialog.h"
#include "gui/dialogs/TransportDialog.h"
#include "gui/kdeext/KStartupLogo.h"
#include "gui/studio/StudioControl.h"
#include "gui/widgets/CurrentProgressDialog.h"
#include "sequencer/RosegardenSequencer.h"
#include "MetronomeMmapper.h"
#include "SegmentMmapperFactory.h"
#include "SequencerMapper.h"
#include "ControlBlockMmapper.h"
#include "sound/AudioFile.h"
#include "sound/MappedComposition.h"
#include "sound/MappedEvent.h"
#include "sound/MappedInstrument.h"
#include "sound/SoundDriver.h"
#include "TempoSegmentMmapper.h"
#include "TimeSigSegmentMmapper.h"
#include <klocale.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <kglobal.h>
#include <QMessageBox>
#include <QApplication>
#include <QByteArray>
#include <QCursor>
#include <QDataStream>
#include <QEvent>
#include <QObject>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <algorithm>


namespace Rosegarden
{

SequenceManager::SequenceManager(RosegardenGUIDoc *doc,
                                 TransportDialog *transport):
            m_doc(doc),
            m_compositionMmapper(new CompositionMmapper(m_doc)),
            m_controlBlockMmapper(new ControlBlockMmapper(m_doc)),
            m_metronomeMmapper(SegmentMmapperFactory::makeMetronome(m_doc)),
            m_tempoSegmentMmapper(SegmentMmapperFactory::makeTempo(m_doc)),
            m_timeSigSegmentMmapper(SegmentMmapperFactory::makeTimeSig(m_doc)),
            m_transportStatus(STOPPED),
            m_soundDriverStatus(NO_DRIVER),
            m_transport(transport),
            m_lastRewoundAt(clock()),
            m_countdownDialog(0),
            m_countdownTimer(new QTimer(m_doc)),
            m_shownOverrunWarning(false),
            m_recordTime(new QTime()),
            m_compositionRefreshStatusId(m_doc->getComposition().getNewRefreshStatusId()),
            m_updateRequested(true),
            m_compositionMmapperResetTimer(new QTimer(m_doc)),
            m_sequencerMapper(0),
            m_reportTimer(new QTimer(m_doc)),
            m_canReport(true),
            m_lastLowLatencySwitchSent(false),
            m_lastTransportStartPosition(0),
            m_sampleRate(0)
{
    // Replaced this with a call to cleanup() from composition mmapper ctor:
    // if done here, this removes the mmapped versions of any segments stored
    // in the autoload (that have only just been mapped by the ctor!)
    //    m_compositionMmapper->cleanup();

    m_countdownDialog = new CountdownDialog(dynamic_cast<QWidget*>
                                            (m_doc->parent())->parentWidget());
    // Connect these for use later
    //
    connect(m_countdownTimer, SIGNAL(timeout()),
            this, SLOT(slotCountdownTimerTimeout()));

    connect(m_reportTimer, SIGNAL(timeout()),
            this, SLOT(slotAllowReport()));

    connect(m_compositionMmapperResetTimer, SIGNAL(timeout()),
            this, SLOT(slotScheduledCompositionMmapperReset()));


    connect(doc->getCommandHistory(), SIGNAL(commandExecuted()),
            this, SLOT(update()));

    m_doc->getComposition().addObserver(this);

    // The owner of this sequence manager will need to call
    // checkSoundDriverStatus on it to set up its status appropriately
    // immediately after construction; we used to do it from here but
    // we're not well placed to handle reporting to the user if it
    // throws an exception (and we don't want to leave the object half
    // constructed).

    // Try to map the sequencer file
    //
    mapSequencer();
}

SequenceManager::~SequenceManager()
{
    m_doc->getComposition().removeObserver(this);

    SEQMAN_DEBUG << "SequenceManager::~SequenceManager()\n";
    delete m_compositionMmapper;
    delete m_controlBlockMmapper;
    delete m_metronomeMmapper;
    delete m_tempoSegmentMmapper;
    delete m_timeSigSegmentMmapper;
    delete m_sequencerMapper;
}

void SequenceManager::setDocument(RosegardenGUIDoc* doc)
{
    SEQMAN_DEBUG << "SequenceManager::setDocument(" << doc << ")\n";

    DataBlockRepository::clear();

    m_doc->getComposition().removeObserver(this);
    disconnect(m_doc->getCommandHistory(), SIGNAL(commandExecuted()));

    m_segments.clear();
    m_triggerSegments.clear();

    m_doc = doc;
    Composition &comp = m_doc->getComposition();

    // Must recreate and reconnect the countdown timer and dialog
    // (bug 729039)
    //
    delete m_countdownDialog;
    delete m_countdownTimer;
    delete m_compositionMmapperResetTimer;

    m_countdownDialog = new CountdownDialog(dynamic_cast<QWidget*>
                                            (m_doc->parent())->parentWidget());

    // Bug 933041: no longer connect the CountdownDialog from
    // SequenceManager; instead let the RosegardenGUIApp connect it to
    // its own slotStop to ensure the right housekeeping is done

    m_countdownTimer = new QTimer(m_doc);

    // Connect this for use later
    //
    connect(m_countdownTimer, SIGNAL(timeout()),
            this, SLOT(slotCountdownTimerTimeout()));

    m_compositionRefreshStatusId = comp.getNewRefreshStatusId();
    comp.addObserver(this);

    connect(m_doc->getCommandHistory(), SIGNAL(commandExecuted()),
            this, SLOT(update()));

    for (Composition::iterator i = comp.begin(); i != comp.end(); ++i) {

        SEQMAN_DEBUG << "Adding segment with rid " << (*i)->getRuntimeId() << endl;

        m_segments.insert(SegmentRefreshMap::value_type
                          (*i, (*i)->getNewRefreshStatusId()));
    }

    for (Composition::triggersegmentcontaineriterator i =
                comp.getTriggerSegments().begin();
            i != comp.getTriggerSegments().end(); ++i) {
        m_triggerSegments.insert(SegmentRefreshMap::value_type
                                 ((*i)->getSegment(),
                                  (*i)->getSegment()->getNewRefreshStatusId()));
    }


    m_compositionMmapperResetTimer = new QTimer(m_doc);
    connect(m_compositionMmapperResetTimer, SIGNAL(timeout()),
            this, SLOT(slotScheduledCompositionMmapperReset()));

    resetCompositionMmapper();

    // Try to map the sequencer file
    //
    mapSequencer();
}

void
SequenceManager::setTransportStatus(const TransportStatus &status)
{
    m_transportStatus = status;
}

void
SequenceManager::mapSequencer()
{
    if (m_sequencerMapper)
        return ;

    try {
        m_sequencerMapper = new SequencerMapper(
                                KGlobal::dirs()->resourceDirs("tmp").last() + "/rosegarden_sequencer_timing_block");
    } catch (Exception) {
        m_sequencerMapper = 0;
    }
}

bool
SequenceManager::play()
{
    mapSequencer();

    Composition &comp = m_doc->getComposition();

    // If already playing or recording then stop
    //
    if (m_transportStatus == PLAYING ||
            m_transportStatus == RECORDING ) {
        stopping();
        return true;
    }

    // This check may throw an exception
    checkSoundDriverStatus(false);

    // Align Instrument lists and send initial program changes
    //
    preparePlayback();

    m_lastTransportStartPosition = comp.getPosition();

    // Update play metronome status
    //
    m_controlBlockMmapper->updateMetronomeData
    (m_metronomeMmapper->getMetronomeInstrument());
    m_controlBlockMmapper->updateMetronomeForPlayback();

    // make sure we toggle the play button
    //
    m_transport->PlayButton()->setOn(true);

    //!!! disable the record button, because recording while playing is horribly
    // broken, and disabling it is less complicated than fixing it
    // see #1223025 - DMM
    //    SEQMAN_DEBUG << "SequenceManager::play() - disabling record button, as we are playing\n";
    //    m_transport->RecordButton()->setEnabled(false);

    if (comp.getCurrentTempo() == 0) {
        comp.setCompositionDefaultTempo(comp.getTempoForQpm(120.0));

        SEQMAN_DEBUG << "SequenceManager::play() - setting Tempo to Default value of 120.000\n";
    } else {
        SEQMAN_DEBUG << "SequenceManager::play() - starting to play\n";
    }

    // Send initial tempo
    //
    double qnD = 60.0 / comp.getTempoQpm(comp.getCurrentTempo());
    RealTime qnTime =
        RealTime(long(qnD),
                 long((qnD - double(long(qnD))) * 1000000000.0));
    StudioControl::sendQuarterNoteLength(qnTime);

    // set the tempo in the transport
    m_transport->setTempo(comp.getCurrentTempo());

    // The arguments for the Sequencer
    RealTime startPos = comp.getElapsedRealTime(comp.getPosition());

    // If we're looping then jump to loop start
    if (comp.isLooping())
        startPos = comp.getElapsedRealTime(comp.getLoopStart());

    QSettings config;
    config.beginGroup( SequencerOptionsConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // config.endGroup();		// corresponding to: config.beginGroup( SequencerOptionsConfigGroup );
    //  


    bool lowLat = qStrToBool( config.value("audiolowlatencymonitoring", "true" ) ) ;

    if (lowLat != m_lastLowLatencySwitchSent) {
        RosegardenSequencer::getInstance()->setLowLatencyMode(lowLat);
        m_lastLowLatencySwitchSent = lowLat;
    }

    RealTime readAhead, audioMix, audioRead, audioWrite;
    long smallFileSize;

    // Apart from perhaps the small file size, I think with hindsight
    // that these options are more easily set to reasonable defaults
    // here than left to the user.  Mostly.

    //!!! need some cleverness somewhere to ensure the read-ahead
    //is larger than the JACK period size

    if (lowLat) {
        readAhead  = RealTime(0, 160000000);
        audioMix   = RealTime(0, 60000000); // ignored in lowlat mode
        audioRead  = RealTime(2, 500000000); // audio read nsec
        audioWrite = RealTime(4, 0);
        smallFileSize = 256; // K
    } else {
        readAhead  = RealTime(0, 500000000);
        audioMix   = RealTime(0, 400000000); // ignored in lowlat mode
        audioRead  = RealTime(2, 500000000); // audio read nsec
        audioWrite = RealTime(4, 0);
        smallFileSize = 256; // K
    }

    int result = 
        RosegardenSequencer::getInstance()->
        play(startPos, readAhead, audioMix, audioRead, audioWrite, smallFileSize);

    if (result) {
        // completed successfully
        m_transportStatus = STARTING_TO_PLAY;
    } else {
        m_transportStatus = STOPPED;
        std::cerr << "ERROR: SequenceManager::play(): Failed to start playback!" << std::endl;
    }

    return false;
}

void
SequenceManager::stopping()
{
    if (m_countdownTimer)
        m_countdownTimer->stop();
    if (m_countdownDialog)
        m_countdownDialog->hide();

    // Do this here rather than in stop() to avoid any potential
    // race condition (we use setPointerPosition() during stop()).
    //
    if (m_transportStatus == STOPPED) {
        /*!!!
                if (m_doc->getComposition().isLooping())
                    m_doc->slotSetPointerPosition(m_doc->getComposition().getLoopStart());
                else
                    m_doc->slotSetPointerPosition(m_doc->getComposition().getStartMarker());
        */
        m_doc->slotSetPointerPosition(m_lastTransportStartPosition);

        return ;
    }

    // Disarm recording and drop back to STOPPED
    //
    if (m_transportStatus == RECORDING_ARMED) {
        m_transportStatus = STOPPED;
        m_transport->RecordButton()->setOn(false);
        m_transport->MetronomeButton()->
        setOn(m_doc->getComposition().usePlayMetronome());
        return ;
    }

    SEQMAN_DEBUG << "SequenceManager::stopping() - preparing to stop\n";

    //    SEQMAN_DEBUG << kdBacktrace() << endl;

    stop();

    m_shownOverrunWarning = false;
}

void
SequenceManager::stop()
{
    // Toggle off the buttons - first record
    //
    if (m_transportStatus == RECORDING) {
        m_transport->RecordButton()->setOn(false);
        m_transport->MetronomeButton()->
        setOn(m_doc->getComposition().usePlayMetronome());

        // Remove the countdown dialog and stop the timer
        //
        m_countdownDialog->hide();
        m_countdownTimer->stop();
    }

    // Now playback
    m_transport->PlayButton()->setOn(false);

    // re-enable the record button if it was previously disabled when
    // going into play mode - DMM
    //    SEQMAN_DEBUG << "SequenceManager::stop() - re-enabling record button\n";
    //    m_transport->RecordButton()->setEnabled(true);


    // "call" the sequencer with a stop so we get a synchronous
    // response - then we can fiddle about with the audio file
    // without worrying about the sequencer causing problems
    // with access to the same audio files.
    //

    // wait cursor
    //
    QApplication::setOverrideCursor(QCursor(Qt::waitCursor));

    RosegardenSequencer::getInstance()->stop();

    // restore
    QApplication::restoreOverrideCursor();

    TransportStatus status = m_transportStatus;

    // set new transport status first, so that if we're stopping
    // recording we don't risk the record segment being restored by a
    // timer while the document is busy trying to do away with it
    m_transportStatus = STOPPED;

    // if we're recording MIDI or Audio then tidy up the recording Segment
    if (status == RECORDING) {
        m_doc->stopRecordingMidi();
        m_doc->stopRecordingAudio();

        SEQMAN_DEBUG << "SequenceManager::stop() - stopped recording\n";
    }

    // always untoggle the play button at this stage
    //
    m_transport->PlayButton()->setOn(false);
    SEQMAN_DEBUG << "SequenceManager::stop() - stopped playing\n";

    // We don't reset controllers at this point - what happens with static
    // controllers the next time we play otherwise?  [rwb]
    //resetControllers();
}

void
SequenceManager::rewind()
{
    Composition &composition = m_doc->getComposition();

    timeT position = composition.getPosition();
    std::pair<timeT, timeT> barRange =
        composition.getBarRangeForTime(position - 1);

    if (m_transportStatus == PLAYING) {

        // if we're playing and we had a rewind request less than 200ms
        // ago and we're some way into the bar but less than half way
        // through it, rewind two barlines instead of one

        clock_t now = clock();
        int elapsed = (now - m_lastRewoundAt) * 1000 / CLOCKS_PER_SEC;

        SEQMAN_DEBUG << "That was " << m_lastRewoundAt << ", this is " << now << ", elapsed is " << elapsed << endl;

        if (elapsed >= 0 && elapsed <= 200) {
            if (position > barRange.first &&
                    position < barRange.second &&
                    position <= (barRange.first + (barRange.second -
                                                   barRange.first) / 2)) {
                barRange = composition.getBarRangeForTime(barRange.first - 1);
            }
        }

        m_lastRewoundAt = now;
    }

    if (barRange.first < composition.getStartMarker()) {
        m_doc->slotSetPointerPosition(composition.getStartMarker());
    } else {
        m_doc->slotSetPointerPosition(barRange.first);
    }
}

void
SequenceManager::fastforward()
{
    Composition &composition = m_doc->getComposition();

    timeT position = composition.getPosition() + 1;
    timeT newPosition = composition.getBarRangeForTime(position).second;

    // Don't skip past end marker
    //
    if (newPosition > composition.getEndMarker())
        newPosition = composition.getEndMarker();

    m_doc->slotSetPointerPosition(newPosition);

}

void
SequenceManager::notifySequencerStatus(TransportStatus status)
{
    // for the moment we don't do anything fancy
    m_transportStatus = status;

}

void
SequenceManager::sendSequencerJump(const RealTime &time)
{
    RosegardenSequencer::getInstance()->jumpTo(time);
}

void
SequenceManager::record(bool toggled)
{
    mapSequencer();

    SEQMAN_DEBUG << "SequenceManager::record(" << toggled << ")" << endl;

    Composition &comp = m_doc->getComposition();
    Studio &studio = m_doc->getStudio();
    QSettings config;
    config.beginGroup( GeneralOptionsConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // config.endGroup();		// corresponding to: config.beginGroup( GeneralOptionsConfigGroup );
    //  


    bool punchIn = false; // are we punching in?

    // If we have any audio tracks armed, then we need to check for
    // a valid audio record path and a working audio subsystem before
    // we go any further

    const Composition::recordtrackcontainer &recordTracks =
        comp.getRecordTracks();

    for (Composition::recordtrackcontainer::const_iterator i =
                recordTracks.begin();
            i != recordTracks.end(); ++i) {

        Track *track = comp.getTrackById(*i);
        InstrumentId instrId = track->getInstrument();
        Instrument *instr = studio.getInstrumentById(instrId);

        if (instr && instr->getType() == Instrument::Audio) {
            if (!m_doc || !(m_soundDriverStatus & AUDIO_OK)) {
                m_transport->RecordButton()->setOn(false);
                throw(Exception("Audio subsystem is not available - can't record audio"));
            }
            // throws BadAudioPathException if path is not valid:
            m_doc->getAudioFileManager().testAudioPath();
            break;
        }
    }

    if (toggled) { // preparing record or punch-in record

        if (m_transportStatus == RECORDING_ARMED) {
            SEQMAN_DEBUG << "SequenceManager::record - unarming record\n";
            m_transportStatus = STOPPED;

            // Toggle the buttons
            m_transport->MetronomeButton()->setOn(comp.usePlayMetronome());
            m_transport->RecordButton()->setOn(false);

            return ;
        }

        if (m_transportStatus == STOPPED) {
            SEQMAN_DEBUG << "SequenceManager::record - armed record\n";
            m_transportStatus = RECORDING_ARMED;

            // Toggle the buttons
            m_transport->MetronomeButton()->setOn(comp.useRecordMetronome());
            m_transport->RecordButton()->setOn(true);

            return ;
        }

        if (m_transportStatus == RECORDING) {
            SEQMAN_DEBUG << "SequenceManager::record - stop recording and keep playing\n";

            if (!RosegardenSequencer::getInstance()->punchOut()) {

                // #1797873 - set new transport status first, so that
                // if we're stopping recording we don't risk the
                // record segment being restored by a timer while the
                // document is busy trying to do away with it
                m_transportStatus = STOPPED;

                m_doc->stopRecordingMidi();
                m_doc->stopRecordingAudio();
                return ;
            }

            // #1797873 - as above
            m_transportStatus = PLAYING;

            m_doc->stopRecordingMidi();
            m_doc->stopRecordingAudio();

            return ;
        }

        if (m_transportStatus == PLAYING) {
            SEQMAN_DEBUG << "SequenceManager::record - punch in recording\n";
            punchIn = true;
            goto punchin;
        }

    } else {

        m_lastTransportStartPosition = comp.getPosition();

punchin:

        // Get the record tracks and check we have a record instrument

        bool haveInstrument = false;
        bool haveAudioInstrument = false;
        bool haveMIDIInstrument = false;
        //TrackId recordMIDITrack = 0;

        for (Composition::recordtrackcontainer::const_iterator i =
                    comp.getRecordTracks().begin();
                i != comp.getRecordTracks().end(); ++i) {

            InstrumentId iid =
                comp.getTrackById(*i)->getInstrument();

            Instrument *inst = studio.getInstrumentById(iid);
            if (inst) {
                haveInstrument = true;
                if (inst->getType() == Instrument::Audio) {
                    haveAudioInstrument = true;
                    break;
                } else { // soft synths count as MIDI for our purposes here
                    haveMIDIInstrument = true;
                    //recordMIDITrack = *i;
                }
            }
        }

        if (!haveInstrument) {
            m_transport->RecordButton()->setDown(false);
            throw(Exception("No Record instrument selected"));
        }

        // may throw an exception
        checkSoundDriverStatus(false);

        // toggle the Metronome button if it's in use
        m_transport->MetronomeButton()->setOn(comp.useRecordMetronome());

        // Update record metronome status
        //
        m_controlBlockMmapper->updateMetronomeData
        (m_metronomeMmapper->getMetronomeInstrument());
        m_controlBlockMmapper->updateMetronomeForRecord();

        // If we are looping then jump to start of loop and start recording,
        // if we're not take off the number of count-in bars and start
        // recording.
        //
        if (comp.isLooping())
            m_doc->slotSetPointerPosition(comp.getLoopStart());
        else {
            if (m_transportStatus != RECORDING_ARMED && punchIn == false) {
                int startBar = comp.getBarNumber(comp.getPosition());
                startBar -= config.value("countinbars", 0).toUInt() ;
                m_doc->slotSetPointerPosition(comp.getBarRange(startBar).first);
            }
        }

        m_doc->setRecordStartTime(m_doc->getComposition().getPosition());

        if (haveAudioInstrument) {
            // Ask the document to update its record latencies so as to
            // do latency compensation when we stop
            m_doc->updateAudioRecordLatency();
        }

        if (haveMIDIInstrument) {
            // Create the record MIDI segment now, so that the
            // composition view has a real segment to display.  It
            // won't actually be added to the composition until the
            // first recorded event arrives.  We don't have to do this
            // from here for audio, because for audio the sequencer
            // calls back on createRecordAudioFiles so as to find out
            // what files it needs to write to.
            /*m_doc->addRecordMIDISegment(recordMIDITrack);*/
            for (Composition::recordtrackcontainer::const_iterator i =
                        comp.getRecordTracks().begin(); i != comp.getRecordTracks().end(); ++i) {
                InstrumentId iid = comp.getTrackById(*i)->getInstrument();
                Instrument *inst = studio.getInstrumentById(iid);
                if (inst && (inst->getType() != Instrument::Audio)) {
                    SEQMAN_DEBUG << "SequenceManager:  mdoc->addRecordMIDISegment(" << *i << ")" << endl;
                    m_doc->addRecordMIDISegment(*i);
                }
            }
        }

        // set the buttons
        m_transport->RecordButton()->setOn(true);
        m_transport->PlayButton()->setOn(true);

        if (comp.getCurrentTempo() == 0) {
            SEQMAN_DEBUG << "SequenceManager::play() - setting Tempo to Default value of 120.000\n";
            comp.setCompositionDefaultTempo(comp.getTempoForQpm(120.0));
        } else {
            SEQMAN_DEBUG << "SequenceManager::record() - starting to record\n";
        }

        // set the tempo in the transport
        //
        m_transport->setTempo(comp.getCurrentTempo());

        // The arguments for the Sequencer - record is similar to playback,
        // we must being playing to record.
        //
        RealTime startPos =
            comp.getElapsedRealTime(comp.getPosition());

        bool lowLat = qStrToBool( config.value("audiolowlatencymonitoring", "true" ) ) ;

        if (lowLat != m_lastLowLatencySwitchSent) {
            RosegardenSequencer::getInstance()->setLowLatencyMode(lowLat);
            m_lastLowLatencySwitchSent = lowLat;
        }

        RealTime readAhead, audioMix, audioRead, audioWrite;
        long smallFileSize;

        // Apart from perhaps the small file size, I think with hindsight
        // that these options are more easily set to reasonable defaults
        // here than left to the user.  Mostly.

        //!!! Duplicates code in play()

        //!!! need some cleverness somewhere to ensure the read-ahead
        //is larger than the JACK period size

        if (lowLat) {
            readAhead  = RealTime(0, 160000000);
            audioMix   = RealTime(0, 60000000); // ignored in lowlat mode
            audioRead  = RealTime(2, 500000000); // audio read nsec
            audioWrite = RealTime(4, 0);
            smallFileSize = 256; // K
        } else {
            readAhead  = RealTime(0, 500000000);
            audioMix   = RealTime(0, 400000000); // ignored in lowlat mode
            audioRead  = RealTime(2, 500000000); // audio read nsec
            audioWrite = RealTime(4, 0);
            smallFileSize = 256; // K
        }

        int result = 
            RosegardenSequencer::getInstance()->
            record(startPos, readAhead, audioMix, audioRead, audioWrite, smallFileSize,
                   STARTING_TO_RECORD);

        if (result) {

            // completed successfully
            m_transportStatus = STARTING_TO_RECORD;

            // Create the countdown timer dialog to show recording time
            // remaining.  (Note (dmm) this has changed, and it now reports
            // the time remaining during both MIDI and audio recording.)
            //
            timeT p = comp.getPosition();
            timeT d = comp.getEndMarker();
            // end marker less current position == available duration
            d -= p;

            // set seconds to total possible time, initially
            RealTime rtd = comp.getElapsedRealTime(d);
            int seconds = rtd.sec;

            // re-initialise
            m_countdownDialog->setTotalTime(seconds);

            // Create the timer
            //
            m_recordTime->start();

            // Start an elapse timer for updating the dialog -
            // it will fire every second.
            //
            m_countdownTimer->start(1000);

            // Pop-up the dialog (don't use exec())
            //
            // bug #1505805, abolish recording countdown dialog
            //m_countdownDialog->show();

        } else {
            // Stop immediately - turn off buttons in parent
            //
            m_transportStatus = STOPPED;

            if (haveAudioInstrument) {
                throw(Exception("Couldn't start recording audio.\nPlease set a valid file path in the Document Properties\n(Composition menu -> Edit Document Properties -> Audio)."));
            }
        }
    }
}

void
SequenceManager::processAsynchronousMidi(const MappedComposition &mC,
                                         AudioManagerDialog *audioManagerDialog)
{
    static bool boolShowingWarning = false;
    static bool boolShowingALSAWarning = false;
    static long warningShownAt = 0;

    if (m_doc == 0 || mC.size() == 0)
        return ;

    MappedComposition::const_iterator i;

    // before applying through-filter, catch program-change-messages
    int prg;
    int bnk_msb;
    int bnk_lsb;
    bnk_lsb = -1;
    bnk_lsb = -1;

    for (i = mC.begin(); i != mC.end(); ++i ) {
		
        // catch bank selects (lsb)
        if ((*i)->getType() == MappedEvent::MidiController){
            prg = (*i)->getData1();
            if (prg == 32 ) {
                // then it's a Bank Select (fine, LSB)
                // get bank nr: 
                bnk_lsb = (*i)->getData2();
            }else 
                if (prg == 0 ) {
                    // then it's a Bank Select (coarse, MSB)
                    // get msb value: 
                    bnk_msb = (*i)->getData2();
                }
        }
        // catch program changes
        if ((*i)->getType() == MappedEvent::MidiProgramChange) {
            // this selects the program-list entry on prog-change-messages 
            // as well as the previously received bank select (lsb)
            prg = (*i)->getData1();
            emit signalSelectProgramNoSend( prg, bnk_lsb, bnk_msb );
        }
    }
	
    // Thru filtering is done at the sequencer for the actual sound
    // output, but here we need both filtered (for OUT display) and
    // unfiltered (for insertable note callbacks) compositions, so
    // we've received the unfiltered copy and will filter here
    MappedComposition tempMC =
        applyFiltering(mC,
                       MappedEvent::MappedEventType(
                           m_doc->getStudio().getMIDIThruFilter()));

    // send to the MIDI labels (which can only hold one event at a time)
    i = mC.begin();
    if (i != mC.end()) {
        m_transport->setMidiInLabel(*i);
    }

    i = tempMC.begin();
    while (i != tempMC.end()) {
        if ((*i)->getRecordedDevice() != Device::CONTROL_DEVICE) {
            m_transport->setMidiOutLabel(*i);
            break;
        }
        ++i;
    }

    for (i = mC.begin(); i != mC.end(); ++i ) {
        if ((*i)->getType() >= MappedEvent::Audio) {
            if ((*i)->getType() == MappedEvent::AudioStopped) {
                /*
                  SEQMAN_DEBUG << "AUDIO FILE ID = "
                  << int((*i)->getData1())
                  << " - FILE STOPPED - " 
                  << "INSTRUMENT = "
                  << (*i)->getInstrument()
                  << endl;
                */

                if (audioManagerDialog && (*i)->getInstrument() ==
                        m_doc->getStudio().getAudioPreviewInstrument()) {
                    audioManagerDialog->
                    closePlayingDialog(
                        AudioFileId((*i)->getData1()));
                }
            }

            if ((*i)->getType() == MappedEvent::AudioLevel)
                sendAudioLevel(*i);

            if ((*i)->getType() ==
                    MappedEvent::AudioGeneratePreview) {
                SEQMAN_DEBUG << "Received AudioGeneratePreview: data1 is " << int((*i)->getData1()) << ", data2 " << int((*i)->getData2()) << ", instrument is " << (*i)->getInstrument() << endl;

                m_doc->finalizeAudioFile((int)(*i)->getData1() +
                                         (int)(*i)->getData2() * 256);
            }
            
            if ((*i)->getType() ==
                MappedEvent::SystemUpdateInstruments) {
                // resync Devices and Instruments
                //
                m_doc->syncDevices();

                /*QSettings config;
                  		config.beginGroup( SequencerOptionsConfigGroup );
                  		// 
                  		// FIX-manually-(GW), add:
                  		// config.endGroup();		// corresponding to: config.beginGroup( SequencerOptionsConfigGroup );
                  		//  

                QString recordDeviceStr = config.value("midirecorddevice") ;
                sendMIDIRecordingDevice(recordDeviceStr);*/
                restoreRecordSubscriptions();
            }

            if (m_transportStatus == PLAYING ||
                m_transportStatus == RECORDING) {
                if ((*i)->getType() == MappedEvent::SystemFailure) {

                    SEQMAN_DEBUG << "Failure of some sort..." << endl;

                    bool handling = true;

                    /* These are the ones that we always report or handle. */

                    if ((*i)->getData1() == MappedEvent::FailureJackDied) {

                        // Something horrible has happened to JACK or we got
                        // bumped out of the graph.  Either way stop playback.
                        //
                        stopping();

                    } else if ((*i)->getData1() == MappedEvent::FailureJackRestartFailed) {

                        QMessageBox::error(
                            dynamic_cast<QWidget*>(m_doc->parent())->parentWidget(),
                            i18n("The JACK Audio subsystem has failed or it has stopped Rosegarden from processing audio.\nPlease restart Rosegarden to continue working with audio.\nQuitting other running applications may improve Rosegarden's performance."));

                    } else if ((*i)->getData1() == MappedEvent::FailureJackRestart) {

                        QMessageBox::error(
                            dynamic_cast<QWidget*>(m_doc->parent())->parentWidget(),
                            i18n("The JACK Audio subsystem has stopped Rosegarden from processing audio, probably because of a processing overload.\nAn attempt to restart the audio service has been made, but some problems may remain.\nQuitting other running applications may improve Rosegarden's performance."));

                    } else if ((*i)->getData1() == MappedEvent::FailureCPUOverload) {

#define REPORT_CPU_OVERLOAD 1
#ifdef REPORT_CPU_OVERLOAD

                        stopping();

                        QMessageBox::error(
                            dynamic_cast<QWidget*>(m_doc->parent())->parentWidget(),
                            i18n("Run out of processor power for real-time audio processing.  Cannot continue."));

#endif

                    } else {

                        handling = false;
                    }

                    if (handling)
                        continue;

                    if (!m_canReport) {
                        SEQMAN_DEBUG << "Not reporting it to user just yet"
                        << endl;
                        continue;
                    }

                    if ((*i)->getData1() == MappedEvent::FailureALSACallFailed) {

                        struct timeval tv;
                        (void)gettimeofday(&tv, 0);

                        if (tv.tv_sec - warningShownAt >= 5 &&
                                !boolShowingALSAWarning) {

                            QString message = i18n("A serious error has occurred in the ALSA MIDI subsystem.  It may not be possible to continue sequencing.  Please check console output for more information.");
                            boolShowingALSAWarning = true;

                            QMessageBox::information(0, message);
                            boolShowingALSAWarning = false;

                            (void)gettimeofday(&tv, 0);
                            warningShownAt = tv.tv_sec;
                        }

                    } else if ((*i)->getData1() == MappedEvent::FailureXRuns) {

                        //#define REPORT_XRUNS 1
#ifdef REPORT_XRUNS

                        struct timeval tv;
                        (void)gettimeofday(&tv, 0);

                        if (tv.tv_sec - warningShownAt >= 5 &&
                                !boolShowingWarning) {

                            QString message = i18n("JACK Audio subsystem is losing sample frames.");
                            boolShowingWarning = true;

                            QMessageBox::information(0, message);
                            boolShowingWarning = false;

                            (void)gettimeofday(&tv, 0);
                            warningShownAt = tv.tv_sec;
                        }
#endif

                    } else if (!m_shownOverrunWarning) {

                        QString message;

                        switch ((*i)->getData1()) {

                        case MappedEvent::FailureDiscUnderrun:
                            message = i18n("Failed to read audio data from disc in time to service the audio subsystem.");
                            break;

                        case MappedEvent::FailureDiscOverrun:
                            message = i18n("Failed to write audio data to disc fast enough to service the audio subsystem.");
                            break;

                        case MappedEvent::FailureBussMixUnderrun:
                            message = i18n("The audio mixing subsystem is failing to keep up.");
                            break;

                        case MappedEvent::FailureMixUnderrun:
                            message = i18n("The audio subsystem is failing to keep up.");
                            break;

                        default:
                            message = i18n("Unknown sequencer failure mode!");
                            break;
                        }

                        m_shownOverrunWarning = true;

#ifdef REPORT_XRUNS

                        QMessageBox::information(0, message);
#else

                        if ((*i)->getData1() == MappedEvent::FailureDiscOverrun) {
                            // the error you can't hear
                            QMessageBox::information(0, message);
                        } else {
                            std::cerr << message << std::endl;
                        }
#endif

                    }

                    // Turn off the report flag and set off a one-shot
                    // timer for 5 seconds.
                    //
                    if (!m_reportTimer->isActive()) {
                        m_canReport = false;
                        m_reportTimer->start(5000, true);
                    }
                }
            } else {
                KStartupLogo::hideIfStillThere();

                if ((*i)->getType() == MappedEvent::SystemFailure) {

                    if ((*i)->getData1() == MappedEvent::FailureJackRestartFailed) {

                        QMessageBox::error(
                            dynamic_cast<QWidget*>(m_doc->parent()),
                            i18n("The JACK Audio subsystem has failed or it has stopped Rosegarden from processing audio.\nPlease restart Rosegarden to continue working with audio.\nQuitting other running applications may improve Rosegarden's performance."));

                    } else if ((*i)->getData1() == MappedEvent::FailureJackRestart) {

                        QMessageBox::error(
                            dynamic_cast<QWidget*>(m_doc->parent()),
                            i18n("The JACK Audio subsystem has stopped Rosegarden from processing audio, probably because of a processing overload.\nAn attempt to restart the audio service has been made, but some problems may remain.\nQuitting other running applications may improve Rosegarden's performance."));

                    } else if ((*i)->getData1() == MappedEvent::WarningImpreciseTimer &&
                               shouldWarnForImpreciseTimer()) {

                        std::cerr << "Rosegarden: WARNING: No accurate sequencer timer available" << std::endl;

                        KStartupLogo::hideIfStillThere();
                        CurrentProgressDialog::freeze();

                        RosegardenGUIApp::self()->awaitDialogClearance();

                        QMessageBox::information(
                            dynamic_cast<QWidget*>(m_doc->parent()),
                            i18n("<h3>System timer resolution is too low</h3><p>Rosegarden was unable to find a high-resolution timing source for MIDI performance.</p><p>This may mean you are using a Linux system with the kernel timer resolution set too low.  Please contact your Linux distributor for more information.</p><p>Some Linux distributors already provide low latency kernels, see <a href=\"http://rosegarden.wiki.sourceforge.net/Low+latency+kernels\">http://rosegarden.wiki.sourceforge.net/Low+latency+kernels</a> for instructions.</p>"), 
			    NULL, NULL, 
			    QMessageBox::Notify + QMessageBox::AllowLink);
                        
                        CurrentProgressDialog::thaw();

                    } else if ((*i)->getData1() == MappedEvent::WarningImpreciseTimerTryRTC &&
                               shouldWarnForImpreciseTimer()) {

                        std::cerr << "Rosegarden: WARNING: No accurate sequencer timer available" << std::endl;

                        KStartupLogo::hideIfStillThere();
                        CurrentProgressDialog::freeze();

                        RosegardenGUIApp::self()->awaitDialogClearance();

                        QMessageBox::information(
                            dynamic_cast<QWidget*>(m_doc->parent()),
                            i18n("<h3>System timer resolution is too low</h3><p>Rosegarden was unable to find a high-resolution timing source for MIDI performance.</p><p>You may be able to solve this problem by loading the RTC timer kernel module.  To do this, try running <b>sudo modprobe snd-rtctimer</b> in a terminal window and then restarting Rosegarden.</p><p>Alternatively, check whether your Linux distributor provides a multimedia-optimized kernel.  See <a href=\"http://rosegarden.wiki.sourceforge.net/Low+latency+kernels\">http://rosegarden.wiki.sourceforge.net/Low+latency+kernels</a> for notes about this.</p>"), 
			    NULL, NULL, 
			    QMessageBox::Notify + QMessageBox::AllowLink);
                        
                        CurrentProgressDialog::thaw();
                    }
                }
            }
        }
    }

    // if we aren't playing or recording, consider invoking any
    // step-by-step clients (using unfiltered composition).  send
    // out any incoming external controller events

    for (i = mC.begin(); i != mC.end(); ++i ) {
        if (m_transportStatus == STOPPED ||
            m_transportStatus == RECORDING_ARMED) {
            if ((*i)->getType() == MappedEvent::MidiNote) {
                if ((*i)->getVelocity() == 0) {
                    emit insertableNoteOffReceived((*i)->getPitch(), (*i)->getVelocity());
                } else {
                    emit insertableNoteOnReceived((*i)->getPitch(), (*i)->getVelocity());
                }
            }
        }
        if ((*i)->getRecordedDevice() == Device::CONTROL_DEVICE) {
            SEQMAN_DEBUG << "controllerDeviceEventReceived" << endl;
            emit controllerDeviceEventReceived(*i);
        }
    }
}

void
SequenceManager::rewindToBeginning()
{
    SEQMAN_DEBUG << "SequenceManager::rewindToBeginning()\n";
    m_doc->slotSetPointerPosition(m_doc->getComposition().getStartMarker());
}

void
SequenceManager::fastForwardToEnd()
{
    SEQMAN_DEBUG << "SequenceManager::fastForwardToEnd()\n";

    Composition &comp = m_doc->getComposition();
    m_doc->slotSetPointerPosition(comp.getDuration());
}

void
SequenceManager::setLoop(const timeT &lhs, const timeT &rhs)
{
    // do not set a loop if JACK transport sync is enabled, because this is
    // completely broken, and apparently broken due to a limitation of JACK
    // transport itself.  #1240039 - DMM
    //    QSettings //    config;
    //    config.beginGroup( SequencerOptionsConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // //    config.endGroup();		// corresponding to: //    config.beginGroup( SequencerOptionsConfigGroup );
    //  

    //    if ( qStrToBool( config.value("jacktransport", "false" ) ) )
    //    {
    //	//!!! message box should go here to inform user of why the loop was
    //	// not set, but I can't add it at the moment due to to the pre-release
    //	// freeze - DMM
    //	return;
    //    }

    RealTime loopStart =
        m_doc->getComposition().getElapsedRealTime(lhs);
    RealTime loopEnd =
        m_doc->getComposition().getElapsedRealTime(rhs);

    RosegardenSequencer::getInstance()->setLoop(loopStart, loopEnd);
}

void
SequenceManager::checkSoundDriverStatus(bool warnUser)
{
    m_soundDriverStatus = RosegardenSequencer::getInstance()->
        getSoundDriverStatus(VERSION);

    SEQMAN_DEBUG << "Sound driver status is: " << m_soundDriverStatus << endl;

    if (!warnUser) return;

#ifdef HAVE_LIBJACK
    if ((m_soundDriverStatus & (AUDIO_OK | MIDI_OK | VERSION_OK)) ==
        (AUDIO_OK | MIDI_OK | VERSION_OK)) return;
#else
    if ((m_soundDriverStatus & (MIDI_OK | VERSION_OK)) ==
        (MIDI_OK | VERSION_OK)) return;
#endif

    KStartupLogo::hideIfStillThere();
    CurrentProgressDialog::freeze();

    QString text = "";

    if (m_soundDriverStatus == NO_DRIVER) {
        text = i18n("<p>Both MIDI and Audio subsystems have failed to initialize.</p><p>You may continue without the sequencer, but we suggest closing Rosegarden, running \"alsaconf\" as root, and starting Rosegarden again.  If you wish to run with no sequencer by design, then use \"rosegarden --nosequencer\" to avoid seeing this error in the future.</p>");
    } else if (!(m_soundDriverStatus & MIDI_OK)) {
        text = i18n("<p>The MIDI subsystem has failed to initialize.</p><p>You may continue without the sequencer, but we suggest closing Rosegarden, running \"modprobe snd-seq-midi\" as root, and starting Rosegarden again.  If you wish to run with no sequencer by design, then use \"rosegarden --nosequencer\" to avoid seeing this error in the future.</p>");
    } else if (!(m_soundDriverStatus & VERSION_OK)) {
        text = i18n("<p>The Rosegarden sequencer module version does not match the GUI module version.</p><p>You have probably mixed up files from two different versions of Rosegarden.  Please check your installation.</p>");
    }

    if (text != "") {
        RosegardenGUIApp::self()->awaitDialogClearance();
        QMessageBox::error(RosegardenGUIApp::self(),
                           i18n("<h3>Sequencer startup failed</h3>%1", text));
        CurrentProgressDialog::thaw();
        return;
    }

#ifdef HAVE_LIBJACK
    if (!(m_soundDriverStatus & AUDIO_OK)) {
        RosegardenGUIApp::self()->awaitDialogClearance();
        QMessageBox::information(RosegardenGUIApp::self(), i18n("<h3>Failed to connect to JACK audio server.</h3><p>Rosegarden could not connect to the JACK audio server.  This probably means the JACK server is not running.</p><p>If you want to be able to play or record audio files or use plugins, you should exit Rosegarden and start the JACK server before running Rosegarden again.</p>"),
                                 i18n("Failed to connect to JACK"),
                                 "startup-jack-failed");
    }
#endif
    CurrentProgressDialog::thaw();
}

void
SequenceManager::preparePlayback(bool forceProgramChanges)
{
    Studio &studio = m_doc->getStudio();
    InstrumentList list = studio.getAllInstruments();
    MappedComposition mC;
    MappedEvent *mE;

    std::set<InstrumentId> activeInstruments;

    Composition &composition = m_doc->getComposition();

    for (Composition::trackcontainer::const_iterator i =
             composition.getTracks().begin();
         i != composition.getTracks().end(); ++i) {

        Track *track = i->second;
        if (track) activeInstruments.insert(track->getInstrument());
    }

    // Send the MappedInstruments (minimal Instrument information
    // required for Performance) to the Sequencer
    //
    InstrumentList::iterator it = list.begin();
    for (; it != list.end(); it++) {

        StudioControl::sendMappedInstrument(MappedInstrument(*it));

        // Send program changes for MIDI Instruments
        //
        if ((*it)->getType() == Instrument::Midi) {

            if (activeInstruments.find((*it)->getId()) ==
                activeInstruments.end()) {
//                std::cerr << "SequenceManager::preparePlayback: instrument "
//                          << (*it)->getId() << " is not in use" << std::endl;
                continue;
            }            

            // send bank select always before program change
            //
            if ((*it)->sendsBankSelect()) {
                mE = new MappedEvent((*it)->getId(),
                                     MappedEvent::MidiController,
                                     MIDI_CONTROLLER_BANK_MSB,
                                     (*it)->getMSB());
                mC.insert(mE);

                mE = new MappedEvent((*it)->getId(),
                                     MappedEvent::MidiController,
                                     MIDI_CONTROLLER_BANK_LSB,
                                     (*it)->getLSB());
                mC.insert(mE);
            }

            // send program change
            //
            if ((*it)->sendsProgramChange() || forceProgramChanges) {
                RG_DEBUG << "SequenceManager::preparePlayback() : sending prg change for "
                << (*it)->getPresentationName().c_str() << endl;

                mE = new MappedEvent((*it)->getId(),
                                     MappedEvent::MidiProgramChange,
                                     (*it)->getProgramChange());
                mC.insert(mE);
            }

        } else if ((*it)->getType() == Instrument::Audio ||
                   (*it)->getType() == Instrument::SoftSynth) {
        } else {
            RG_DEBUG << "SequenceManager::preparePlayback - "
            << "unrecognised instrument type" << endl;
        }


    }

    // Send the MappedComposition if it's got anything in it
    showVisuals(mC);
    StudioControl::sendMappedComposition(mC);
}

void
SequenceManager::sendAudioLevel(MappedEvent *mE)
{
    RosegardenGUIView *v;
    QList<RosegardenGUIView>& viewList = m_doc->getViewList();

    for (v = viewList.first(); v != 0; v = viewList.next()) {
        v->showVisuals(mE);
    }

}

void
SequenceManager::resetControllers()
{
    SEQMAN_DEBUG << "SequenceManager::resetControllers - resetting\n";

    // Should do all Midi Instrument - not just guess like this is doing
    // currently.

    InstrumentList list = m_doc->getStudio().getPresentationInstruments();
    InstrumentList::iterator it;

    MappedComposition mC;

    for (it = list.begin(); it != list.end(); it++) {
        if ((*it)->getType() == Instrument::Midi) {
            MappedEvent *mE = new MappedEvent((*it)->getId(),
                                              MappedEvent::MidiController,
                                              MIDI_CONTROLLER_RESET,
                                              0);
            mC.insert(mE);
        }
    }

    StudioControl::sendMappedComposition(mC);
    //showVisuals(mC);
}

void
SequenceManager::resetMidiNetwork()
{
    SEQMAN_DEBUG << "SequenceManager::resetMidiNetwork - resetting\n";
    MappedComposition mC;

    // Should do all Midi Instrument - not just guess like this is doing
    // currently.

    for (unsigned int i = 0; i < 16; i++) {
        MappedEvent *mE =
            new MappedEvent(MidiInstrumentBase + i,
                            MappedEvent::MidiController,
                            MIDI_SYSTEM_RESET,
                            0);

        mC.insert(mE);
    }
    showVisuals(mC);
    StudioControl::sendMappedComposition(mC);
}

void
SequenceManager::sendMIDIRecordingDevice(const QString recordDeviceStr)
{

    if (!recordDeviceStr.isEmpty()) {
        int recordDevice = recordDeviceStr.toInt();

        if (recordDevice >= 0) {
            MappedEvent mE(MidiInstrumentBase,  // InstrumentId
                           MappedEvent::SystemRecordDevice,
                           MidiByte(recordDevice),
                           MidiByte(true));

            StudioControl::sendMappedEvent(mE);
            SEQMAN_DEBUG << "set MIDI record device to "
            << recordDevice << endl;
        }
    }
}

void
SequenceManager::restoreRecordSubscriptions()
{
    QSettings config;
    config.beginGroup( SequencerOptionsConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // config.endGroup();		// corresponding to: config.beginGroup( SequencerOptionsConfigGroup );
    //  

    //QString recordDeviceStr = config.value("midirecorddevice") ;
    QStringList devList = config.value("midirecorddevice").toStringList();

    for ( QStringList::ConstIterator it = devList.begin();
            it != devList.end(); ++it) {
        sendMIDIRecordingDevice(*it);
    }

}

void
SequenceManager::reinitialiseSequencerStudio()
{
    QSettings config;
    config.beginGroup( SequencerOptionsConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // config.endGroup();		// corresponding to: config.beginGroup( SequencerOptionsConfigGroup );
    //  

    //QString recordDeviceStr = config.value("midirecorddevice") ;

    //sendMIDIRecordingDevice(recordDeviceStr);
    restoreRecordSubscriptions();

    // Toggle JACK audio ports appropriately
    //
    bool submasterOuts = qStrToBool( config.value("audiosubmasterouts", "false" ) ) ;
    bool faderOuts = qStrToBool( config.value("audiofaderouts", "false" ) ) ;
    unsigned int audioFileFormat = config.value("audiorecordfileformat", 1).toUInt() ;

    MidiByte ports = 0;
    if (faderOuts) {
        ports |= MappedEvent::FaderOuts;
    }
    if (submasterOuts) {
        ports |= MappedEvent::SubmasterOuts;
    }
    MappedEvent mEports
    (MidiInstrumentBase,
     MappedEvent::SystemAudioPorts,
     ports);

    StudioControl::sendMappedEvent(mEports);

    MappedEvent mEff
    (MidiInstrumentBase,
     MappedEvent::SystemAudioFileFormat,
     audioFileFormat);
    StudioControl::sendMappedEvent(mEff);


    // Set the studio from the current document
    //
    m_doc->initialiseStudio();
}

void
SequenceManager::panic()
{
    SEQMAN_DEBUG << "panic button\n";

    stopping();

    MappedEvent mE(MidiInstrumentBase, MappedEvent::Panic, 0, 0);
    emit setValue(40);
    StudioControl::sendMappedEvent(mE);
    emit setValue(100);

    //    Studio &studio = m_doc->getStudio();
    //
    //    InstrumentList list = studio.getPresentationInstruments();
    //    InstrumentList::iterator it;
    //
    //    int maxDevices = 0, device = 0;
    //    for (it = list.begin(); it != list.end(); it++)
    //        if ((*it)->getType() == Instrument::Midi)
    //            maxDevices++;
    //
    //    emit setValue(40);
    //
    //    for (it = list.begin(); it != list.end(); it++)
    //    {
    //        if ((*it)->getType() == Instrument::Midi)
    //        {
    //            for (unsigned int i = 0; i < 128; i++)
    //            {
    //                MappedEvent
    //                    mE((*it)->getId(),
    //                                MappedEvent::MidiNote,
    //                                i,
    //                                0);
    //
    //                StudioControl::sendMappedEvent(mE);
    //            }
    //
    //            device++;
    //        }
    //
    //        emit setValue(int(90.0 * (double(device) / double(maxDevices))));
    //    }
    //
    //    resetControllers();
}

void
SequenceManager::showVisuals(const MappedComposition &mC)
{
    MappedComposition::const_iterator it = mC.begin();
    if (it != mC.end())
        m_transport->setMidiOutLabel(*it);
}

MappedComposition
SequenceManager::applyFiltering(const MappedComposition &mC,
                                MappedEvent::MappedEventType filter)
{
    MappedComposition retMc;
    MappedComposition::const_iterator it = mC.begin();

    for (; it != mC.end(); it++) {
        if (!((*it)->getType() & filter))
            retMc.insert(new MappedEvent(*it));
    }

    return retMc;
}

void SequenceManager::resetCompositionMmapper()
{
    SEQMAN_DEBUG << "SequenceManager::resetCompositionMmapper()\n";
    delete m_compositionMmapper;
    m_compositionMmapper = new CompositionMmapper(m_doc);

    resetMetronomeMmapper();
    resetTempoSegmentMmapper();
    resetTimeSigSegmentMmapper();
    resetControlBlockMmapper();
}

void SequenceManager::resetMetronomeMmapper()
{
    SEQMAN_DEBUG << "SequenceManager::resetMetronomeMmapper()\n";

    delete m_metronomeMmapper;
    m_metronomeMmapper = SegmentMmapperFactory::makeMetronome(m_doc);
}

void SequenceManager::resetTempoSegmentMmapper()
{
    SEQMAN_DEBUG << "SequenceManager::resetTempoSegmentMmapper()\n";

    delete m_tempoSegmentMmapper;
    m_tempoSegmentMmapper = SegmentMmapperFactory::makeTempo(m_doc);
}

void SequenceManager::resetTimeSigSegmentMmapper()
{
    SEQMAN_DEBUG << "SequenceManager::resetTimeSigSegmentMmapper()\n";

    delete m_timeSigSegmentMmapper;
    m_timeSigSegmentMmapper = SegmentMmapperFactory::makeTimeSig(m_doc);
}

void SequenceManager::resetControlBlockMmapper()
{
    SEQMAN_DEBUG << "SequenceManager::resetControlBlockMmapper()\n";

    m_controlBlockMmapper->setDocument(m_doc);
}

bool SequenceManager::event(QEvent *e)
{
    if (e->type() == QEvent::User) {
        SEQMAN_DEBUG << "SequenceManager::event() with user event\n";
        if (m_updateRequested) {
            SEQMAN_DEBUG << "SequenceManager::event(): update requested\n";
            checkRefreshStatus();
            m_updateRequested = false;
        }
        return true;
    } else {
        return QObject::event(e);
    }
}

void SequenceManager::update()
{
    SEQMAN_DEBUG << "SequenceManager::update()\n";
    // schedule a refresh-status check for the next event loop
    QEvent *e = new QEvent(QEvent::User);
    m_updateRequested = true;
    QApplication::postEvent(this, e);
}

void SequenceManager::checkRefreshStatus()
{
    SEQMAN_DEBUG << "SequenceManager::checkRefreshStatus()\n";

    // Look at trigger segments first: if one of those has changed, we'll
    // need to be aware of it when scanning segments subsequently

    TriggerSegmentRec::SegmentRuntimeIdSet ridset;
    Composition &comp = m_doc->getComposition();
    SegmentRefreshMap newTriggerMap;

    for (Composition::triggersegmentcontaineriterator i =
                comp.getTriggerSegments().begin();
            i != comp.getTriggerSegments().end(); ++i) {

        Segment *s = (*i)->getSegment();

        if (m_triggerSegments.find(s) == m_triggerSegments.end()) {
            newTriggerMap[s] = s->getNewRefreshStatusId();
        } else {
            newTriggerMap[s] = m_triggerSegments[s];
        }

        if (s->getRefreshStatus(newTriggerMap[s]).needsRefresh()) {
            TriggerSegmentRec::SegmentRuntimeIdSet &thisSet = (*i)->getReferences();
            ridset.insert(thisSet.begin(), thisSet.end());
            s->getRefreshStatus(newTriggerMap[s]).setNeedsRefresh(false);
        }
    }

    m_triggerSegments = newTriggerMap;

    SEQMAN_DEBUG << "SequenceManager::checkRefreshStatus: segments modified by changes to trigger segments are:" << endl;
    int x = 0;
    for (TriggerSegmentRec::SegmentRuntimeIdSet::iterator i = ridset.begin();
            i != ridset.end(); ++i) {
        SEQMAN_DEBUG << x << ": " << *i << endl;
        ++x;
    }

    std::vector<Segment*>::iterator i;

    // Check removed segments first
    for (i = m_removedSegments.begin(); i != m_removedSegments.end(); ++i) {
        processRemovedSegment(*i);
    }
    m_removedSegments.clear();

    SEQMAN_DEBUG << "SequenceManager::checkRefreshStatus: we have "
    << m_segments.size() << " segments" << endl;

    // then the ones which are still there
    for (SegmentRefreshMap::iterator i = m_segments.begin();
            i != m_segments.end(); ++i) {
        if (i->first->getRefreshStatus(i->second).needsRefresh() ||
                ridset.find(i->first->getRuntimeId()) != ridset.end()) {
            segmentModified(i->first);
            i->first->getRefreshStatus(i->second).setNeedsRefresh(false);
        }
    }

    // then added ones
    for (i = m_addedSegments.begin(); i != m_addedSegments.end(); ++i) {
        processAddedSegment(*i);
    }
    m_addedSegments.clear();
}

void SequenceManager::segmentModified(Segment* s)
{
    SEQMAN_DEBUG << "SequenceManager::segmentModified(" << s << ")\n";

    bool sizeChanged = m_compositionMmapper->segmentModified(s);

    SEQMAN_DEBUG << "SequenceManager::segmentModified() : size changed = "
    << sizeChanged << endl;

    if ((m_transportStatus == PLAYING) && sizeChanged) {
        RosegardenSequencer::getInstance()->
            remapSegment((QString)m_compositionMmapper->getSegmentFileName(s),
                         (size_t)m_compositionMmapper->getSegmentFileSize(s));
    }
}

void SequenceManager::segmentAdded(const Composition*, Segment* s)
{
    SEQMAN_DEBUG << "SequenceManager::segmentAdded(" << s << ")\n";
    m_addedSegments.push_back(s);
}

void SequenceManager::segmentRemoved(const Composition*, Segment* s)
{
    SEQMAN_DEBUG << "SequenceManager::segmentRemoved(" << s << ")\n";
    m_removedSegments.push_back(s);
    std::vector<Segment*>::iterator i = find(m_addedSegments.begin(), m_addedSegments.end(), s);
    if (i != m_addedSegments.end())
        m_addedSegments.erase(i);
}

void SequenceManager::segmentRepeatChanged(const Composition*, Segment* s, bool repeat)
{
    SEQMAN_DEBUG << "SequenceManager::segmentRepeatChanged(" << s << ", " << repeat << ")\n";
    segmentModified(s);
}

void SequenceManager::segmentRepeatEndChanged(const Composition*, Segment* s, timeT newEndTime)
{
    SEQMAN_DEBUG << "SequenceManager::segmentRepeatEndChanged(" << s << ", " << newEndTime << ")\n";
    segmentModified(s);
}

void SequenceManager::segmentEventsTimingChanged(const Composition*, Segment * s, timeT t, RealTime)
{
    SEQMAN_DEBUG << "SequenceManager::segmentEventsTimingChanged(" << s << ", " << t << ")\n";
    segmentModified(s);
    if (s && s->getType() == Segment::Audio && m_transportStatus == PLAYING) {
        RosegardenSequencer::getInstance()->remapTracks();
    }
}

void SequenceManager::segmentTransposeChanged(const Composition*, Segment *s, int transpose)
{
    SEQMAN_DEBUG << "SequenceManager::segmentTransposeChanged(" << s << ", " << transpose << ")\n";
    segmentModified(s);
}

void SequenceManager::segmentTrackChanged(const Composition*, Segment *s, TrackId id)
{
    SEQMAN_DEBUG << "SequenceManager::segmentTrackChanged(" << s << ", " << id << ")\n";
    segmentModified(s);
    if (s && s->getType() == Segment::Audio && m_transportStatus == PLAYING) {
        RosegardenSequencer::getInstance()->remapTracks();
    }
}

void SequenceManager::segmentEndMarkerChanged(const Composition*, Segment *s, bool)
{
    SEQMAN_DEBUG << "SequenceManager::segmentEndMarkerChanged(" << s << ")\n";
    segmentModified(s);
}

void SequenceManager::processAddedSegment(Segment* s)
{
    SEQMAN_DEBUG << "SequenceManager::processAddedSegment(" << s << ")\n";

    m_compositionMmapper->segmentAdded(s);

    if (m_transportStatus == PLAYING) {
        RosegardenSequencer::getInstance()->addSegment
            (m_compositionMmapper->getSegmentFileName(s));
    }

    // Add to segments map
    int id = s->getNewRefreshStatusId();
    m_segments.insert(SegmentRefreshMap::value_type(s, id));

}

void SequenceManager::processRemovedSegment(Segment* s)
{
    SEQMAN_DEBUG << "SequenceManager::processRemovedSegment(" << s << ")\n";

    QString filename = m_compositionMmapper->getSegmentFileName(s);
    m_compositionMmapper->segmentDeleted(s);

    if (m_transportStatus == PLAYING) {
        RosegardenSequencer::getInstance()->deleteSegment(filename);
    }

    // Remove from segments map
    m_segments.erase(s);
}

void SequenceManager::endMarkerTimeChanged(const Composition *, bool /*shorten*/)
{
    SEQMAN_DEBUG << "SequenceManager::endMarkerTimeChanged()\n";
    m_compositionMmapperResetTimer->start(500, true); // schedule a composition mmapper reset in 0.5s
}

void SequenceManager::timeSignatureChanged(const Composition *)
{
    resetMetronomeMmapper();
}

void SequenceManager::trackChanged(const Composition *, Track* t)
{
    SEQMAN_DEBUG << "SequenceManager::trackChanged(" << t << ", " << (t ? t->getPosition() : -1) << ")\n";
    m_controlBlockMmapper->updateTrackData(t);

    if (m_transportStatus == PLAYING) {
        RosegardenSequencer::getInstance()->remapTracks();
    }
}

void SequenceManager::trackDeleted(const Composition *, TrackId t)
{
    m_controlBlockMmapper->setTrackDeleted(t);
}

void SequenceManager::metronomeChanged(InstrumentId id,
                                       bool regenerateTicks)
{
    // This method is called when the user has changed the
    // metronome instrument, pitch etc

    SEQMAN_DEBUG << "SequenceManager::metronomeChanged (simple)"
                 << ", instrument = "
                 << id
                 << endl;

    if (regenerateTicks)
        resetMetronomeMmapper();

    m_controlBlockMmapper->updateMetronomeData(id);
    if (m_transportStatus == PLAYING) {
        m_controlBlockMmapper->updateMetronomeForPlayback();
    } else {
        m_controlBlockMmapper->updateMetronomeForRecord();
    }

    m_metronomeMmapper->refresh();
    m_timeSigSegmentMmapper->refresh();
    m_tempoSegmentMmapper->refresh();
}

void SequenceManager::metronomeChanged(const Composition *)
{
    // This method is called when the muting status in the composition
    // has changed -- the metronome itself has not actually changed

    SEQMAN_DEBUG << "SequenceManager::metronomeChanged "
                 << ", instrument = "
                 << m_metronomeMmapper->getMetronomeInstrument()
                 << endl;

    m_controlBlockMmapper->updateMetronomeData
    (m_metronomeMmapper->getMetronomeInstrument());

    if (m_transportStatus == PLAYING) {
        m_controlBlockMmapper->updateMetronomeForPlayback();
    } else {
        m_controlBlockMmapper->updateMetronomeForRecord();
    }
}

void SequenceManager::filtersChanged(MidiFilter thruFilter,
                                     MidiFilter recordFilter)
{
    m_controlBlockMmapper->updateMidiFilters(thruFilter, recordFilter);
}

void SequenceManager::soloChanged(const Composition *, bool solo, TrackId selectedTrack)
{
    if (m_controlBlockMmapper->updateSoloData(solo, selectedTrack)) {
        if (m_transportStatus == PLAYING) {
            RosegardenSequencer::getInstance()->remapTracks();
        }
    }
}

void SequenceManager::tempoChanged(const Composition *c)
{
    SEQMAN_DEBUG << "SequenceManager::tempoChanged()\n";

    // Refresh all segments
    //
    for (SegmentRefreshMap::iterator i = m_segments.begin();
            i != m_segments.end(); ++i) {
        segmentModified(i->first);
    }

    // and metronome, time sig and tempo
    //
    m_metronomeMmapper->refresh();
    m_timeSigSegmentMmapper->refresh();
    m_tempoSegmentMmapper->refresh();

    if (c->isLooping())
        setLoop(c->getLoopStart(), c->getLoopEnd());
    else if (m_transportStatus == PLAYING) {
        // If the tempo changes during playback, reset the pointer
        // position because the sequencer keeps track of position in
        // real time and we want to maintain the same position in
        // musical time.  Turn off play tracking while this happens,
        // so that we don't jump about in the main window while the
        // user's trying to drag the tempo in it.  (That doesn't help
        // for matrix or notation though, sadly)
        bool tracking = RosegardenGUIApp::self()->isTrackEditorPlayTracking();
        if (tracking)
            RosegardenGUIApp::self()->slotToggleTracking();
        m_doc->slotSetPointerPosition(c->getPosition());
        if (tracking)
            RosegardenGUIApp::self()->slotToggleTracking();
    }
}

void
SequenceManager::sendTransportControlStatuses()
{
    QSettings config;
    config.beginGroup( SequencerOptionsConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // config.endGroup();		// corresponding to: config.beginGroup( SequencerOptionsConfigGroup );
    //  


    // Get the config values
    //
    bool jackTransport = qStrToBool( config.value("jacktransport", "false" ) ) ;
    bool jackMaster = qStrToBool( config.value("jackmaster", "false" ) ) ;

    int mmcMode = config.value("mmcmode", 0).toInt() ;
    int mtcMode = config.value("mtcmode", 0).toInt() ;

    int midiClock = config.value("midiclock", 0).toInt() ;
    bool midiSyncAuto = qStrToBool( config.value("midisyncautoconnect", "false" ) ) ;

    // Send JACK transport
    //
    int jackValue = 0;
    if (jackTransport && jackMaster)
        jackValue = 2;
    else {
        if (jackTransport)
            jackValue = 1;
        else
            jackValue = 0;
    }

    MappedEvent mEjackValue(MidiInstrumentBase,  // InstrumentId
                            MappedEvent::SystemJackTransport,
                            MidiByte(jackValue));
    StudioControl::sendMappedEvent(mEjackValue);


    // Send MMC transport
    //
    MappedEvent mEmmcValue(MidiInstrumentBase,  // InstrumentId
                           MappedEvent::SystemMMCTransport,
                           MidiByte(mmcMode));

    StudioControl::sendMappedEvent(mEmmcValue);


    // Send MTC transport
    //
    MappedEvent mEmtcValue(MidiInstrumentBase,  // InstrumentId
                           MappedEvent::SystemMTCTransport,
                           MidiByte(mtcMode));

    StudioControl::sendMappedEvent(mEmtcValue);


    // Send MIDI Clock
    //
    MappedEvent mEmidiClock(MidiInstrumentBase,  // InstrumentId
                            MappedEvent::SystemMIDIClock,
                            MidiByte(midiClock));

    StudioControl::sendMappedEvent(mEmidiClock);


    // Send MIDI Sync Auto-Connect
    //
    MappedEvent mEmidiSyncAuto(MidiInstrumentBase,  // InstrumentId
                               MappedEvent::SystemMIDISyncAuto,
                               MidiByte(midiSyncAuto ? 1 : 0));

    StudioControl::sendMappedEvent(mEmidiSyncAuto);

}

void
SequenceManager::slotCountdownTimerTimeout()
{
    // Set the elapsed time in seconds
    //
    m_countdownDialog->setElapsedTime(m_recordTime->elapsed() / 1000);
}

void
SequenceManager::slotFoundMountPoint(const QString&,
                                     unsigned long kBSize,
                                     unsigned long /*kBUsed*/,
                                     unsigned long kBAvail)
{
    m_gotDiskSpaceResult = true;
    m_diskSpaceKBAvail = kBAvail;
}

void
SequenceManager::enableMIDIThruRouting(bool state)
{
    m_controlBlockMmapper->enableMIDIThruRouting(state);
}

int
SequenceManager::getSampleRate() 
{
    if (m_sampleRate != 0) return m_sampleRate;

    m_sampleRate = RosegardenSequencer::getInstance()->getSampleRate();

    return m_sampleRate;
}

bool
SequenceManager::shouldWarnForImpreciseTimer()
{
    QSettings confq4;
    confq4.beginGroup( SequencerOptionsConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // confq4.endGroup();		// corresponding to: confq4.beginGroup( SequencerOptionsConfigGroup );
    //  

    QString timer = confq4.value("timer").toString();
    if (timer == "(auto)" || timer == "") return true;
    else return false; // if the user has chosen the timer, leave them alone
}

}
#include "SequenceManager.moc"
