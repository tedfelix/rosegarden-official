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

#ifndef RG_SEQUENCEMANAGER_H
#define RG_SEQUENCEMANAGER_H

#include "base/Composition.h"
#include "base/RealTime.h"
#include "gui/application/TransportStatus.h"
#include "sound/MappedEventList.h"
#include "sound/MappedEvent.h"

#include <QObject>
#include <QString>

class QTimer;
class QTime;
class QEvent;

#include <vector>
#include <map>

namespace Rosegarden
{


class Track;
class TrackEditor;
class TimeSigSegmentMapper;
class TempoSegmentMapper;
class Segment;
class RosegardenDocument;
class MetronomeMapper;
class CountdownDialog;
class CompositionMapper;
class AudioManagerDialog;
class MappedBufMetaIterator;


/**
 * A layer between the UI (RosegardenMainWindow) and the sequencer
 * (RosegardenSequencer).
 */
class ROSEGARDENPRIVATE_EXPORT SequenceManager :
        public QObject, public CompositionObserver
{
    Q_OBJECT
public:
    /**
     * SequenceManager is not designed to operate without a document;
     * you must call setDocument before you do anything with it.
     */
    SequenceManager();
    ~SequenceManager();

    /**
     * Sets (replaces) the internal document, and sets a parent widget for
     * the CountDownDialog.
     *
     * ??? Subscribe for RMW::documentChanged() instead of this.
     *     parentWidget is just RMW::self().
     */
    void setDocument(RosegardenDocument *doc, QWidget *parentWidget);

    /**
     * Update m_soundDriverStatus.
     *
     * This is called at startup and by play() and record().  At startup,
     * warnUser is true which indicates that a warning dialog can be
     * shown if there is a problem with the setup.  In other cases, the
     * status is only sent to the debug output.  And only if debug output
     * is enabled.
     */
    void checkSoundDriverStatus(bool warnUser);

    /// Find what has been initialised and what hasn't
    unsigned int getSoundDriverStatus() const  { return m_soundDriverStatus; }

    /// Reinitialise the studio.
    /**
     * Sends a SystemAudioPorts and a SystemAudioFileFormat event to
     * RosegardenSequencer.
     *
     * Called only by RosegardenMainWindow's ctor.
     */
    static void reinitialiseSequencerStudio();

    /// Send JACK and MMC transport control statuses
    static void sendTransportControlStatuses();

    /// Align Instrument lists before playback starts.
    void preparePlayback();

    //
    // Transport controls
    //

    /// Start or pause playback.
    void play();
    /// Stop playback.
    void stop();
    /// Start recording.
    void record(bool countIn);

    void jumpTo(const RealTime &time);

    void setLoop(const timeT &lhs, const timeT &rhs);

    void setTransportStatus(TransportStatus status)
            { m_transportStatus = status; }
    TransportStatus getTransportStatus() const  { return m_transportStatus; }

    /**
     * Sends tempo to RosegardenSequencer::setQuarterNoteLength().
     *
     * Emits signalTempoChanged() which is connected to
     * TransportDialog::slotTempoChanged().
     */
    void setTempo(const tempoT tempo);

    /// Handle incoming MappedEvent's.
    /**
     * This handles both incoming events when recording and incoming events
     * that are unrelated to recording.
     *
     * The events come from RosegardenMainWindow who gets them from
     * SequencerDataBlock and RosegardenSequencer.
     *
     * This routine mainly emits the following signals which are handled
     * by various parts of the UI:
     *
     *   - sigProgramChange() -> MIPP::slotExternalProgramChange()
     *   - signalMidiInLabel() -> TransportDialog::slotMidiInLabel()
     *   - signalMidiOutLabel() -> TransportDialog::slotMidiOutLabel()
     *   - insertableNoteOffReceived()
     *     -> NotationView::slotInsertableNoteOffReceived()
     *     -> PitchTrackerView::slotInsertableNoteOffReceived()
     *     -> MatrixView::slotInsertableNoteOffReceived()
     *   - insertableNoteOnReceived()
     *     -> NotationView::slotInsertableNoteOnReceived()
     *     -> PitchTrackerView::slotInsertableNoteOnReceived()
     *     -> MatrixView::slotInsertableNoteOnReceived()
     *   - controllerDeviceEventReceived()
     *     -> RMVW::slotControllerDeviceEventReceived()
     *
     * This routine also performs extensive error checking and displays
     * error messages when a problem is detected.
     *
     * ??? It feels to me like this spaghetti can be simplified.  Why not
     *     create a new AsyncMIDIHandler object that makes a more direct
     *     connection between RosegardenSequencer/SequencerDataBlock and
     *     the UI?  That would be a first step.  Then we might be able to
     *     make the connections even more direct.
     */
    void processAsynchronousMidi(const MappedEventList &mC,
                                 AudioManagerDialog *aMD);

    /// Reset MIDI network.  Send an FF Reset on all devices and channels.
    void resetMidiNetwork();

    /// Send all note offs and resets to MIDI devices.
    /**
     * The actual work appears to be done by AlsaDriver::processEventsOut()
     * when it sees a MappedEvent::Panic.
     */
    void panic();

    /// ??? The CountdownDialog has been disabled.
    CountdownDialog *getCountdownDialog()  { return m_countdownDialog; }

    /// Assemble and return a meta-iterator for MIDI file generation.
    MappedBufMetaIterator *makeTempMetaiterator();

    //
    // CompositionObserver interface
    //

    virtual void segmentAdded(const Composition *, Segment *);
    virtual void segmentRemoved(const Composition *, Segment *);
    virtual void segmentRepeatChanged(const Composition *, Segment *, bool);
    virtual void segmentRepeatEndChanged(const Composition *, Segment *, timeT);
    virtual void segmentEventsTimingChanged(const Composition *, Segment *, timeT delay, RealTime rtDelay);
    virtual void segmentTransposeChanged(const Composition *, Segment *, int transpose);
    virtual void segmentTrackChanged(const Composition *, Segment *, TrackId id);
    virtual void segmentEndMarkerChanged(const Composition *, Segment *, bool);
    virtual void endMarkerTimeChanged(const Composition *, bool shorten);
    virtual void tracksAdded(const Composition *, std::vector<TrackId> &/*trackIds*/);
    virtual void trackChanged(const Composition *, Track *);
    virtual void tracksDeleted(const Composition *, std::vector<TrackId> &/*trackIds*/);
    virtual void timeSignatureChanged(const Composition *);
    virtual void metronomeChanged(const Composition *);
    virtual void selectedTrackChanged(const Composition *);
    virtual void tempoChanged(const Composition *);

    /**
     * Called by TrackButtons.
     *
     * ??? Is there a notification we can subscribe to instead?
     */
    void segmentInstrumentChanged(Segment *s);

    /// Handle QEvent::User messages from update().
    /**
     * QObject override.
     *
     * Calls checkRefreshStatus().
     */
    virtual bool event(QEvent *e);

    /// Sets a new Instrument for the metronome and regenerates the ticks.
    /**
     * Used by ManageMetronomeDialog and MIDIConfigurationPage to reconfigure
     * the metronome.
     *
     * ??? Both callers set regenerateTicks to true.  Get rid of it.
     *
     * @see resetMetronomeMapper(), ControlBlock::setInstrumentForMetronome()
     */
    void metronomeChanged(InstrumentId id, bool regenerateTicks);

    /// Forward new filtering parameters to ControlBlock.
    /**
     * Used by MidiFilterDialog.
     */
    static void filtersChanged(MidiFilter thruFilter,
                               MidiFilter recordFilter);

    /// Get sample rate from RosegardenSequencer.
    int getSampleRate() const;

public slots:

    /**
     * Connected to CommandHistory::commandExecuted() to ensure an update
     * after every command.
     */
    void update();

    // Transport Controls (cont'd)

    // The following are slots for the test driver:
    // test_notationview_selection.cpp

    void rewind();
    void fastforward();
    void rewindToBeginning();
    void fastForwardToEnd();

signals:
    /// A program change was received.
    /**
     * Emitted by processAsynchronousMidi().
     *
     * Connected to MIDIInstrumentParameterPanel::slotExternalProgramChange().
     *
     * Incoming program changes from a connected device are sent to the MIPP
     * where, if the "Receive External" checkbox is checked, the bank and
     * program on the MIPP will be changed to match.
     */
    void sigProgramChange(int bankMSB, int bankLSB, int programChange);

    /// A note-on event was received that might be of interest to the editors.
    /**
     * Emitted by processAsynchronousMidi().
     *
     * Connected to MatrixView, NotationView, and PitchTrackerView for
     * step editing features.
     */
    void insertableNoteOnReceived(int pitch, int velocity);

    /// A note-off event was received that might be of interest to the editors.
    /**
     * Emitted by processAsynchronousMidi().
     *
     * Connected to MatrixView, NotationView, and PitchTrackerView for
     * step editing features.
     */
    void insertableNoteOffReceived(int pitch, int velocity);

    /// An event was received from the "external controller" port.
    /**
     * Emitted by processAsynchronousMidi().
     *
     * Connected to
     * RosegardenMainViewWindow::slotControllerDeviceEventReceived().  These
     * are events from the "external controller" port.
     */
    void controllerDeviceEventReceived(MappedEvent *event);

    /// Signal RosegardenMainWindow to display a warning.
    /**
     * Connected to RosegardenMainWindow::slotDisplayWarning().
     *
     * RMW displays an appropriate warning icon on the status bar
     * (WarningWidget) and displays a WarningDialog in response to this.
     */
    void sendWarning(int type, QString text, QString informativeText);

    // TransportDialog signals

    /// Connected to TransportDialog::slotTempoChanged().
    void signalTempoChanged(tempoT tempo);
    /// Connected to TransportDialog::slotMidiInLabel().
    void signalMidiInLabel(const MappedEvent *event);
    /// Connected to TransportDialog::slotMidiOutLabel().
    void signalMidiOutLabel(const MappedEvent *event);
    /// Connected to TransportDialog::slotPlaying().
    void signalPlaying(bool checked);
    /// Connected to TransportDialog::slotRecording().
    void signalRecording(bool checked);
    /// Connected to TransportDialog::slotMetronomeActivated().
    void signalMetronomeActivated(bool checked);

private slots:
    void slotCountdownTimerTimeout();

    // Activated by timer to allow a message to be reported to 
    // the user - we use this mechanism so that the user isn't
    // bombarded with dialogs in the event of lots of failures.
    //
    void slotAllowReport() { m_canReport = true; }

    void slotFoundMountPoint(const QString&,
                             unsigned long kBSize,
                             unsigned long kBUsed,
                             unsigned long kBAvail);

    void slotScheduledCompositionMapperReset();
    
private:

    void processAddedSegment(Segment *);
    void processRemovedSegment(Segment *);
    void segmentModified(Segment *);

    void stop2();

    /// Update the MIDI out label on the TransportDialog.
    /*
     * Emits signalMidiOutLabel().
     *
     * ??? rename: updateMIDIOutLabel()
     * ??? Only one caller.  Inline?
     */
    void showVisuals(const MappedEventList &mC);

    /// Apply filtering to a MappedEventList.
    /**
     * Copies events that DO NOT match filter from eventsIn to
     * eventsOut.
     *
     * ??? Only one caller.  Inline?
     */
    void applyFiltering(const MappedEventList &eventsIn,
                        MappedEvent::MappedEventType filter,
                        MappedEventList &eventsOut);

    void resetCompositionMapper();
    void populateCompositionMapper();
    void resetControlBlock();
    void resetMetronomeMapper();
    void resetTempoSegmentMapper();
    void resetTimeSigSegmentMapper();
    void checkRefreshStatus();
    bool shouldWarnForImpreciseTimer();
    
    //--------------- Data members ---------------------------------

    RosegardenDocument    *m_doc;
    CompositionMapper     *m_compositionMapper;
    MetronomeMapper       *m_metronomeMapper;
    TempoSegmentMapper    *m_tempoSegmentMapper;
    TimeSigSegmentMapper  *m_timeSigSegmentMapper;

    std::vector<Segment *> m_addedSegments;
    std::vector<Segment *> m_removedSegments;
    bool m_metronomeNeedsRefresh;

    // statuses
    TransportStatus            m_transportStatus;
    unsigned int               m_soundDriverStatus;

    clock_t                    m_lastRewoundAt;

    /// ??? The CountdownDialog has been disabled.
    CountdownDialog *m_countdownDialog;
    QTimer *m_countdownTimer;

    bool                      m_shownOverrunWarning;

    // Keep a track of elapsed record time with this object
    //
    QTime                     *m_recordTime;

    typedef std::map<Segment *, int> SegmentRefreshMap;
    SegmentRefreshMap m_segments; // map to refresh status id
    SegmentRefreshMap m_triggerSegments;
    unsigned int m_compositionRefreshStatusId;
    bool m_updateRequested;

    // used to schedule a composition mapper reset when the
    // composition end time marker changes this can be caused by a
    // window resize, and since the reset is potentially expensive we
    // want to collapse several following requests into one.
    //QTimer                    *m_compositionMapperResetTimer;

    // Just to make sure we don't bother the user too often
    //
    QTimer                    *m_reportTimer;
    bool                       m_canReport;

    bool                       m_gotDiskSpaceResult;
    unsigned long              m_diskSpaceKBAvail;

    bool                       m_lastLowLatencySwitchSent;

    timeT                      m_lastTransportStartPosition;

    /// Cache of sample rate to avoid locks.
    mutable int m_sampleRate;

    tempoT m_tempo;
};




}

#endif
