// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2004
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

#ifndef _SEQUENCERMANAGER_H_
#define _SEQUENCERMANAGER_H_

#include <kapp.h>
#include <kprocess.h>

#include "audiomanagerdialog.h"
#include "rosegardendcop.h"

#include "Midi.h"
#include "Sound.h"
#include "Track.h"
#include "MappedComposition.h"
#include "MappedCommon.h"
#include "MappedStudio.h"
#include "RealTime.h"

#include <ctime>


// SequenceManager is a helper class that wraps the sequencing
// functionality at the GUI level.  The sequencer still communicates
// with the RosegardenGUIApp but all calls are passed on directly
// to this class.
//
// Basically this neatens up the source code and provides a
// logical break in the design.
//

class RosegardenGUIDoc;
class CountdownDialog;
class QTimer;
class QTime;

class SegmentMmapper;
class CompositionMmapper;
class ControlBlockMmapper;
class MetronomeMmapper;
class TempoSegmentMmapper;
class TimeSigSegmentMmapper;
class SequencerMapper;

namespace Rosegarden
{

class AudioPluginManager;
class RosegardenTransportDialog;

class SequenceManager : public QObject, public CompositionObserver
{
    Q_OBJECT
public:
    SequenceManager(RosegardenGUIDoc *doc,
		    RosegardenTransportDialog *transport);
    ~SequenceManager();

    /**
     * Replaces the internal document
     */
    void setDocument(RosegardenGUIDoc*);

    /**
     * Return the current internal document
     */
    RosegardenGUIDoc* getDocument();

    //
    // Transport controls
    //

    /// returns true if the call actually paused playback
    bool  play();

    // We don't call stop() directly - using stopping() and then
    // call stop().
    //
    void stop();

    void stopping();
    void rewind();
    void fastforward();
    void record(bool countIn);
    void rewindToBeginning();
    void fastForwardToEnd();

    void setPlayStartTime(const timeT &time);
    void setLoop(const timeT &lhs, const timeT &rhs);
    void notifySequencerStatus(TransportStatus status);
    void sendSequencerJump(const RealTime &time);

    // Events coming in
    void processAsynchronousMidi(const MappedComposition &mC,
                                 Rosegarden::AudioManagerDialog *aMD);

    // Before playing and recording (throws exceptions)
    //
    void checkSoundDriverStatus();

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

    /// Send the audio level to VU meters
    void sendAudioLevel(Rosegarden::MappedEvent *mE);

    /// Find what has been initialised and what hasn't
    unsigned int getSoundDriverStatus() { return m_soundDriverStatus; }

    /// Reset MIDI controllers
    void resetControllers();

    /// Reset MIDI network
    void resetMidiNetwork();

    /**
     * Get the plugins that are available at the sequencer and put
     * them in the local pluginmanager
     */
    void getSequencerPlugins(Rosegarden::AudioPluginManager *);


    /// Reinitialise the studio
    void reinitialiseSequencerStudio();

    /// Send JACK and MMC transport control statuses
    void sendTransportControlStatuses();

    /// Send all note offs and resets to MIDI devices
    void panic();

    /// Send an MC to the view
    void showVisuals(const Rosegarden::MappedComposition &mC);

    /// Apply in-situ filtering to a MappedComposition
    Rosegarden::MappedComposition
        applyFiltering(const Rosegarden::MappedComposition &mC,
                       Rosegarden::MappedEvent::MappedEventType filter);

    CountdownDialog* getCountdownDialog() { return m_countdownDialog; }


    //
    // CompositionObserver interface
    //
    virtual void segmentAdded              (const Composition*, Segment*);
    virtual void segmentRemoved            (const Composition*, Segment*);
    virtual void segmentRepeatChanged      (const Composition*, Segment*, bool);
    virtual void segmentEventsTimingChanged(const Composition*, Segment *, timeT delay, RealTime rtDelay);
    virtual void segmentTransposeChanged   (const Composition*, Segment *, int transpose);
    virtual void segmentTrackChanged       (const Composition*, Segment *, TrackId id);
    virtual void endMarkerTimeChanged      (const Composition*, bool shorten);
    virtual void trackChanged              (const Composition*, Track*);
    virtual void timeSignatureChanged      (const Composition*);
    virtual void metronomeChanged          (const Composition*);
    virtual void soloChanged               (const Composition*, bool solo, TrackId selectedTrack);
    virtual void tempoChanged              (const Composition*);
    virtual void compositionDeleted        (const Composition*);

    void processAddedSegment(Segment*);
    void processRemovedSegment(Segment*);
    void segmentModified(Segment*);

    virtual bool event(QEvent *e);

    /// for the gui to call to indicate that the metronome needs to be remapped
    void metronomeChanged(Rosegarden::InstrumentId id, bool regenerateTicks);

    /// for the gui to call to indicate that a MIDI filter needs to be remapped
    void filtersChanged(Rosegarden::MidiFilter thruFilter,
			Rosegarden::MidiFilter recordFilter);

    /// Return the current sequencer memory mapped file
    SequencerMapper* getSequencerMapper() { return m_sequencerMapper; }

    /// Ensure that the sequencer file is mapped
    void mapSequencer();
    
public slots:

    void update();

signals:
    void setProgress(int);
    void incrementProgress(int);

    void insertableNoteOnReceived(int pitch, int velocity);
    void insertableNoteOffReceived(int pitch, int velocity);
    
protected slots:
    void slotCountdownCancelled();
    void slotCountdownTimerTimeout();
    void slotCountdownStop();

    // Activated by timer to allow a message to be reported to 
    // the user - we use this mechanism so that the user isn't
    // bombarded with dialogs in the event of lots of failures.
    //
    void slotAllowReport() { m_canReport = true; }

    void slotFoundMountPoint(const QString&,
                             unsigned long kBSize,
                             unsigned long kBUsed,
                             unsigned long kBAvail);
protected:

    void resetCompositionMmapper();
    void resetControlBlockMmapper();
    void resetMetronomeMmapper();
    void resetTempoSegmentMmapper();
    void resetTimeSigSegmentMmapper();
    void checkRefreshStatus();
    
    //--------------- Data members ---------------------------------

    Rosegarden::MappedComposition  m_mC;
    RosegardenGUIDoc              *m_doc;
    CompositionMmapper            *m_compositionMmapper;
    ControlBlockMmapper           *m_controlBlockMmapper;
    MetronomeMmapper              *m_metronomeMmapper;
    TempoSegmentMmapper           *m_tempoSegmentMmapper;
    TimeSigSegmentMmapper         *m_timeSigSegmentMmapper;

    std::vector<Segment*> m_addedSegments;
    std::vector<Segment*> m_removedSegments;
    bool m_metronomeNeedsRefresh;

    // statuses
    TransportStatus            m_transportStatus;
    unsigned int               m_soundDriverStatus;

    // pointer to the transport dialog
    RosegardenTransportDialog *m_transport;

    RealTime                   m_playbackAudioLatency;

    clock_t                    m_lastRewoundAt;

    CountdownDialog           *m_countdownDialog;
    QTimer                    *m_countdownTimer;

    bool                      m_shownOverrunWarning;

    // Keep a track of elapsed record time with this object
    //
    QTime                     *m_recordTime;

    typedef std::map<Rosegarden::Segment *, int> SegmentRefreshMap;
    SegmentRefreshMap m_segments; // map to refresh status id
    SegmentRefreshMap m_triggerSegments;
    unsigned int m_compositionRefreshStatusId;
    bool m_updateRequested;

    // Information that the sequencer is providing to us - for the moment
    // it's only the position pointer.
    //
    SequencerMapper          *m_sequencerMapper;

    // Just to make sure we don't bother the user too often
    //
    QTimer                    *m_reportTimer;
    bool                       m_canReport;

    bool                       m_gotDiskSpaceResult;
    unsigned long              m_diskSpaceKBAvail;
};

}


#endif // _SEQUENCERMANAGER_H_
