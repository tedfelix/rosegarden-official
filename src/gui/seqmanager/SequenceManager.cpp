/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[SequenceManager]"

#define RG_NO_DEBUG_PRINT 1

#include "SequenceManager.h"

#include "sound/Midi.h"  // for MIDI_SYSTEM_RESET
#include "sound/ControlBlock.h"
#include "misc/Debug.h"
#include "misc/Strings.h"  // for qStrToBool()
#include "misc/ConfigGroups.h"
#include "base/Composition.h"
#include "base/Device.h"
#include "base/Exception.h"
#include "base/Instrument.h"
#include "base/MidiProgram.h"  // for MidiFilter
#include "base/RealTime.h"
#include "base/Segment.h"
#include "base/Studio.h"
#include "base/Track.h"
#include "base/TriggerSegment.h"
#include "CompositionMapper.h"
#include "document/RosegardenDocument.h"
#include "document/CommandHistory.h"
#include "gui/dialogs/AudioManagerDialog.h"
#include "gui/dialogs/CountdownDialog.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/widgets/StartupLogo.h"
#include "gui/studio/StudioControl.h"
#include "gui/widgets/WarningWidget.h"
#include "sequencer/RosegardenSequencer.h"
#include "MarkerMapper.h"
#include "MetronomeMapper.h"
#include "TempoSegmentMapper.h"
#include "TimeSigSegmentMapper.h"
#include "sound/AudioFile.h"  // For AudioFileId
#include "sound/MappedEventList.h"
#include "sound/MappedEvent.h"
#include "sound/MappedInstrument.h"

#include "rosegarden-version.h"  // for VERSION

#include <QSettings>
#include <QMessageBox>
#include <QApplication>
#include <QCursor>
#include <QEvent>
#include <QString>
#include <QTimer>

#include <utility>  // For std::pair.

namespace Rosegarden
{

SequenceManager::SequenceManager() :
    m_doc(nullptr),
    m_soundDriverStatus(NO_DRIVER),
    m_compositionMapper(),
    m_metronomeMapper(nullptr),
    m_tempoSegmentMapper(nullptr),
    m_timeSigSegmentMapper(nullptr),
    m_refreshRequested(true),
    m_segments(),
    m_triggerSegments(),
    m_addedSegments(),
    m_removedSegments(),
    m_shownOverrunWarning(false),
    m_reportTimer(nullptr),
    m_canReport(true),
    m_transportStatus(STOPPED),
    m_lastRewoundAt(clock()),
    m_countdownDialog(nullptr),
    m_countdownTimer(nullptr),
    m_recordTime(new QTime()),
    m_lastTransportStartPosition(0),
    m_realRecordStart(RealTime::zeroTime),
    m_sampleRate(0),
    m_tempo(0)
{
}

SequenceManager::~SequenceManager()
{
    RG_DEBUG << "dtor...";

    if (m_doc)
        m_doc->getComposition().removeObserver(this);
}

void
SequenceManager::setDocument(RosegardenDocument *doc)
{
    RG_DEBUG << "setDocument(" << doc << ")";

    DataBlockRepository::clear();

    if (m_doc)
        m_doc->getComposition().removeObserver(this);

    // Avoid duplicate connections.
    disconnect(CommandHistory::getInstance(), SIGNAL(commandExecuted()));

    m_segments.clear();
    m_triggerSegments.clear();

    m_doc = doc;

    m_doc->setSequenceManager(this);

    // Must recreate and reconnect the countdown timer and dialog
    // (bug #200 was 729039 "audio recording bug")
    //
    delete m_countdownDialog;
    delete m_countdownTimer;

    m_countdownDialog = new CountdownDialog(RosegardenMainWindow::self());

    // Bug #394: playback pointer wonkiness when stopping recording
    // (was 933041)  No longer connect the CountdownDialog from
    // SequenceManager; instead let the RosegardenMainWindow connect it to
    // its own slotStop to ensure the right housekeeping is done

    m_countdownTimer = new QTimer(m_doc);
    connect(m_countdownTimer, &QTimer::timeout,
            this, &SequenceManager::slotCountdownTimerTimeout);

    m_doc->getComposition().addObserver(this);

    connect(CommandHistory::getInstance(), SIGNAL(commandExecuted()),
            this, SLOT(update()));

    if (doc->isSoundEnabled()) {
        resetCompositionMapper();
        populateCompositionMapper();
    }
}

void
SequenceManager::play()
{
    if (!m_doc)
        return;

    Composition &comp = m_doc->getComposition();

    // If already playing or recording then stop
    if (m_transportStatus == PLAYING  ||
        m_transportStatus == RECORDING) {
        stop();
        return;
    }

    // This check may throw an exception
    checkSoundDriverStatus(false);

    // Make sure RosegardenSequencer has the right Instrument objects.
    preparePlayback();

    // Remember the last playback position so that we can return on stop.
    m_lastTransportStartPosition = comp.getPosition();

    // Update play metronome status
    ControlBlock::getInstance()->setInstrumentForMetronome(
            m_metronomeMapper->getMetronomeInstrument());
    ControlBlock::getInstance()->setMetronomeMuted(!comp.usePlayMetronome());

    // Depress the play button.
    emit signalPlaying(true);

    if (comp.getCurrentTempo() == 0) {
        RG_DEBUG << "play() - setting Tempo to Default value of 120.000";

        comp.setCompositionDefaultTempo(comp.getTempoForQpm(120.0));
    }

    setTempo(comp.getCurrentTempo());

    RealTime startPos = comp.getElapsedRealTime(comp.getPosition());

    // If we're looping then jump to loop start
    if (comp.isLooping())
        startPos = comp.getElapsedRealTime(comp.getLoopStart());

    int result = RosegardenSequencer::getInstance()->play(startPos);

    // Failed?  Bail.
    if (!result) {
        RG_WARNING << "play(): WARNING: Failed to start playback!";
        m_transportStatus = STOPPED;
        return;
    }

    m_transportStatus = STARTING_TO_PLAY;
}

void
SequenceManager::stop()
{
    if (!m_doc)
        return;

    if (m_countdownTimer)
        m_countdownTimer->stop();
    if (m_countdownDialog)
        m_countdownDialog->hide();

    // If the user presses stop while stopped, return to where we last started.
    if (m_transportStatus == STOPPED) {
        m_doc->slotSetPointerPosition(m_lastTransportStartPosition);
        return;
    }

    // If recording hasn't started yet, drop back to STOPPED.
    if (m_transportStatus == RECORDING_ARMED) {
        m_transportStatus = STOPPED;

        emit signalRecording(false);
        emit signalMetronomeActivated(m_doc->getComposition().usePlayMetronome());

        return;
    }

    RG_DEBUG << "stop() - preparing to stop playback or recording in progress";

    if (m_transportStatus == RECORDING) {
        emit signalRecording(false);
        emit signalMetronomeActivated(m_doc->getComposition().usePlayMetronome());
    }

    emit signalPlaying(false);

    // wait cursor
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // "call" the sequencer with a stop so we get a synchronous
    // response - then we can fiddle about with the audio file
    // without worrying about the sequencer causing problems
    // with access to the same audio files.
    RosegardenSequencer::getInstance()->stop();

    // restore
    QApplication::restoreOverrideCursor();

    TransportStatus previousStatus = m_transportStatus;

    // set new transport status first, so that if we're stopping
    // recording we don't risk the record segment being restored by a
    // timer while the document is busy trying to do away with it
    m_transportStatus = STOPPED;

    // if we were recording MIDI or Audio then tidy up the recording Segment
    if (previousStatus == RECORDING) {
        m_doc->stopRecordingMidi();
        m_doc->stopRecordingAudio();

        RG_DEBUG << "stop() - stopped recording";
    } else {
        m_doc->stopPlaying();
    }

    // always untoggle the play button at this stage
    emit signalPlaying(false);

    RG_DEBUG << "stop() - stopped playing";

    m_shownOverrunWarning = false;
}

void
SequenceManager::rewind()
{
    if (!m_doc)
        return;

    Composition &composition = m_doc->getComposition();

    timeT position = composition.getPosition();

    // Subtract one from position to make sure we go back one bar if we
    // are stopped and sitting at the beginning of a bar.
    std::pair<timeT, timeT> barRange =
        composition.getBarRangeForTime(position - 1);

    if (m_transportStatus == PLAYING) {
        // Compute elapsed time since last rewind press.
        clock_t now = clock();
        int elapsed = (now - m_lastRewoundAt) * 1000 / CLOCKS_PER_SEC;

        //RG_DEBUG << "rewind(): That was " << m_lastRewoundAt << ", this is " << now << ", elapsed is " << elapsed;

        // If we had a rewind press less than 200ms ago...
        if (elapsed >= 0 && elapsed <= 200) {

            timeT halfway = barRange.first +
                    (barRange.second - barRange.first) / 2;

            // If we're less than half way into the bar
            if (position <= halfway) {
                // Rewind an additional bar.
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
    if (!m_doc)
        return;

    Composition &composition = m_doc->getComposition();

    timeT position = composition.getPosition();
    timeT newPosition = composition.getBarEndForTime(position);

    // Don't skip past end marker.
    if (newPosition > composition.getEndMarker())
        newPosition = composition.getEndMarker();

    m_doc->slotSetPointerPosition(newPosition);
}

void
SequenceManager::jumpTo(const RealTime &time)
{
    RosegardenSequencer::getInstance()->jumpTo(time);
}

void
SequenceManager::record(bool toggled)
{
    if (!m_doc) return;

    m_realRecordStart = RealTime::zeroTime;

    RG_DEBUG << "record(" << toggled << ")";
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
                emit signalRecording(false);
                throw(Exception(QObject::tr("Audio subsystem is not available - can't record audio")));
            }
            // throws BadAudioPathException if path is not valid:
            m_doc->getAudioFileManager().testAudioPath();
            break;
        }
    }

    if (toggled) { // preparing record or punch-in record

        if (m_transportStatus == RECORDING_ARMED) {
            RG_DEBUG << "record() - unarming record";
            m_transportStatus = STOPPED;

            // Toggle the buttons
            emit signalMetronomeActivated(comp.usePlayMetronome());
            emit signalRecording(false);

            return ;
        }

        if (m_transportStatus == STOPPED) {
            RG_DEBUG << "record() - armed record";

            // Recording is now armed, but will not start until the countdown
            // is over.  Then RosegardenMainWindow::slotHandleInputs() will
            // sync us up to the current sequencer state.  If there is no
            // countdown, we will likely go directly to RECORDING.
            // ??? If there is a CountdownDialog, wouldn't we go directly to
            //     STOPPED due to RMW::slotHandleInputs()?  Or is that off in
            //     STOPPED?
            m_transportStatus = RECORDING_ARMED;

            // Toggle the buttons
            emit signalMetronomeActivated(comp.useRecordMetronome());
            emit signalRecording(true);

            return ;
        }

        if (m_transportStatus == RECORDING) {
            RG_DEBUG << "record() - stop recording and keep playing";
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
            RG_DEBUG << "record() - punch in recording";
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
            // TRANSLATORS: the pixmap in this error string contains no English
            // text and is suitable for use by all languages
            throw(Exception(QObject::tr("<qt><p>No tracks were armed for recording.</p><p>Please arm at least one of the recording LEDs <img src=\":pixmaps/tooltip/record-leds.png\"> and try again</p></qt>")));
        }

        // may throw an exception
        checkSoundDriverStatus(false);

        // toggle the Metronome button if it's in use
        emit signalMetronomeActivated(comp.useRecordMetronome());

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
                m_realRecordStart = comp.getElapsedRealTime(comp.getBarRange(startBar).first);
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
                    RG_DEBUG << "record(): mdoc->addRecordMIDISegment(" << *i << ")";
                    // Create the record MIDI segment now, so that the
                    // composition view has a real segment to display.  It
                    // won't actually be added to the composition until the
                    // first recorded event arrives.  We don't have to do this
                    // from here for audio, because for audio the sequencer
                    // calls back on createRecordAudioFiles so as to find out
                    // what files it needs to write to.
                    m_doc->addRecordMIDISegment(*i);

                    // ??? Why not softsynths too?
                    if (inst->getType() == Instrument::Midi) {
                        // Send Program Changes on recording tracks like 11.11.42
                        // used to.  Fix for bug #1356 "Wrong instrument when
                        // recording on multiple tracks/channels".  This is a
                        // simple way to support multiple MIDI controllers.
                        inst->sendChannelSetup();
                    }
                }
            }
        }

        // set the buttons
        emit signalRecording(true);
        emit signalPlaying(true);

        if (comp.getCurrentTempo() == 0) {
            RG_DEBUG << "record() - setting Tempo to Default value of 120.000";
            comp.setCompositionDefaultTempo(comp.getTempoForQpm(120.0));
        } else {
            RG_DEBUG << "record() - starting to record";
        }

        setTempo(comp.getCurrentTempo());

        // The arguments for the Sequencer - record is similar to playback,
        // we must being playing to record.
        //
        RealTime startPos =
            comp.getElapsedRealTime(comp.getPosition());

        int result = RosegardenSequencer::getInstance()->record(
                startPos,
                STARTING_TO_RECORD);  // recordMode

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
            // bug #941 (old #1505805), abolish recording countdown dialog.
            // ??? This was removed way back in [r7366] (2006-07-07).  But
            //     there appears to be renewed interest in feature request
            //     #453.  Should we clean this up or not?
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

    if (m_doc == nullptr || mC.size() == 0)
        return ;

    MappedEventList::const_iterator i;

    // Before applying thru filter, catch program changes and
    // send them to MIPP.

    int bankMSB = -1;
    int bankLSB = -1;

    // For each event
    for (i = mC.begin(); i != mC.end(); ++i) {

        const MappedEvent *event = (*i);

        // Bank Select

        // ??? This *requires* that BS and PC come in together.  Is there a
        //     guarantee that they will come in together?

        if (event->getType() == MappedEvent::MidiController) {
            int controlNumber = event->getData1();

            // If Bank Select LSB
            if (controlNumber == 32) {
                bankLSB = event->getData2();
            } else if (controlNumber == 0) {  // Bank Select MSB
                bankMSB = event->getData2();
            }
        }

        // Program Change

        if (event->getType() == MappedEvent::MidiProgramChange) {
            int programChange = event->getData1();

            // Send to MIPP.  If "Receive external" is checked, the MIPP
            // will update to show this bank and program.
            emit sigProgramChange(bankMSB, bankLSB, programChange);
        }

    }
	
    // send to the MIDI labels (which can only hold one event at a time)

    i = mC.begin();
    if (i != mC.end())
        emit signalMidiInLabel(*i);

    // Thru filtering is done at the sequencer for the actual sound
    // output, but here we need both filtered (for OUT display) and
    // unfiltered (for insertable note callbacks) compositions, so
    // we've received the unfiltered copy and will filter here

    MappedEvent::MappedEventType filter = MappedEvent::MappedEventType(
            m_doc->getStudio().getMIDIThruFilter());

    // For each event
    for (MappedEventList::const_iterator it = mC.begin();
         it != mC.end();
         ++it) {
        // Skip events destined for the "external controller" port.
        if ((*it)->getRecordedDevice() == Device::CONTROL_DEVICE)
            continue;

        // Skip events that are filtered.
        if (((*it)->getType() & filter) != 0)
            continue;

        // Show the event on the TransportDialog's MIDI out label.
        emit signalMidiOutLabel(*it);

        // We can only show one at a time.
        break;
    }

    for (i = mC.begin(); i != mC.end(); ++i ) {
        if ((*i)->getType() >= MappedEvent::Audio) {
            if ((*i)->getType() == MappedEvent::AudioStopped) {
                //RG_DEBUG << "AUDIO FILE ID = " << int((*i)->getData1()) << " - FILE STOPPED - " << "INSTRUMENT = " << (*i)->getInstrument();

                if (audioManagerDialog && (*i)->getInstrument() ==
                        m_doc->getStudio().getAudioPreviewInstrument()) {
                    audioManagerDialog->
                    closePlayingDialog(
                        AudioFileId((*i)->getData1()));
                }
            }

            if ((*i)->getType() ==
                    MappedEvent::AudioGeneratePreview) {
                RG_DEBUG << "processAsynchronousMidi(): Received AudioGeneratePreview: data1 is "
                         << int((*i)->getData1()) << ", data2 "
                         << int((*i)->getData2()) << ", instrument is "
                         << (*i)->getInstrument();
                m_doc->finalizeAudioFile((int)(*i)->getData1() +
                                         (int)(*i)->getData2() * 256);
            }

            if ((*i)->getType() == MappedEvent::SystemUpdateInstruments) {

                // resync Devices and Instruments
                //
//!DEVPUSH                m_doc->syncDevices();
            }

            // ??? Error handling appears to start here.  Can we pluck this
            //     out into a routine?  The indentation walls are closing in
            //     on us...

            if (m_transportStatus == PLAYING ||
                m_transportStatus == RECORDING) {
                if ((*i)->getType() == MappedEvent::SystemFailure) {

                    RG_DEBUG << "processAsynchronousMidi(): Failure of some sort...";

                    bool handling = true;

                    /* These are the ones that we always report or handle. */

                    if ((*i)->getData1() == MappedEvent::FailureJackDied) {

                        // Something horrible has happened to JACK or we got
                        // bumped out of the graph.  Either way stop playback.
                        //
                        stop();

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

                        stop();

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
                        RG_DEBUG << "processAsynchronousMidi(): Not reporting it to user just yet";
                        continue;
                    }

                    if ((*i)->getData1() == MappedEvent::FailureALSACallFailed) {

                        struct timeval tv;
                        (void)gettimeofday(&tv, nullptr);

                        if (tv.tv_sec - warningShownAt >= 5 &&
                                !boolShowingALSAWarning) {

                            QString message = tr("A serious error has occurred in the ALSA MIDI subsystem.  It may not be possible to continue sequencing.  Please check console output for more information.");
                            boolShowingALSAWarning = true;

                            QMessageBox::information(
                              nullptr,
                              "", /* no title */
                              message,
                              QMessageBox::Ok,
                              QMessageBox::Ok);
                            boolShowingALSAWarning = false;

                            (void)gettimeofday(&tv, nullptr);
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
                              nullptr,
                              "", /* no title */
                              message,
                              QMessageBox::Ok,
                              QMessageBox::Ok);
                        } else {
                            RG_WARNING << message;
                        }
#endif

                    }

                    // Turn off the report flag and set off a one-shot
                    // timer for 5 seconds.
                    if (!m_reportTimer->isActive()) {
                        // ??? This is never set back to true.
                        m_canReport = false;
                        // ??? This timer isn't connected to anything.  Looks
                        //     like it was supposed to be connected to
                        //     slotAllowReport().
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
                        // !!! Does this attempt to restart the "audio service"
                        //     EVER work?  I don't think I've ever seen that work.
                        //     When this is gone, it's gone, and time to restart.
                        //     But let's not change the translated message.
                        QMessageBox::critical(
                            RosegardenMainWindow::self(), "",
                            tr("The JACK Audio subsystem has stopped Rosegarden from processing audio, probably because of a processing overload.\nAn attempt to restart the audio service has been made, but some problems may remain.\nQuitting other running applications may improve Rosegarden's performance."));

                    } else if ((*i)->getData1() == MappedEvent::WarningImpreciseTimer &&
                               shouldWarnForImpreciseTimer()) {

                        RG_WARNING << "Rosegarden: WARNING: No accurate sequencer timer available";

//                        StartupLogo::hideIfStillThere();
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
                        
                    } else if ((*i)->getData1() == MappedEvent::WarningImpreciseTimerTryRTC &&
                               shouldWarnForImpreciseTimer()) {

                        RG_WARNING << "Rosegarden: WARNING: No accurate sequencer timer available (and kernel is new enough for RTC addendum)";

//                        StartupLogo::hideIfStillThere();
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
            RG_DEBUG << "processAsynchronousMidi(): Emitting controllerDeviceEventReceived()...";
            emit controllerDeviceEventReceived(*i);
        }
    }
}

void
SequenceManager::rewindToBeginning()
{
    RG_DEBUG << "rewindToBeginning()";
    m_doc->slotSetPointerPosition(m_doc->getComposition().getStartMarker());
}

void
SequenceManager::fastForwardToEnd()
{
    RG_DEBUG << "fastForwardToEnd()";
    Composition &comp = m_doc->getComposition();
    m_doc->slotSetPointerPosition(comp.getEndMarker());
}

void
SequenceManager::setLoop(const timeT &lhs, const timeT &rhs)
{
    // !!!  So who disabled the following, why?  Are loops with JACK transport
    //      sync no longer hideously broken?
    //
    // do not set a loop if JACK transport sync is enabled, because this is
    // completely broken, and apparently broken due to a limitation of JACK
    // transport itself.  #1240039 - DMM
    //    QSettings settings;
    //    settings.beginGroup( SequencerOptionsConfigGroup );
    // 

    //    if ( qStrToBool( settings.value("jacktransport", "false" ) ) )
    //    {
    //	// !!! message box should go here to inform user of why the loop was
    //	//     not set, but I can't add it at the moment due to to the pre-release
    //	//     freeze - DMM
    //    settings.endGroup();
    //	return;
    //    }

    RealTime loopStart =
        m_doc->getComposition().getElapsedRealTime(lhs);
    RealTime loopEnd =
        m_doc->getComposition().getElapsedRealTime(rhs);

    RosegardenSequencer::getInstance()->setLoop(loopStart, loopEnd);
}

bool SequenceManager::inCountIn(const RealTime &time) const
{
    if (m_transportStatus == RECORDING || m_transportStatus == STARTING_TO_RECORD) {
        if (time < m_realRecordStart) return true;
    }
    return false;
}

void
SequenceManager::checkSoundDriverStatus(bool warnUser)
{
    // Update local copy of status.
    // ??? Can we get rid of this member?  Only record() uses it.  Why
    //     not just let record() call getSoundDriverStatus(VERSION) directly?
    //     Then we can get rid of all the callers that call this with warnUser
    //     set to false.  Then we can get rid of warnUser.  No, it's worse
    //     than that.  There's also a getSoundDriverStatus() in here that
    //     provides access to this copy.  We would have to that over to
    //     providing RosegardenSequencer::getSoundDriverStatus().  Then
    //     probably inline that into each caller.  It's doable, but a bit
    //     more involved than it appears at first glance.  I'm also a little
    //     worried that this local status has more values than the
    //     RosegardenSequencer one.  Or perhaps it is out of sync and there's
    //     a reason.
    m_soundDriverStatus = RosegardenSequencer::getInstance()->
        getSoundDriverStatus(VERSION);

    RG_DEBUG << "checkSoundDriverStatus(): Sound driver status:" <<
            "MIDI" << (((m_soundDriverStatus & MIDI_OK) != 0) ? "ok" : "NOT OK") << "|" <<
            "Audio" << (((m_soundDriverStatus & AUDIO_OK) != 0) ? "ok" : "NOT OK") << "|" <<
            "Version" << (((m_soundDriverStatus & VERSION_OK) != 0) ? "ok" : "NOT OK");

    if (!warnUser)
        return;

#ifdef HAVE_LIBJACK
    if ((m_soundDriverStatus & (AUDIO_OK | MIDI_OK | VERSION_OK)) ==
        (AUDIO_OK | MIDI_OK | VERSION_OK)) return;
#else
    if ((m_soundDriverStatus & (MIDI_OK | VERSION_OK)) ==
        (MIDI_OK | VERSION_OK)) return;
#endif

    StartupLogo::hideIfStillThere();

    QString text;
    QString informativeText;

    if (m_soundDriverStatus == NO_DRIVER) {
        text = tr("<h3>Sequencer engine unavailable!</h3>");
        informativeText = tr("<p>Both MIDI and Audio subsystems have failed to initialize.</p><p>If you wish to run with no sequencer by design, then use \"rosegarden --nosound\" to avoid seeing this error in the future.  Otherwise, we recommend that you repair your system configuration and start Rosegarden again.</p>");
    } else if (!(m_soundDriverStatus & MIDI_OK)) {
        text = tr("<h3>MIDI sequencing unavailable!</h3>");
        informativeText = tr("<p>The MIDI subsystem has failed to initialize.</p><p>You may continue without the sequencer, but we suggest closing Rosegarden, running \"modprobe snd-seq-midi\" as root, and starting Rosegarden again.  If you wish to run with no sequencer by design, then use \"rosegarden --nosound\" to avoid seeing this error in the future.</p>");
    }

    if (!text.isEmpty()) {
        emit sendWarning(WarningWidget::Midi, text, informativeText);
        return;
    } 

#ifdef HAVE_LIBJACK

    if (!(m_soundDriverStatus & AUDIO_OK)) {
        // This is to avoid us ever showing the same dialog more than
        // once during a single run of the program -- it's quite
        // separate from the suppression function
        // ??? But this routine is only ever called with "true" once.  From
        //     RMW's ctor.  There is no need for this.
        static bool showJackWarning = true;

        if (showJackWarning) {
            text = tr("<h3>Audio sequencing and synth plugins unavailable!</h3>");
            informativeText = tr("<p>Rosegarden could not connect to the JACK audio server.  This probably means that Rosegarden was unable to start the audio server due to a problem with your configuration, your system installation, or both.</p><p>If you want to be able to play or record audio files or use plugins, we suggest that you exit Rosegarden and use the JACK Control utility (qjackctl) to try different settings until you arrive at a configuration that permits JACK to start.  You may also need to install a realtime kernel, edit your system security configuration, and so on.  Unfortunately, this is an extremely complex subject.</p><p> Once you establish a working JACK configuration, Rosegarden will be able to start the audio server automatically in the future.</p>");
            emit sendWarning(WarningWidget::Audio, text, informativeText);
            
            showJackWarning = false; 
        } 
    }
#endif
}

void
SequenceManager::preparePlayback()
{
    // ??? rename: setMappedInstruments()

    // ??? Where does this function really belong?  It iterates over the
    //     Instrument's in the Studio and calls
    //     RosegardenSequencer::setMappedInstrument().  Seems like
    //     RosegardenSequencer might be a better place.  Or would Studio
    //     make more sense?

    Studio &studio = m_doc->getStudio();
    const InstrumentList list = studio.getAllInstruments();

    // Send the MappedInstruments full information to the Sequencer 
    InstrumentList::const_iterator it = list.begin();
    for (; it != list.end(); ++it) {
        StudioControl::sendMappedInstrument(MappedInstrument(*it));
    }
}

void
SequenceManager::resetMidiNetwork()
{
    MappedEventList mC;

    // ??? This should send resets on all MIDI channels on all MIDI
    //     Device's.  As it is now, it only does the first Device.

    for (unsigned int i = 0; i < 16; ++i) {
        MappedEvent *mE =
            new MappedEvent(MidiInstrumentBase + i,
                            MappedEvent::MidiController,
                            MIDI_SYSTEM_RESET,
                            0);

        mC.insert(mE);

        // Display the first one on the TransportDialog.
        if (i == 0)
            emit signalMidiOutLabel(mE);
    }

    // Send it out.
    StudioControl::sendMappedEventList(mC);
}

void
SequenceManager::reinitialiseSequencerStudio()
{
    // ??? static function.  What class does this really belong in?
    //     RosegardenSequencer seems logical since all this does is call
    //     RosegardenSequencer::processMappedEvent() based on the config file.

    QSettings settings;
    settings.beginGroup( SequencerOptionsConfigGroup );

    // Toggle JACK audio ports appropriately

    MidiByte ports = 0;

    bool faderOuts =
            qStrToBool( settings.value("audiofaderouts", "false" ) ) ;
    if (faderOuts)
        ports |= MappedEvent::FaderOuts;

    bool submasterOuts =
            qStrToBool( settings.value("audiosubmasterouts", "false" ) ) ;
    if (submasterOuts)
        ports |= MappedEvent::SubmasterOuts;

    MappedEvent mEports(
            MidiInstrumentBase, MappedEvent::SystemAudioPorts, ports);
    StudioControl::sendMappedEvent(mEports);

    // Audio File Format

    unsigned int audioFileFormat =
            settings.value("audiorecordfileformat", 1).toUInt() ;

    MappedEvent mEff(
            MidiInstrumentBase, MappedEvent::SystemAudioFileFormat,
            audioFileFormat);
    StudioControl::sendMappedEvent(mEff);

    settings.endGroup();
}

void
SequenceManager::panic()
{
    RG_DEBUG << "panic()";
    
    stop();

    MappedEvent mE(MidiInstrumentBase, MappedEvent::Panic, 0, 0);
    StudioControl::sendMappedEvent(mE);
}

void SequenceManager::setTempo(const tempoT tempo)
{
    if (m_tempo == tempo)
        return;
    m_tempo = tempo;

    // Send the quarter note length to the sequencer.
    // Quarter Note Length is sent (MIDI CLOCK) at 24ppqn.
    //
    double qnD = 60.0 / Composition::getTempoQpm(tempo);
    RealTime qnTime =
        RealTime(long(qnD),
                 long((qnD - double(long(qnD))) * 1000000000.0));

    StudioControl::sendQuarterNoteLength(qnTime);

    // set the tempo in the transport
    emit signalTempoChanged(tempo);
}

void SequenceManager::resetCompositionMapper()
{
    RG_DEBUG << "resetCompositionMapper()";
    
    RosegardenSequencer::getInstance()->compositionAboutToBeDeleted();

    m_compositionMapper.reset(new CompositionMapper(m_doc));

    resetMetronomeMapper();
    resetTempoSegmentMapper();
    resetTimeSigSegmentMapper();

    // Reset ControlBlock.
    ControlBlock::getInstance()->setDocument(m_doc);
}

void SequenceManager::populateCompositionMapper()
{
    Composition &comp = m_doc->getComposition();

    for (Composition::iterator i = comp.begin(); i != comp.end(); ++i) {
        RG_DEBUG << "populateCompositionMapper(): Adding segment with rid " << (*i)->getRuntimeId();
        segmentAdded(*i);
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
    RG_DEBUG << "resetMetronomeMapper()";
    
    if (m_metronomeMapper) {
        RosegardenSequencer::getInstance()->segmentAboutToBeDeleted
            (m_metronomeMapper);
    }

    m_metronomeMapper =
            QSharedPointer<MetronomeMapper>(new MetronomeMapper(m_doc));
    RosegardenSequencer::getInstance()->segmentAdded
        (m_metronomeMapper);
}

void SequenceManager::resetTempoSegmentMapper()
{
    RG_DEBUG << "resetTempoSegmentMapper()";
    
    if (m_tempoSegmentMapper) {
        RosegardenSequencer::getInstance()->segmentAboutToBeDeleted
            (m_tempoSegmentMapper);
    }

    m_tempoSegmentMapper =
            QSharedPointer<TempoSegmentMapper>(new TempoSegmentMapper(m_doc));
    RosegardenSequencer::getInstance()->segmentAdded
        (m_tempoSegmentMapper);
}

void SequenceManager::resetTimeSigSegmentMapper()
{
    RG_DEBUG << "resetTimeSigSegmentMapper()";
    
    if (m_timeSigSegmentMapper) {
        RosegardenSequencer::getInstance()->segmentAboutToBeDeleted
            (m_timeSigSegmentMapper);
    }

    m_timeSigSegmentMapper =
            QSharedPointer<TimeSigSegmentMapper>(new TimeSigSegmentMapper(m_doc));
    RosegardenSequencer::getInstance()->segmentAdded
        (m_timeSigSegmentMapper);
}

bool SequenceManager::event(QEvent *e)
{
    if (e->type() == QEvent::User) {
        //RG_DEBUG << "event() with user event";
        if (m_refreshRequested) {
            //RG_DEBUG << "event(): update requested";
            refresh();
            m_refreshRequested = false;
        }
        return true;
    } else {
        return QObject::event(e);
    }
}

void SequenceManager::update()
{
    //RG_DEBUG << "update()";
    // schedule a refresh-status check for the next event loop
    QEvent *e = new QEvent(QEvent::User);
    // Let the handler know we want a refresh().
    // ??? But we always want a refresh().  When wouldn't we?  Are
    //     we getting QEvent::User events from elsewhere?
    m_refreshRequested = true;
    QApplication::postEvent(this, e);
}

void SequenceManager::refresh()
{
    RG_DEBUG << "refresh()";

    Composition &comp = m_doc->getComposition();

    // ??? See Segment::m_refreshStatusArray for insight into what this
    //     is doing.

    // Look at trigger segments first: if one of those has changed, we'll
    // need to be aware of it when scanning segments subsequently

    // List of Segments modified by changes to trigger Segments.  These
    // will need a refresh.
    // ??? Instead of gathering these, can't we just set the refresh
    //     status for the Segment?  Or would that cause others to do
    //     refreshes?  Can we just set *our* refresh status?
    // ??? rename: triggerRefreshSet
    TriggerSegmentRec::SegmentRuntimeIdSet ridset;

    // List of all trigger Segments.
    SegmentRefreshMap newTriggerMap;

    // For each trigger Segment in the composition
    for (Composition::triggersegmentcontaineriterator i =
             comp.getTriggerSegments().begin();
         i != comp.getTriggerSegments().end(); ++i) {

        Segment *s = (*i)->getSegment();

        // If we don't have this one
        if (m_triggerSegments.find(s) == m_triggerSegments.end()) {
            // Make a new trigger Segment entry.
            newTriggerMap[s] = s->getNewRefreshStatusId();
        } else {
            // Use the existing entry.
            newTriggerMap[s] = m_triggerSegments[s];
        }

        // If this trigger Segment needs a refresh
        if (s->getRefreshStatus(newTriggerMap[s]).needsRefresh()) {
            // Collect all the Segments this will affect.
            TriggerSegmentRec::SegmentRuntimeIdSet &thisSet =
                    (*i)->getReferences();
            ridset.insert(thisSet.begin(), thisSet.end());

            // Clear the trigger Segment's refresh flag.
            s->getRefreshStatus(newTriggerMap[s]).setNeedsRefresh(false);
        }
    }

    m_triggerSegments = newTriggerMap;

#if 0
    RG_DEBUG << "refresh(): segments modified by changes to trigger segments are:";
    int x = 0;
    for (TriggerSegmentRec::SegmentRuntimeIdSet::iterator i = ridset.begin();
            i != ridset.end(); ++i) {
        RG_DEBUG << x << ": " << *i;
        ++x;
    }
#endif

    std::vector<Segment*>::iterator i;

    // Removed Segments

    for (i = m_removedSegments.begin(); i != m_removedSegments.end(); ++i) {
        segmentDeleted(*i);
    }
    m_removedSegments.clear();

    RG_DEBUG << "refresh(): we have " << m_segments.size() << " segments";

    // Current Segments

    for (SegmentRefreshMap::iterator i = m_segments.begin();
            i != m_segments.end(); ++i) {
        // If this Segment needs a refresh
        if (i->first->getRefreshStatus(i->second).needsRefresh() ||
                ridset.find(i->first->getRuntimeId()) != ridset.end()) {
            segmentModified(i->first);
            i->first->getRefreshStatus(i->second).setNeedsRefresh(false);
        }
    }

    // Added Segments

    for (i = m_addedSegments.begin(); i != m_addedSegments.end(); ++i) {
        segmentAdded(*i);
    }
    m_addedSegments.clear();
}

void
SequenceManager::segmentModified(Segment* s)
{
    RG_DEBUG << "segmentModified(" << s << ")";

    bool sizeChanged = m_compositionMapper->segmentModified(s);

    RG_DEBUG << "segmentModified() : size changed = " << sizeChanged;

    RosegardenSequencer::getInstance()->segmentModified
        (m_compositionMapper->getMappedEventBuffer(s));
}

void SequenceManager::segmentAdded(const Composition*, Segment* s)
{
    RG_DEBUG << "segmentAdded(" << s << "); queueing";
    m_addedSegments.push_back(s);
}

void SequenceManager::segmentRemoved(const Composition*, Segment* s)
{
    RG_DEBUG << "segmentRemoved(" << s << ")";

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
    RG_DEBUG << "segmentRepeatChanged(" << s << ", " << repeat << ")";
    segmentModified(s);
}

void SequenceManager::segmentRepeatEndChanged(const Composition*, Segment* s, timeT newEndTime)
{
    RG_DEBUG << "segmentRepeatEndChanged(" << s << ", " << newEndTime << ")";
    segmentModified(s);
}

void SequenceManager::segmentEventsTimingChanged(const Composition*, Segment * s, timeT t, RealTime)
{
    RG_DEBUG << "segmentEventsTimingChanged(" << s << ", " << t << ")";
    segmentModified(s);
    if (s && s->getType() == Segment::Audio && m_transportStatus == PLAYING) {
        RosegardenSequencer::getInstance()->remapTracks();
    }
}

void SequenceManager::segmentTransposeChanged(const Composition*, Segment *s, int transpose)
{
    RG_DEBUG << "segmentTransposeChanged(" << s << ", " << transpose << ")";
    segmentModified(s);
}

void SequenceManager::segmentTrackChanged(const Composition*, Segment *s, TrackId id)
{
    RG_DEBUG << "segmentTrackChanged(" << s << ", " << id << ")";
    segmentModified(s);
    if (s && s->getType() == Segment::Audio && m_transportStatus == PLAYING) {
        RosegardenSequencer::getInstance()->remapTracks();
    }
}

void SequenceManager::segmentEndMarkerChanged(const Composition*, Segment *s, bool)
{
    RG_DEBUG << "segmentEndMarkerChanged(" << s << ")";
    segmentModified(s);
}

void SequenceManager::segmentInstrumentChanged(Segment *s)
{
    RG_DEBUG << "segmentInstrumentChanged(" << s << ")";
    // Quick and dirty: Redo the whole segment.
    segmentModified(s);
}

void SequenceManager::segmentAdded(Segment* s)
{
    RG_DEBUG << "segmentAdded(" << s << ")";
    m_compositionMapper->segmentAdded(s);

    RosegardenSequencer::getInstance()->segmentAdded
        (m_compositionMapper->getMappedEventBuffer(s));

    // Add to segments map
    int id = s->getNewRefreshStatusId();
    m_segments.insert(SegmentRefreshMap::value_type(s, id));
}

void SequenceManager::segmentDeleted(Segment* s)
{
    RG_DEBUG << "segmentDeleted(" << s << ")";

    // !!! WARNING !!!
    // The "s" segment pointer that is coming in to this routine has already
    // been deleted.  This is a POINTER TO DELETED MEMORY.  It cannot be
    // dereferenced in any way.  Each of the following lines of code will be
    // explained to make it clear that the pointer is not being dereferenced.
    // ??? This needs to be fixed.  Passing around pointers that point to
    //     nowhere is just asking for trouble.  E.g. what if the same memory
    //     address is allocated to a new Segment, that Segment is added, then
    //     this routine is called for the old Segment?  We remove the
    //     new Segment.

    {
        // getMappedEventBuffer() uses the segment pointer value as an
    	// index into a map.  So this is not a dereference.
        QSharedPointer<MappedEventBuffer> mapper =
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
    RG_DEBUG << "endMarkerTimeChanged()";

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
    }
}

void SequenceManager::timeSignatureChanged(const Composition *)
{
    resetMetronomeMapper();
}

void SequenceManager::tracksAdded(const Composition* c, std::vector<TrackId> &trackIds)
{
    RG_DEBUG << "tracksAdded()  tracks: " << trackIds.size();

    // For each track added, call ControlBlock::updateTrackData()
    for (unsigned i = 0; i < trackIds.size(); ++i) {
        RG_DEBUG << "  ID: " << trackIds[i];

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
    RG_DEBUG << "trackChanged()  ID: " << t->getId();

    ControlBlock::getInstance()->updateTrackData(t);

    if (m_transportStatus == PLAYING) {
        RosegardenSequencer::getInstance()->remapTracks();
    }
}

void SequenceManager::tracksDeleted(const Composition *, std::vector<TrackId> &trackIds)
{
    RG_DEBUG << "tracksDeleted()  tracks:" << trackIds.size();

    for (unsigned i = 0; i < trackIds.size(); ++i) {
        RG_DEBUG << "  ID: " << trackIds[i];
        ControlBlock::getInstance()->setTrackDeleted(trackIds[i], true);
    }
}

void SequenceManager::metronomeChanged(InstrumentId id,
                                       bool regenerateTicks)
{
    // This method is called when the user has changed the
    // metronome instrument, pitch etc

    RG_DEBUG << "metronomeChanged() (simple)" << ", instrument = " << id;
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

    RG_DEBUG << "metronomeChanged() " << ", instrument = " << m_metronomeMapper->getMetronomeInstrument();
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

void SequenceManager::selectedTrackChanged(const Composition *composition)
{
    TrackId selectedTrackId = composition->getSelectedTrack();
    ControlBlock::getInstance()->setSelectedTrack(selectedTrackId);
}

void SequenceManager::tempoChanged(const Composition *c)
{
    RG_DEBUG << "tempoChanged()";

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

        // Tempo has changed during playback.

        // Reset the playback position because the sequencer keeps track of
        // position in real time (seconds) and we want to maintain the same
        // position in musical time (bars/beats).
        m_doc->slotSetPointerPosition(c->getPosition());

    }
}

void
SequenceManager::sendTransportControlStatuses()
{
    // ??? static function.  Where does this really belong?  I suspect
    //     RosegardenSequencer.

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
SequenceManager::slotScheduledCompositionMapperReset()
{
    // ??? Inline into only caller.
    resetCompositionMapper();
    populateCompositionMapper();
}

int
SequenceManager::getSampleRate() const
{
    // Get from cache if it's there.
    if (m_sampleRate != 0)
        return m_sampleRate;

    // Cache the result to avoid locks.
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
makeTempMetaiterator()
{
    MappedBufMetaIterator *metaiterator = new MappedBufMetaIterator;
    // Add the mappers we know of.  Not the metronome because we don't
    // export that.
    metaiterator->addSegment(m_tempoSegmentMapper);
    metaiterator->addSegment(m_timeSigSegmentMapper);
    // We don't hold on to the marker mapper because we only use it
    // when exporting.
    metaiterator->addSegment(QSharedPointer<MarkerMapper>(new MarkerMapper(m_doc)));
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
