// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4 v0.2
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

#include "audiomanagerdialog.h"
#include "rosegardendcop.h"

#include "Midi.h"
#include "Sound.h"
#include "Track.h"
#include "MappedComposition.h"
#include "MappedStudio.h"
#include "RealTime.h"


// SequenceManager is a helper class that wraps the sequencing
// functionality at the GUI level.  The sequencer still communicates
// with the RosegardenGUIApp but all calls are passed on directly
// to this class.
//
// Basically this neatens up the source code and provides a
// logical break in the design.
//

class RosegardenGUIDoc;
class QTimer;

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
                                         const RealTime &sliceEnd,
                                         bool firstFetch);

    // Transport controls
    void play();

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
    void insertMetronomeClicks(const timeT &sliceStart, const timeT &sliceEnd);

    // Events coming in
    void processRecordedMidi(const MappedComposition &mC);
    void processAsynchronousMidi(const MappedComposition &mC,
                                 Rosegarden::AudioManagerDialog *aMD);

    // Before playing and recording (throws exceptions)
    //
    void checkSoundDriverStatus();

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
    void sendMappedEvent(Rosegarden::MappedEvent *mE);


    // Update our GUI with latest audio recording information - actual
    // sample file is recorded directly at the sequencer level so this
    // is just for informational purposes.
    //
    void processRecordedAudio(const Rosegarden::RealTime &time);

    // Suspend the sequencer to allow for a safe DCOP call() i.e. one
    // when we don't hang both clients 'cos they're blocking on each
    // other.
    //
    void suspendSequencer(bool value);

    // Send the audio level to VU meters
    //
    void sendAudioLevel(Rosegarden::MappedEvent *mE);

    // Find what has been initialised and what hasn't
    //
    unsigned int getSoundDriverStatus() { return m_soundDriverStatus; }

    // Reset MIDI controllers
    //
    void resetControllers();

    // Send this to the sequencer to update its map
    //
    void sendMappedInstrument(const MappedInstrument &mI);

    // Set a property on a MappedStudio object
    //
    void setMappedProperty(Rosegarden::MappedObjectId id,
                           const Rosegarden::MappedObjectProperty &property,
                           Rosegarden::MappedObjectValue value);

public slots:
    // Empty the m_clearToSend flag
    //
    //void slotClearToSendElapsed();

private:
    Rosegarden::MappedComposition m_mC;
    RosegardenGUIDoc *m_doc;

    // statuses
    TransportStatus m_transportStatus;
    unsigned int    m_soundDriverStatus;

    // pointer to the transport dialog
    RosegardenTransportDialog *m_transport;

    // A hangover from the blocking stop() which we haven't yet
    // removed.
    //
    bool    m_sendStop;

};

}


#endif // _SEQUENCERMANAGER_H_
