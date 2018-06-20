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

    //
    // Transport controls
    //

    /// returns true if the call actually paused playback
    bool play();

    // We don't call stop() directly - using stopping() and then
    // call stop().
    //
    void stop();

    void stopping();
    void record(bool countIn);

    void setLoop(const timeT &lhs, const timeT &rhs);
    void notifySequencerStatus(TransportStatus status);
    void sendSequencerJump(const RealTime &time);

    // Events coming in
    void processAsynchronousMidi(const MappedEventList &mC,
                                 AudioManagerDialog *aMD);

    // Before playing and recording.  If warnUser is true, show the
    // user a warning dialog if there is a problem with the setup.
    //
    void checkSoundDriverStatus(bool warnUser);

    /**
     * Send program changes and align Instrument lists before playback
     * starts.
     * Also called at document loading (with arg set to true) to reset all instruments
     * (fix for bug 820174)
     *
     * @arg forceProgramChanges if true, always send program changes even if the instrument is
     * set not to send any.
     */
    void preparePlayback(bool forceProgramChanges = false);

    /// Check and set sequencer status
    void setTransportStatus(const TransportStatus &status);
    TransportStatus getTransportStatus() const { return m_transportStatus; }

    /**
     * Suspend the sequencer to allow for a safe DCOP call() i.e. one
     * when we don't hang both clients 'cos they're blocking on each
     * other.
     */
    void suspendSequencer(bool value);

    /// Find what has been initialised and what hasn't
    unsigned int getSoundDriverStatus() { return m_soundDriverStatus; }

    /// Reset MIDI controllers
    void resetControllers();

    /// Reset MIDI network
    void resetMidiNetwork();

    /// Reinitialise the studio
    void reinitialiseSequencerStudio();

    /// Send JACK and MMC transport control statuses
    void sendTransportControlStatuses();

    /// Send all note offs and resets to MIDI devices
    void panic();

    /// Set tempo (also notifies StudioControl and TransportDialog)
    void setTempo(const tempoT tempo);

    /// Send an MC to the view
    void showVisuals(const MappedEventList &mC);

    /// Apply in-situ filtering to a MappedEventList
    /**
     * ??? Return by value?  COPY.
     */
    MappedEventList
        applyFiltering(const MappedEventList &mC,
                       MappedEvent::MappedEventType filter);

    CountdownDialog* getCountdownDialog() { return m_countdownDialog; }

    // Return a new metaiterator on the current composition (suitable
    // for MidiFile)
    MappedBufMetaIterator *makeTempMetaiterator(void);
    //
    // CompositionObserver interface
    //
    virtual void segmentAdded              (const Composition*, Segment*);
    virtual void segmentRemoved            (const Composition*, Segment*);
    virtual void segmentRepeatChanged      (const Composition*, Segment*, bool);
    virtual void segmentRepeatEndChanged   (const Composition*, Segment*, timeT);
    virtual void segmentEventsTimingChanged(const Composition*, Segment *, timeT delay, RealTime rtDelay);
    virtual void segmentTransposeChanged   (const Composition*, Segment *, int transpose);
    virtual void segmentTrackChanged       (const Composition*, Segment *, TrackId id);
    virtual void segmentEndMarkerChanged   (const Composition*, Segment *, bool);
    virtual void endMarkerTimeChanged      (const Composition*, bool shorten);
    virtual void tracksAdded               (const Composition*, std::vector<TrackId> &/*trackIds*/);
    virtual void trackChanged              (const Composition*, Track*);
    virtual void tracksDeleted             (const Composition*, std::vector<TrackId> &/*trackIds*/);
    virtual void timeSignatureChanged      (const Composition*);
    virtual void metronomeChanged          (const Composition*);
    virtual void selectedTrackChanged      (const Composition *);
    virtual void tempoChanged              (const Composition*);

    void processAddedSegment(Segment*);
    void processRemovedSegment(Segment*);
    void segmentModified(Segment*);
    void segmentInstrumentChanged(Segment *s);

    virtual bool event(QEvent *e);

    /// for the gui to call to indicate that the metronome needs to be remapped
    void metronomeChanged(InstrumentId id, bool regenerateTicks);

    /// for the gui to call to indicate that a MIDI filter needs to be remapped
    void filtersChanged(MidiFilter thruFilter,
                        MidiFilter recordFilter);

    int getSampleRate(); // may return 0 if sequencer uncontactable

public slots:

    void update();
    void rewind();
    void fastforward();
    void rewindToBeginning();
    void fastForwardToEnd();

signals:
    void signalSelectProgramNoSend(int, int, int);

    void insertableNoteOnReceived(int pitch, int velocity);
    void insertableNoteOffReceived(int pitch, int velocity);
    void controllerDeviceEventReceived(MappedEvent *ev);

    /// signal RosegardenMainWindow to display a warning on the WarningWidget
    void sendWarning(int type, QString text, QString informativeText);

    /// signal GUI changes to the TransportDialog
    void signalTempoChanged(tempoT);
    void signalMidiInLabel(const MappedEvent *event);
    void signalMidiOutLabel(const MappedEvent *event);
    void signalPlaying(bool checked);
    void signalRecording(bool checked);
    void signalMetronomeActivated(bool checked);

protected slots:
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
    
protected:

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

    CountdownDialog           *m_countdownDialog;
    QTimer                    *m_countdownTimer;

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

    int                        m_sampleRate;

    tempoT m_tempo;
};




}

#endif
