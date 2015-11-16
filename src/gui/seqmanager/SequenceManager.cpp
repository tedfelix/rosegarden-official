/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

// #define DEBUG_SEQUENCE_MANAGER 1
#if !defined DEBUG_SEQUENCE_MANAGER
#define RG_NO_DEBUG_PRINT 1
#endif

#include "SequenceManager.h"

#include "sound/Midi.h"
#include "sound/ControlBlock.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "misc/ConfigGroups.h"
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
#include "CompositionMapper.h"
#include "document/RosegardenDocument.h"
#include "document/CommandHistory.h"
#include "gui/application/RosegardenApplication.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/application/RosegardenMainViewWidget.h"
#include "gui/dialogs/AudioManagerDialog.h"
#include "gui/dialogs/CountdownDialog.h"
#include "gui/dialogs/TransportDialog.h"
#include "gui/widgets/StartupLogo.h"
#include "gui/studio/StudioControl.h"
#include "gui/dialogs/DialogSuppressor.h"
#include "gui/widgets/CurrentProgressDialog.h"
#include "gui/widgets/WarningWidget.h"
#include "sequencer/RosegardenSequencer.h"
#include "MarkerMapper.h"
#include "MetronomeMapper.h"
#include "TempoSegmentMapper.h"
#include "TimeSigSegmentMapper.h"
#include "sound/AudioFile.h"
#include "sound/MappedEventList.h"
#include "sound/MappedEvent.h"
#include "sound/MappedInstrument.h"
#include "sound/SoundDriver.h"

#include "rosegarden-version.h"

#include <QSettings>
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

SequenceManager::SequenceManager(TransportDialog *transport) :
    m_doc(0),
    m_compositionMapper(0),
    m_metronomeMapper(0),
    m_tempoSegmentMapper(0),
    m_timeSigSegmentMapper(0),
    m_transportStatus(STOPPED),
    m_soundDriverStatus(NO_DRIVER),
    m_transport(transport),
    m_lastRewoundAt(clock()),
    m_countdownDialog(0),
    m_countdownTimer(0),
    m_shownOverrunWarning(false),
    m_recordTime(new QTime()),
    m_updateRequested(true),
    //m_compositionMapperResetTimer(0),
    m_reportTimer(0),
    m_canReport(true),
    m_lastLowLatencySwitchSent(false),
    m_lastTransportStartPosition(0),
    m_sampleRate(0)
{
    // The owner of this sequence manager will need to call
    // checkSoundDriverStatus on it to set up its status appropriately
    // immediately after construction; we used to do it from here but
    // we're not well placed to handle reporting to the user if it
    // throws an exception (and we don't want to leave the object half
    // constructed).
}

SequenceManager::~SequenceManager()
{
    if (m_doc) {
        m_doc->getComposition().removeObserver(this);
    }

    SEQMAN_DEBUG << "SequenceManager::~SequenceManager()" << endl;
    delete m_compositionMapper;
    m_metronomeMapper->removeOwner();
    m_tempoSegmentMapper->removeOwner();
    m_timeSigSegmentMapper->removeOwner();
}

void
SequenceManager::setDocument(RosegardenDocument *doc)
{
    SEQMAN_DEBUG << "SequenceManager::setDocument(" << doc << ")" << endl;

    DataBlockRepository::clear();

    if (m_doc) m_doc->getComposition().removeObserver(this);
    disconnect(CommandHistory::getInstance(), SIGNAL(commandExecuted()));

    m_segments.clear();
    m_triggerSegments.clear();

    m_doc = doc;
    Composition &comp = m_doc->getComposition();

    m_doc->setSequenceManager(this);

    // Must recreate and reconnect the countdown timer and dialog
    // (bug 729039)
    //
    delete m_countdownDialog;
    delete m_countdownTimer;
    //delete m_compositionMapperResetTimer;

    m_countdownDialog = new CountdownDialog(dynamic_cast<QWidget*>
                                            (m_doc->parent())->parentWidget());

    // Bug 933041: no longer connect the CountdownDialog from
    // SequenceManager; instead let the RosegardenMainWindow connect it to
    // its own slotStop to ensure the right housekeeping is done

    m_countdownTimer = new QTimer(m_doc);
    connect(m_countdownTimer, SIGNAL(timeout()),
            this, SLOT(slotCountdownTimerTimeout()));

    //m_compositionMapperResetTimer = new QTimer(m_doc);
    //connect(m_compositionMapperResetTimer, SIGNAL(timeout()),
    //        this, SLOT(slotScheduledCompositionMapperReset()));

    m_compositionRefreshStatusId = comp.getNewRefreshStatusId();
    comp.addObserver(this);

    connect(CommandHistory::getInstance(), SIGNAL(commandExecuted()),
            this, SLOT(update()));

    resetCompositionMapper();
    
    populateCompositionMapper();
}

void
SequenceManager::setTransportStatus(const TransportStatus &status)
{
    m_transportStatus = status;
}

bool
SequenceManager::play()
{
    if (!m_doc) return false;

    Composition &comp = m_doc->getComposition();

    // If already playing or recording then stop
    //
    if (m_transportStatus == PLAYING ||
        m_transportStatus == RECORDING) {
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
    ControlBlock::getInstance()->setInstrumentForMetronome
        (m_metronomeMapper->getMetronomeInstrument());
    ControlBlock::getInstance()->setMetronomeMuted(!comp.usePlayMetronome());

    // make sure we toggle the play button
    //
    m_transport->PlayButton()->setChecked(true);

    //!!! disable the record button, because recording while playing is horribly
    // broken, and disabling it is less complicated than fixing it
    // see #1223025 - DMM
    //    SEQMAN_DEBUG << "SequenceManager::play() - disabling record button, as we are playing\n";
    //    m_transport->RecordButton()->setEnabled(false);

    if (comp.getCurrentTempo() == 0) {
        comp.setCompositionDefaultTempo(comp.getTempoForQpm(120.0));

        SEQMAN_DEBUG << "SequenceManager::play() - setting Tempo to Default value of 120.000" << endl;
    } else {
        SEQMAN_DEBUG << "SequenceManager::play() - starting to play" << endl;
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

    QSettings settings;
    settings.beginGroup( SequencerOptionsConfigGroup );

    bool lowLat = qStrToBool( settings.value("audiolowlatencymonitoring", "true" ) ) ;

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
    settings.endGroup();

    return false;
}

void
SequenceManager::stopping()
{
    if (!m_doc) return;

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
        m_transport->RecordButton()->setChecked(false);
        m_transport->MetronomeButton()->
          setChecked(m_doc->getComposition().usePlayMetronome());
        return ;
    }

    SEQMAN_DEBUG << "SequenceManager::stopping() - preparing to stop" << endl;
    //    SEQMAN_DEBUG << kdBacktrace() << endl;

    stop();

    m_shownOverrunWarning = false;
}

void
SequenceManager::stop()
{
    if (!m_doc) return;

    // Toggle off the buttons - first record
    //
    if (m_transportStatus == RECORDING) {
        m_transport->RecordButton()->setChecked(false);
        m_transport->MetronomeButton()->
          setChecked(m_doc->getComposition().usePlayMetronome());

        // Remove the countdown dialog and stop the timer
        //
        m_countdownDialog->hide();
        m_countdownTimer->stop();
    }

    // Now playback
    m_transport->PlayButton()->setChecked(false);

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
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

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

        SEQMAN_DEBUG << "SequenceManager::stop() - stopped recording" << endl;
    } else {
        m_doc->stopPlaying();
    }

    // always untoggle the play button at this stage
    //
    m_transport->PlayButton()->setChecked(false);
    SEQMAN_DEBUG << "SequenceManager::stop() - stopped playing" << endl;
    // We don't reset controllers at this point - what happens with static
    // controllers the next time we play otherwise?  [rwb]
    //resetControllers();
}

void
SequenceManager::rewind()
{
    if (!m_doc) return;

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
    if (!m_doc) return;

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
    if (!m_doc) return;

    SEQMAN_DEBUG << "SequenceManager::record(" << toggled << ")" << endl;
    Composition &comp = m_doc->getComposition();
    Studio &studio = m_doc->getStudio();

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
                m_transport->RecordButton()->setChecked(false);
                throw(Exception(QObject::tr("Audio subsystem is not available - can't record audio")));
            }
            // throws BadAudioPathException if path is not valid:
            m_doc->getAudioFileManager().testAudioPath();
            break;
        }
    }

    if (toggled) { // preparing record or punch-in record

        if (m_transportStatus == RECORDING_ARMED) {
            SEQMAN_DEBUG << "SequenceManager::record - unarming record" << endl;
            m_transportStatus = STOPPED;

            // Toggle the buttons
            m_transport->MetronomeButton()->setChecked(comp.usePlayMetronome());
            m_transport->RecordButton()->setChecked(false);

            return ;
        }

        if (m_transportStatus == STOPPED) {
            SEQMAN_DEBUG << "SequenceManager::record - armed record" << endl;
            m_transportStatus = RECORDING_ARMED;

            // Toggle the buttons
            m_transport->MetronomeButton()->setChecked(comp.useRecordMetronome());
            m_transport->RecordButton()->setChecked(true);

            return ;
        }

        if (m_transportStatus == RECORDING) {
            SEQMAN_DEBUG << "SequenceManager::record - stop recording and keep playing" << endl;
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
            SEQMAN_DEBUG << "SequenceManager::record - punch in recording" << endl;
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
            // TRANSLATORS: the pixmap in this error string contains no English
            // text and is suitable for use by all languages
            throw(Exception(QObject::tr("<qt><p>No tracks were armed for recording.</p><p>Please arm at least one of the recording LEDs <img src=\":pixmaps/tooltip/record-leds.png\"> and try again</p></qt>")));
        }

        // may throw an exception
        checkSoundDriverStatus(false);

        // toggle the Metronome button if it's in use
        m_transport->MetronomeButton()->setChecked(comp.useRecordMetronome());

        // Update record metronome status
        //
        ControlBlock::getInstance()->setInstrumentForMetronome
            (m_metronomeMapper->getMetronomeInstrument());
        ControlBlock::getInstance()->setMetronomeMuted(!comp.useRecordMetronome());
		
        QSettings settings;
        settings.beginGroup(GeneralOptionsConfigGroup);
		
        // If we are looping then jump to start of loop and start recording,
        // if we're not take off the number of count-in bars and start
        // recording.
        //
        if (comp.isLooping())
            m_doc->slotSetPointerPosition(comp.getLoopStart());
        else {
            if (m_transportStatus != RECORDING_ARMED && punchIn == false) {
                int startBar = comp.getBarNumber(comp.getPosition());
                startBar -= settings.value("countinbars", 0).toUInt();
                m_doc->slotSetPointerPosition(comp.getBarRange(startBar).first);
            }
        }

        settings.endGroup();

        m_doc->setRecordStartTime(m_doc->getComposition().getPosition());

        if (haveAudioInstrument) {
            // Ask the document to update its record latencies so as to
            // do latency compensation when we stop
            m_doc->updateAudioRecordLatency();
        }

        if (haveMIDIInstrument) {
            // For each recording segment...
            for (Composition::recordtrackcontainer::const_iterator i =
                        comp.getRecordTracks().begin(); i != comp.getRecordTracks().end(); ++i) {
                // Get the Instrument for this Track
                InstrumentId iid = comp.getTrackById(*i)->getInstrument();
                Instrument *inst = studio.getInstrumentById(iid);

                // If this is a MIDI instrument
                if (inst && (inst->getType() != Instrument::Audio)) {
                    SEQMAN_DEBUG << "SequenceManager:  mdoc->addRecordMIDISegment(" << *i << ")" << endl;
                    // Create the record MIDI segment now, so that the
                    // composition view has a real segment to display.  It
                    // won't actually be added to the composition until the
                    // first recorded event arrives.  We don't have to do this
                    // from here for audio, because for audio the sequencer
                    // calls back on createRecordAudioFiles so as to find out
                    // what files it needs to write to.
                    m_doc->addRecordMIDISegment(*i);

                    if (inst->getType() == Instrument::Midi) {
                        // Get the channel based on the instrument number.  E.g.
                        // MIDI Instrument #1 is channel 0.
                        int channel = inst->getNaturalChannel();

                        // Send Program Changes on recording tracks like 11.11.42
                        // used to.  Fix for bug #1356 "Wrong instrument when
                        // recording on multiple tracks/channels".  This is a
                        // simple way to support multiple MIDI controllers.
                        StudioControl::sendChannelSetup(inst, channel);
                    }
                }
            }
        }

        // set the buttons
        m_transport->RecordButton()->setChecked(true);
        m_transport->PlayButton()->setChecked(true);

        if (comp.getCurrentTempo() == 0) {
            SEQMAN_DEBUG << "SequenceManager::play() - setting Tempo to Default value of 120.000" << endl;
            comp.setCompositionDefaultTempo(comp.getTempoForQpm(120.0));
        } else {
            SEQMAN_DEBUG << "SequenceManager::record() - starting to record" << endl;
        }

        // set the tempo in the transport
        //
        m_transport->setTempo(comp.getCurrentTempo());

        // The arguments for the Sequencer - record is similar to playback,
        // we must being playing to record.
        //
        RealTime startPos =
            comp.getElapsedRealTime(comp.getPosition());

//         QSettings settings;
        settings.beginGroup( GeneralOptionsConfigGroup );

        bool lowLat = qStrToBool( settings.value("audiolowlatencymonitoring", "true" ) ) ;
        settings.endGroup();

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
                throw(Exception(QObject::tr("<qt><p>Couldn't start recording audio.</p><p>Please set a valid recording path in <b>Composition -> Edit Document Properties... -> Audio</b></p></qt>")));
            }
        }
    }
}

void
SequenceManager::processAsynchronousMidi(const MappedEventList &mC,
                                         AudioManagerDialog *audioManagerDialog)
{
#ifdef REPORT_XRUNS
    static bool boolShowingWarning = false;
#endif
    static bool boolShowingALSAWarning = false;
    static long warningShownAt = 0;

    if (m_doc == 0 || mC.size() == 0)
        return ;

    MappedEventList::const_iterator i;

    // before applying through-filter, catch program-change-messages
    int prg;
    int bnk_msb;
    int bnk_lsb;
    bnk_msb = -1;
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
    MappedEventList tempMC =
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
                SEQMAN_DEBUG << "Received AudioGeneratePreview: data1 is " 
                             << int((*i)->getData1()) << ", data2 "
                             << int((*i)->getData2()) << ", instrument is " 
                             << (*i)->getInstrument() << endl;
                m_doc->finalizeAudioFile((int)(*i)->getData1() +
                                         (int)(*i)->getData2() * 256);
            }
            
            if ((*i)->getType() == MappedEvent::SystemUpdateInstruments) {

                // resync Devices and Instruments
                //
//!DEVPUSH                m_doc->syncDevices();
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

                        QMessageBox::critical(
                            dynamic_cast<QWidget*>(m_doc->parent())->parentWidget(), "",
                            tr("The JACK Audio subsystem has failed or it has stopped Rosegarden from processing audio.\nPlease restart Rosegarden to continue working with audio.\nQuitting other running applications may improve Rosegarden's performance."));

                    } else if ((*i)->getData1() == MappedEvent::FailureJackRestart) {

                        QMessageBox::critical(
                            dynamic_cast<QWidget*>(m_doc->parent())->parentWidget(), "",
                            tr("The JACK Audio subsystem has stopped Rosegarden from processing audio, probably because of a processing overload.\nAn attempt to restart the audio service has been made, but some problems may remain.\nQuitting other running applications may improve Rosegarden's performance."));

                    } else if ((*i)->getData1() == MappedEvent::FailureCPUOverload) {

#define REPORT_CPU_OVERLOAD 1
#ifdef REPORT_CPU_OVERLOAD

                        stopping();

                        QMessageBox::critical(
                            dynamic_cast<QWidget*>(m_doc->parent())->parentWidget(), "",
                            tr("Out of processor power for real-time audio processing.  Cannot continue."));

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

                            QString message = tr("A serious error has occurred in the ALSA MIDI subsystem.  It may not be possible to continue sequencing.  Please check console output for more information.");
                            boolShowingALSAWarning = true;

                            QMessageBox::information(
                              0,
                              "", /* no title */
                              message,
                              QMessageBox::Ok,
                              QMessageBox::Ok);
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

                            QString message = tr("JACK Audio subsystem is losing sample frames.");
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
                            message = tr("Failed to read audio data from disk in time to service the audio subsystem.");
                            break;

                        case MappedEvent::FailureDiscOverrun:
                            message = tr("Failed to write audio data to disk fast enough to service the audio subsystem.");
                            break;

                        case MappedEvent::FailureBussMixUnderrun:
                            message = tr("The audio mixing subsystem is failing to keep up.");
                            break;

                        case MappedEvent::FailureMixUnderrun:
                            message = tr("The audio subsystem is failing to keep up.");
                            break;

                        default:
                            message = tr("Unknown sequencer failure mode!");
                            break;
                        }

                        m_shownOverrunWarning = true;

#ifdef REPORT_XRUNS

                        QMessageBox::information(0, message);
#else

                        if ((*i)->getData1() == MappedEvent::FailureDiscOverrun) {
                            // the error you can't hear
                            QMessageBox::information(
                              0,
                              "", /* no title */
                              message,
                              QMessageBox::Ok,
                              QMessageBox::Ok);
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
                        m_reportTimer->setSingleShot(true);
                        m_reportTimer->start(5000);
                    }
                }
            } else {
//                StartupLogo::hideIfStillThere();

                if ((*i)->getType() == MappedEvent::SystemFailure) {

                    QString text(tr("<h3>System timer resolution is too low!</h3>"));
                    QString informativeText("");

                    if ((*i)->getData1() == MappedEvent::FailureJackRestartFailed) {

                        QMessageBox::critical(
                            RosegardenMainWindow::self(), "",
                            tr("The JACK Audio subsystem has failed or it has stopped Rosegarden from processing audio.\nPlease restart Rosegarden to continue working with audio.\nQuitting other running applications may improve Rosegarden's performance."));

                    } else if ((*i)->getData1() == MappedEvent::FailureJackRestart) {
                        //!!! Does this attempt to restart the "audio service"
                        // EVER work?  I don't think I've ever seen that work.
                        // When this is gone, it's gone, and time to restart.
                        // But let's not change the translated message.
                        QMessageBox::critical(
                            RosegardenMainWindow::self(), "",
                            tr("The JACK Audio subsystem has stopped Rosegarden from processing audio, probably because of a processing overload.\nAn attempt to restart the audio service has been made, but some problems may remain.\nQuitting other running applications may improve Rosegarden's performance."));

                    } else if ((*i)->getData1() == MappedEvent::WarningImpreciseTimer &&
                               shouldWarnForImpreciseTimer()) {

                        std::cerr << "Rosegarden: WARNING: No accurate sequencer timer available" << std::endl;

//                        StartupLogo::hideIfStillThere();
//                        CurrentProgressDialog::freeze();
//
//                        RosegardenMainWindow::self()->awaitDialogClearance();

                        // This is to avoid us ever showing the same
                        // dialog more than once during a single run
                        // of the program -- it's quite separate from
                        // the suppression function
                        static bool showTimerWarning = true;

                        if (showTimerWarning) {
                            informativeText =tr("<p>Rosegarden was unable to find a high-resolution timing source for MIDI performance.</p><p>This may mean you are using a Linux system with the kernel timer resolution set too low.  Please contact your Linux distributor for more information.</p><p>Some Linux distributors already provide low latency kernels, see the <a style=\"color:gold\" href=\"http://www.rosegardenmusic.com/wiki/low-latency_kernels\">Rosegarden website</a> for instructions.</p>");

                            // whatever, don't show again during this run
                            showTimerWarning = false;
                        }
                        
                        CurrentProgressDialog::thaw();

                    } else if ((*i)->getData1() == MappedEvent::WarningImpreciseTimerTryRTC &&
                               shouldWarnForImpreciseTimer()) {

                        std::cerr << "Rosegarden: WARNING: No accurate sequencer timer available (and kernel is new enough for RTC addendum)" << std::endl;

//                        StartupLogo::hideIfStillThere();
//                        CurrentProgressDialog::freeze();
//
//                        RosegardenMainWindow::self()->awaitDialogClearance();
                        
                        // This is to avoid us ever showing the same
                        // dialog more than once during a single run
                        // of the program -- it's quite separate from
                        // the suppression function
                        static bool showAltTimerWarning = true;

                        if (showAltTimerWarning) {
                            informativeText = tr("<p>Rosegarden was unable to find a high-resolution timing source for MIDI performance.</p><p>You may be able to solve this problem by loading the RTC timer kernel module.  To do this, try running <b>sudo modprobe snd-rtctimer</b> in a terminal window and then restarting Rosegarden.</p><p>Alternatively, check whether your Linux distributor provides a multimedia-optimized kernel.  See the <a style=\"color:gold\"  href=\"http://www.rosegardenmusic.com/wiki/low-latency_kernels\">Rosegarden website</a> for notes about this.</p>");

                            // whatever, don't show again during this run
                            showAltTimerWarning = false;
                        }

                        // if we got some informative text, shoot it out to the
                        // warning widget queue
                        if (informativeText != "")
                            emit sendWarning(WarningWidget::Timer, text, informativeText);

//                        CurrentProgressDialog::thaw();
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
    SEQMAN_DEBUG << "SequenceManager::rewindToBeginning()" << endl;
    m_doc->slotSetPointerPosition(m_doc->getComposition().getStartMarker());
}

void
SequenceManager::fastForwardToEnd()
{
    SEQMAN_DEBUG << "SequenceManager::fastForwardToEnd()" << endl;
    Composition &comp = m_doc->getComposition();
    m_doc->slotSetPointerPosition(comp.getEndMarker());
}

void
SequenceManager::setLoop(const timeT &lhs, const timeT &rhs)
{
    //!!!  So who disabled the following, why?  Are loops with JACK transport
    // sync no longer hideously broken?
    //
    // do not set a loop if JACK transport sync is enabled, because this is
    // completely broken, and apparently broken due to a limitation of JACK
    // transport itself.  #1240039 - DMM
    //    QSettings settings;
    //    settings.beginGroup( SequencerOptionsConfigGroup );
    // 

    //    if ( qStrToBool( settings.value("jacktransport", "false" ) ) )
    //    {
    //	//!!! message box should go here to inform user of why the loop was
    //	// not set, but I can't add it at the moment due to to the pre-release
    //	// freeze - DMM
    //    settings.endGroup();
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

    StartupLogo::hideIfStillThere();
    CurrentProgressDialog::freeze();

    QString text("");
    QString informativeText("");

    if (m_soundDriverStatus == NO_DRIVER) {
        text = tr("<h3>Sequencer engine unavailable!</h3>");
        informativeText = tr("<p>Both MIDI and Audio subsystems have failed to initialize.</p><p>If you wish to run with no sequencer by design, then use \"rosegarden --nosequencer\" to avoid seeing this error in the future.  Otherwise, we recommend that you repair your system configuration and start Rosegarden again.</p>");
    } else if (!(m_soundDriverStatus & MIDI_OK)) {
        text = tr("<h3>MIDI sequencing unavailable!</h3>");
        informativeText = tr("<p>The MIDI subsystem has failed to initialize.</p><p>You may continue without the sequencer, but we suggest closing Rosegarden, running \"modprobe snd-seq-midi\" as root, and starting Rosegarden again.  If you wish to run with no sequencer by design, then use \"rosegarden --nosequencer\" to avoid seeing this error in the future.</p>");
    }

    if (text != "") {
        emit sendWarning(WarningWidget::Midi, text, informativeText);
        return;
    } 

#ifdef HAVE_LIBJACK

    if (!(m_soundDriverStatus & AUDIO_OK)) {
        // This is to avoid us ever showing the same dialog more than
        // once during a single run of the program -- it's quite
        // separate from the suppression function
        static bool showJackWarning = true;

        if (showJackWarning) {
            text = tr("<h3>Audio sequencing and synth plugins unavailable!</h3>");
            informativeText = tr("<p>Rosegarden could not connect to the JACK audio server.  This probably means that Rosegarden was unable to start the audio server due to a problem with your configuration, your system installation, or both.</p><p>If you want to be able to play or record audio files or use plugins, we suggest that you exit Rosegarden and use the JACK Control utility (qjackctl) to try different settings until you arrive at a configuration that permits JACK to start.  You may also need to install a realtime kernel, edit your system security configuration, and so on.  Unfortunately, this is an extremely complex subject.</p><p> Once you establish a working JACK configuration, Rosegarden will be able to start the audio server automatically in the future.</p>");
            emit sendWarning(WarningWidget::Audio, text, informativeText);
            
            showJackWarning = false; 
        } 
    }
#endif
    CurrentProgressDialog::thaw();
}

void
SequenceManager::preparePlayback(bool /*forceProgramChanges*/)
{
    Studio &studio = m_doc->getStudio();
    const InstrumentList list = studio.getAllInstruments();

    // Send the MappedInstruments full information to the Sequencer 
    InstrumentList::const_iterator it = list.begin();
    for (; it != list.end(); ++it) {
        StudioControl::sendMappedInstrument(MappedInstrument(*it));
    }
}

void
SequenceManager::sendAudioLevel(MappedEvent *mE)
{
    foreach(RosegardenMainViewWidget* v, m_doc->getViewList()) {
        v->showVisuals(mE);
    }
}

void
SequenceManager::resetMidiNetwork()
{
    SEQMAN_DEBUG << "SequenceManager::resetMidiNetwork - resetting" << endl;
    MappedEventList mC;

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
    StudioControl::sendMappedEventList(mC);
}

void
SequenceManager::reinitialiseSequencerStudio()
{
    QSettings settings;
    settings.beginGroup( SequencerOptionsConfigGroup );

    // Toggle JACK audio ports appropriately
    //
    bool submasterOuts = qStrToBool( settings.value("audiosubmasterouts", "false" ) ) ;
    bool faderOuts = qStrToBool( settings.value("audiofaderouts", "false" ) ) ;
    unsigned int audioFileFormat = settings.value("audiorecordfileformat", 1).toUInt() ;

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

    settings.endGroup();
}

void
SequenceManager::panic()
{
    SEQMAN_DEBUG << "panic button" << endl;
    
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
SequenceManager::showVisuals(const MappedEventList &mC)
{
    MappedEventList::const_iterator it = mC.begin();
    if (it != mC.end())
        m_transport->setMidiOutLabel(*it);
}

MappedEventList
SequenceManager::applyFiltering(const MappedEventList &mC,
                                MappedEvent::MappedEventType filter)
{
    MappedEventList retMc;
    MappedEventList::const_iterator it = mC.begin();

    for (; it != mC.end(); ++it) {
        if (!((*it)->getType() & filter))
            retMc.insert(new MappedEvent(*it));
    }

    return retMc;
}

void SequenceManager::resetCompositionMapper()
{
    SEQMAN_DEBUG << "SequenceManager::resetCompositionMapper()" << endl;
    
    RosegardenSequencer::getInstance()->compositionAboutToBeDeleted();

    delete m_compositionMapper;
    m_compositionMapper = new CompositionMapper(m_doc);

    resetMetronomeMapper();
    resetTempoSegmentMapper();
    resetTimeSigSegmentMapper();
    resetControlBlock();
}

void SequenceManager::populateCompositionMapper()
{
    Composition &comp = m_doc->getComposition();

    for (Composition::iterator i = comp.begin(); i != comp.end(); ++i) {
        SEQMAN_DEBUG << "Adding segment with rid "
                     << (*i)->getRuntimeId() << endl;
        processAddedSegment(*i);
    }

    for (Composition::triggersegmentcontaineriterator i =
                comp.getTriggerSegments().begin();
         i != comp.getTriggerSegments().end(); ++i) {
        m_triggerSegments.insert(SegmentRefreshMap::value_type
                                 ((*i)->getSegment(),
                                  (*i)->getSegment()->getNewRefreshStatusId()));
    }

}
void SequenceManager::resetMetronomeMapper()
{
    SEQMAN_DEBUG << "SequenceManager::resetMetronomeMapper()" << endl;
    
    if (m_metronomeMapper) {
        RosegardenSequencer::getInstance()->segmentAboutToBeDeleted
            (m_metronomeMapper);
        m_metronomeMapper->removeOwner();
    }

    m_metronomeMapper = new MetronomeMapper(m_doc);
    m_metronomeMapper->addOwner();
    RosegardenSequencer::getInstance()->segmentAdded
        (m_metronomeMapper);
}

void SequenceManager::resetTempoSegmentMapper()
{
    SEQMAN_DEBUG << "SequenceManager::resetTempoSegmentMapper()" << endl;
    
    if (m_tempoSegmentMapper) {
        RosegardenSequencer::getInstance()->segmentAboutToBeDeleted
            (m_tempoSegmentMapper);
        m_tempoSegmentMapper->removeOwner();
    }

    m_tempoSegmentMapper = new TempoSegmentMapper(m_doc);
    m_tempoSegmentMapper->addOwner();
    RosegardenSequencer::getInstance()->segmentAdded
        (m_tempoSegmentMapper);
}

void SequenceManager::resetTimeSigSegmentMapper()
{
    SEQMAN_DEBUG << "SequenceManager::resetTimeSigSegmentMapper()" << endl;
    
    if (m_timeSigSegmentMapper) {
        RosegardenSequencer::getInstance()->segmentAboutToBeDeleted
            (m_timeSigSegmentMapper);
        m_timeSigSegmentMapper->removeOwner();
    }

    m_timeSigSegmentMapper = new TimeSigSegmentMapper(m_doc);
    m_timeSigSegmentMapper->addOwner();
    RosegardenSequencer::getInstance()->segmentAdded
        (m_timeSigSegmentMapper);
}

void SequenceManager::resetControlBlock()
{
    SEQMAN_DEBUG << "SequenceManager::resetControlBlockMapper()" << endl;
    
    ControlBlock::getInstance()->setDocument(m_doc);
}

bool SequenceManager::event(QEvent *e)
{
    if (e->type() == QEvent::User) {
        SEQMAN_DEBUG << "SequenceManager::event() with user event" << endl;
        if (m_updateRequested) {
            SEQMAN_DEBUG << "SequenceManager::event(): update requested"
                         << endl;
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
    SEQMAN_DEBUG << "SequenceManager::update()" << endl;
    // schedule a refresh-status check for the next event loop
    QEvent *e = new QEvent(QEvent::User);
    m_updateRequested = true;
    QApplication::postEvent(this, e);
}

void SequenceManager::checkRefreshStatus()
{
    SEQMAN_DEBUG << "SequenceManager::checkRefreshStatus()" << endl;
    
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

#ifdef DEBUG_SEQUENCE_MANAGER
    SEQMAN_DEBUG << "SequenceManager::checkRefreshStatus: segments modified by changes to trigger segments are:" << endl;
    int x = 0;
    for (TriggerSegmentRec::SegmentRuntimeIdSet::iterator i = ridset.begin();
            i != ridset.end(); ++i) {
        SEQMAN_DEBUG << x << ": " << *i << endl;
        ++x;
    }
#endif

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

void
SequenceManager::segmentModified(Segment* s)
{
    SEQMAN_DEBUG << "SequenceManager::segmentModified(" << s << ")";

    bool sizeChanged = m_compositionMapper->segmentModified(s);

    SEQMAN_DEBUG << "SequenceManager::segmentModified() : size changed = "
                 << sizeChanged;

    RosegardenSequencer::getInstance()->segmentModified
        (m_compositionMapper->getMappedEventBuffer(s));
}

void SequenceManager::segmentAdded(const Composition*, Segment* s)
{
    SEQMAN_DEBUG << "SequenceManager::segmentAdded(" << s
                 << "); queueing" 
                 << endl;
    m_addedSegments.push_back(s);
}

void SequenceManager::segmentRemoved(const Composition*, Segment* s)
{
    SEQMAN_DEBUG << "SequenceManager::segmentRemoved(" << s
                 << ")" << endl;

    // !!! WARNING !!!
    // The segment pointer "s" is about to be deleted by
    // Composition::deleteSegment(Composition::iterator).  After this routine
    // ends, this pointer cannot be dereferenced.
    m_removedSegments.push_back(s);

    std::vector<Segment*>::iterator i =
        find(m_addedSegments.begin(), m_addedSegments.end(), s);
    if (i != m_addedSegments.end()) {
        m_addedSegments.erase(i);
    }
}

void SequenceManager::segmentRepeatChanged(const Composition*, Segment* s, bool repeat)
{
    SEQMAN_DEBUG << "SequenceManager::segmentRepeatChanged(" << s
                 << ", " << repeat << ")" 
                 << endl;
    segmentModified(s);
}

void SequenceManager::segmentRepeatEndChanged(const Composition*, Segment* s, timeT newEndTime)
{
    SEQMAN_DEBUG << "SequenceManager::segmentRepeatEndChanged(" << s
                 << ", " << newEndTime << ")" 
                 << endl;
    segmentModified(s);
}

void SequenceManager::segmentEventsTimingChanged(const Composition*, Segment * s, timeT t, RealTime)
{
    SEQMAN_DEBUG << "SequenceManager::segmentEventsTimingChanged(" << s
                 << ", " << t << ")" 
                 << endl;
    segmentModified(s);
    if (s && s->getType() == Segment::Audio && m_transportStatus == PLAYING) {
        RosegardenSequencer::getInstance()->remapTracks();
    }
}

void SequenceManager::segmentTransposeChanged(const Composition*, Segment *s, int transpose)
{
    SEQMAN_DEBUG << "SequenceManager::segmentTransposeChanged(" << s
                 << ", " << transpose << ")" 
                 << endl;
    segmentModified(s);
}

void SequenceManager::segmentTrackChanged(const Composition*, Segment *s, TrackId id)
{
    SEQMAN_DEBUG << "SequenceManager::segmentTrackChanged(" << s
                 << ", " << id << ")" 
                 << endl;
    segmentModified(s);
    if (s && s->getType() == Segment::Audio && m_transportStatus == PLAYING) {
        RosegardenSequencer::getInstance()->remapTracks();
    }
}

void SequenceManager::segmentEndMarkerChanged(const Composition*, Segment *s, bool)
{
    SEQMAN_DEBUG << "SequenceManager::segmentEndMarkerChanged(" << s
                 << ")" << endl;
    segmentModified(s);
}

void SequenceManager::segmentInstrumentChanged(Segment *s)
{
    SEQMAN_DEBUG << "SequenceManager::segmentInstrumentChanged(" << s
                 << ")" << endl;
    // Quick and dirty: Redo the whole segment.
    segmentModified(s);
}

void SequenceManager::processAddedSegment(Segment* s)
{
    SEQMAN_DEBUG << "SequenceManager::processAddedSegment(" << s 
                 << ")" << endl;
    m_compositionMapper->segmentAdded(s);

    RosegardenSequencer::getInstance()->segmentAdded
        (m_compositionMapper->getMappedEventBuffer(s));

    // Add to segments map
    int id = s->getNewRefreshStatusId();
    m_segments.insert(SegmentRefreshMap::value_type(s, id));
}

void SequenceManager::processRemovedSegment(Segment* s)
{
    SEQMAN_DEBUG << "SequenceManager::processRemovedSegment()" << endl;
    // !!! WARNING !!!
    // The "s" segment pointer that is coming in to this routine has already
    // been deleted.  This is a POINTER TO DELETED MEMORY.  It cannot be
    // dereferenced in any way.  Each of the following lines of code will be
    // explained to make it clear that the pointer is not being dereferenced.

    {
        // getMappedEventBuffer() uses the segment pointer value as an
    	// index into a map.  So this is not a dereference.
        MappedEventBuffer *mapper =
            m_compositionMapper->getMappedEventBuffer(s);
        // segmentDeleted() has been reviewed and should only be using
        // the pointer as an index into a container.  segmentDeleted()
        // doesn't delete the mapper, which the metaiterators own.
        m_compositionMapper->segmentDeleted(s);
        RosegardenSequencer::getInstance()->segmentAboutToBeDeleted(mapper);
        // Now mapper may have been deleted.
    }
    // Remove from segments map
    // This uses "s" as an index.  It is not dereferenced.
    m_segments.erase(s);
}

void SequenceManager::endMarkerTimeChanged(const Composition *, bool /*shorten*/)
{
    SEQMAN_DEBUG << "SequenceManager::endMarkerTimeChanged()" << endl;

    if (m_transportStatus == RECORDING) {
        // When recording, we just need to extend the metronome segment
        // to include the new bars.
        // ??? This is pretty extreme as it destroys and recreates the
        //     segment.  Can't we just add events to the existing segment
        //     instead?  Maybe always have one or two bars extra so there
        //     is no interruption.  As it is, this will likely cause an
        //     interruption in metronome events when the composition is
        //     expanded.
        resetMetronomeMapper();
    } else {
        // Reset the composition mapper.  The main thing this does is
        // update the metronome segment.  There appear to be other
        // important things that need to be done as well.
        slotScheduledCompositionMapperReset();

        // This used to be done after a .5 second delay.  Doesn't seem
        // necessary now.
        // Call slotScheduledCompositionMapperReset() in .5 seconds to
        // reset the composition mapper based on the new composition size.
        //m_compositionMapperResetTimer->setSingleShot(true);
        //m_compositionMapperResetTimer->start(500);
    }
}

void SequenceManager::timeSignatureChanged(const Composition *)
{
    resetMetronomeMapper();
}

void SequenceManager::tracksAdded(const Composition* c, std::vector<TrackId> &trackIds)
{
    SEQMAN_DEBUG << "SequenceManager::tracksAdded()  tracks: " << trackIds.size();

    // For each track added, call ControlBlock::updateTrackData()
    for (unsigned i = 0; i < trackIds.size(); ++i) {
        SEQMAN_DEBUG << "  ID: " << trackIds[i];

        Track *t = c->getTrackById(trackIds[i]);
        ControlBlock::getInstance()->updateTrackData(t);

        // ??? Can we move this out of this for loop and call it once after
        //     we are done calling updateTrackData() for each track?
        if (m_transportStatus == PLAYING) {
            RosegardenSequencer::getInstance()->remapTracks();
        }
    }
}

void SequenceManager::trackChanged(const Composition *, Track* t)
{
    SEQMAN_DEBUG << "SequenceManager::trackChanged()  ID: " << t->getId();

    ControlBlock::getInstance()->updateTrackData(t);

    if (m_transportStatus == PLAYING) {
        RosegardenSequencer::getInstance()->remapTracks();
    }
}

void SequenceManager::tracksDeleted(const Composition *, std::vector<TrackId> &trackIds)
{
    SEQMAN_DEBUG << "SequenceManager::tracksDeleted()  tracks:" << trackIds.size();
    for (unsigned i = 0; i < trackIds.size(); ++i) {
        SEQMAN_DEBUG << "  ID: " << trackIds[i];
        ControlBlock::getInstance()->setTrackDeleted(trackIds[i], true);
    }
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
    if (regenerateTicks) resetMetronomeMapper();

    Composition &comp = m_doc->getComposition();
    ControlBlock::getInstance()->setInstrumentForMetronome(id);

    if (m_transportStatus == PLAYING) {
        ControlBlock::getInstance()->setMetronomeMuted(!comp.usePlayMetronome());
    } else {
        ControlBlock::getInstance()->setMetronomeMuted(!comp.useRecordMetronome());
    }

    m_metronomeMapper->refresh();
    m_timeSigSegmentMapper->refresh();
    m_tempoSegmentMapper->refresh();
}

void SequenceManager::metronomeChanged(const Composition *comp)
{
    // This method is called when the muting status in the composition
    // has changed -- the metronome itself has not actually changed

    SEQMAN_DEBUG << "SequenceManager::metronomeChanged "
                 << ", instrument = "
                 << m_metronomeMapper->getMetronomeInstrument()
                 << endl;
    if (!comp) comp = &m_doc->getComposition();
    ControlBlock::getInstance()->setInstrumentForMetronome
        (m_metronomeMapper->getMetronomeInstrument());

    if (m_transportStatus == PLAYING) {
        ControlBlock::getInstance()->setMetronomeMuted(!comp->usePlayMetronome());
    } else {
        ControlBlock::getInstance()->setMetronomeMuted(!comp->useRecordMetronome());
    }
}

void SequenceManager::filtersChanged(MidiFilter thruFilter,
                                     MidiFilter recordFilter)
{
    ControlBlock::getInstance()->setThruFilter(thruFilter);
    ControlBlock::getInstance()->setRecordFilter(recordFilter);
}

void SequenceManager::soloChanged(const Composition *, bool solo, TrackId selectedTrack)
{
    bool changed = false;
    ControlBlock *cb = ControlBlock::getInstance();
    if (solo != cb->isSolo() ||
        (solo && selectedTrack != cb->getSelectedTrack())) {
        changed = true;
    }
    cb->setSolo(solo);
    cb->setSelectedTrack(selectedTrack);
    if (changed && m_transportStatus == PLAYING) {
        RosegardenSequencer::getInstance()->remapTracks();
    }
}

void SequenceManager::tempoChanged(const Composition *c)
{
    SEQMAN_DEBUG << "SequenceManager::tempoChanged()" << endl;
    // Refresh all segments
    //
    for (SegmentRefreshMap::iterator i = m_segments.begin();
         i != m_segments.end(); ++i) {
        segmentModified(i->first);
    }

    // and metronome, time sig and tempo
    //
    m_metronomeMapper->refresh();
    m_timeSigSegmentMapper->refresh();
    m_tempoSegmentMapper->refresh();

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
        bool tracking = RosegardenMainWindow::self()->isTrackEditorPlayTracking();
        if (tracking)
            RosegardenMainWindow::self()->slotToggleTracking();
        m_doc->slotSetPointerPosition(c->getPosition());
        if (tracking)
            RosegardenMainWindow::self()->slotToggleTracking();
    }
}

void
SequenceManager::sendTransportControlStatuses()
{
    QSettings settings;
    settings.beginGroup( SequencerOptionsConfigGroup );

    // Get the settings values
    //
    bool jackTransport = qStrToBool( settings.value("jacktransport", "false" ) ) ;
    bool jackMaster = qStrToBool( settings.value("jackmaster", "false" ) ) ;

    int mmcMode = settings.value("mmcmode", 0).toInt() ;
    int mtcMode = settings.value("mtcmode", 0).toInt() ;

    int midiClock = settings.value("midiclock", 0).toInt() ;
    bool midiSyncAuto = qStrToBool( settings.value("midisyncautoconnect", "false" ) ) ;

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

    settings.endGroup();
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
                                     unsigned long /*kBSize*/,
                                     unsigned long /*kBUsed*/,
                                     unsigned long kBAvail)
{
    m_gotDiskSpaceResult = true;
    m_diskSpaceKBAvail = kBAvail;
}

void
SequenceManager::slotScheduledCompositionMapperReset()
{
    resetCompositionMapper();
    populateCompositionMapper();
}

void
SequenceManager::enableMIDIThruRouting(bool state)
{
    ControlBlock::getInstance()->setMidiRoutingEnabled(state);
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
    QSettings settings;
    settings.beginGroup( SequencerOptionsConfigGroup );

    QString timer = settings.value("timer").toString();
    settings.endGroup();

    if (timer == "(auto)" || timer == "") return true;
    else return false; // if the user has chosen the timer, leave them alone
}

// Return a new metaiterator on the current composition (suitable
// for MidiFile)
MappedBufMetaIterator *
SequenceManager::
makeTempMetaiterator(void)
{
    MappedBufMetaIterator *metaiterator = new MappedBufMetaIterator;
    // Add the mappers we know of.  Not the metronome because we don't
    // export that.
    metaiterator->addSegment(m_tempoSegmentMapper);
    metaiterator->addSegment(m_timeSigSegmentMapper);
    // We don't hold on to the marker mapper because we only use it
    // when exporting.
    metaiterator->addSegment(new MarkerMapper(m_doc));
    typedef CompositionMapper::SegmentMappers container;
    typedef container::iterator iterator;
    container &mapperContainer = m_compositionMapper->m_segmentMappers;
    for (iterator i = mapperContainer.begin();
         i != mapperContainer.end();
         ++i) {
        metaiterator->addSegment(i->second);
    }
    return metaiterator;
}

}
