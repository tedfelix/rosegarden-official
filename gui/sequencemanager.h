// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
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

#ifndef _SEQUENCERMANAGER_H_
#define _SEQUENCERMANAGER_H_

#include <kapp.h>
#include <kprocess.h>

#include "rosegardendcop.h"
#include "MappedComposition.h"
#include "RealTime.h"
#include "Sound.h"
#include "Track.h"
#include "Midi.h"


// SequenceManager is a helper class that wraps the sequencing
// functionality at the GUI level.  The sequencer still communicates
// with the RosegardenGUIApp but all calls are passed on directly
// to this class.
//
// Basically this neatens up the source code and provides a
// logical break in the design.
//

class RosegardenGUIDoc;

namespace Rosegarden
{

class RosegardenTransportDialog;


class SequenceManager : public QObject
{
    Q_OBJECT
public:
    SequenceManager(RosegardenGUIDoc *doc,
                     RosegardenTransportDialog *transport);
    ~SequenceManager();

    // Called from the sequencer - gets a slice of events
    //
    MappedComposition* getSequencerSlice(const RealTime &sliceStart,
                                         const RealTime &sliceEnd);

    // Transport controls
    void play();
    void stop();
    void rewind();
    void fastforward();
    void record();
    void rewindToBeginning();
    void fastForwardToEnd();

    void setPlayStartTime(const timeT &time);
    void setLoop(const timeT &lhs, const timeT &rhs);
    void notifySequencerStatus(TransportStatus status);
    void sendSequencerJump(const RealTime &time);
    void insertMetronomeClicks(const timeT &sliceStart, const timeT &sliceEnd);

    // Events coming in
    void processRecordedMidi(const MappedComposition &mC);
    void processAsynchronousMidi(const MappedComposition &mC);

    // Before playing and recording (throws exceptions)
    //
    void checkSoundSystemStatus();

    // Send program changes and align Instrument lists before playback
    // starts.
    //
    void preparePlayback();

    // Check and set sequencer status
    void setTransportStatus(const TransportStatus &status);
    TransportStatus getTransportStatus() const { return m_transportStatus; }

    // For immediate processing at the other end - use this method
    //
    void sendMappedComposition(const Rosegarden::MappedComposition &mC);

    void sendMidiController(Rosegarden::InstrumentId id,
                            Rosegarden::MidiByte control,
                            Rosegarden::MidiByte value);

private:

    Rosegarden::MappedComposition m_mC;
    RosegardenGUIDoc *m_doc;

    // Latencies
    RealTime m_playLatency;
    RealTime m_fetchLatency;
    RealTime m_readAhead;

    // statuses
    TransportStatus m_transportStatus;
    SoundSystemStatus m_soundSystemStatus;

    // Metronome details
    //
    InstrumentId m_metronomeInstrument;
    TrackId      m_metronomeTrack;
    MidiByte     m_metronomePitch;
    MidiByte     m_metronomeBarVelocity;
    MidiByte     m_metronomeBeatVelocity;
    RealTime     m_metronomeDuration;

    // pointer to the transport dialog
    RosegardenTransportDialog *m_transport;

};

}


#endif // _SEQUENCERMANAGER_H_
