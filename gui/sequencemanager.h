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
#include "rosegardenguidoc.h"
#include "MappedComposition.h"
#include "rosegardentransportdialog.h"
#include "RealTime.h"
#include "Sound.h"
#include "Track.h"
#include "Midi.h"


namespace Rosegarden
{

class SequenceManager
{
public:
    SequenceManager(RosegardenGUIDoc *doc,
                     RosegardenTransportDialog *transport);
    ~SequenceManager();

    MappedComposition* getSequencerSlice(const RealTime &sliceStart,
                                         const RealTime &sliceEnd);

    void play();
    void stop();
    void rewind();
    void fastforward();
    void record();
    void rewindToBeginning();
    void fastForwardToEnd();

    void setPlayStartTime(const timeT &time);

    void notifySequencerStatus(TransportStatus status);
    void sendSequencerJump(const RealTime &time);

    void processRecordedMidi(const MappedComposition &mC);
    void processAsynchronousMidi(const MappedComposition &mC);

    void setLoop(const timeT &lhs, const timeT &rhs);

    void checkSoundSystemStatus();

    void insertMetronomeClicks(const timeT &sliceStart, const timeT &sliceEnd);

    // Upwards interfaces
    //
    void setPointerPosition(timeT time);
    void setPointerPosition(RealTime realTime);
    bool launchSequencer();
    void sequencerExited(KProcess*);

private:

    RealTime m_playLatency;
    RealTime m_fetchLatency;
    RealTime m_readAhead;

    TransportStatus m_transportStatus;
    SoundSystemStatus m_soundSystemStatus;

    Rosegarden::MappedComposition m_mC;
    RosegardenGUIDoc *m_doc;

    // Metronome details
    //
    InstrumentId m_metronomeInstrument;
    TrackId      m_metronomeTrack;
    MidiByte     m_metronomePitch;
    MidiByte     m_metronomeBarVelocity;
    MidiByte     m_metronomeBeatVelocity;
    RealTime     m_metronomeDuration;

    KProcess* m_sequencerProcess;

    // pointer to the transport dialog
    RosegardenTransportDialog *m_transport;

};

}


#endif // _SEQUENCERMANAGER_H_
