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

#ifndef _ROSEGARDEN_SEQUENCER_APP_H_
#define _ROSEGARDEN_SEQUENCER_APP_H_
 
// RosegardenSequencerApp is the sequencer application for Rosegarden.
// It owns a Rosegarden::Sequencer object which wraps the aRTS level
// funtionality.  At this level we deal with comms with the Rosegarden
// GUI application, the high level marshalling of data and main event
// loop of the sequencer.  [rwb]
//
//


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// include files for Qt
#include <qstrlist.h>

// include files for KDE 
#include <kapp.h>
#include <kmainwindow.h>
#include <kaccel.h>

#include "Composition.h"
#include "rosegardendcop.h"
#include "rosegardensequenceriface.h"
#include "MappedComposition.h"
#include "Sequencer.h"
#include "Event.h"

class KURL;
class KRecentFilesAction;

// forward declaration of the RosegardenGUI classes
class RosegardenGUIDoc;
class RosegardenGUIView;

class RosegardenSequencerApp : public KMainWindow,
                               virtual public RosegardenSequencerIface
{
    Q_OBJECT

public:
    RosegardenSequencerApp();
    ~RosegardenSequencerApp();

protected:

public slots:
    virtual void quit();

    // Based on RealTime timestamps
    //
    int play(const Rosegarden::RealTime &position,
             const Rosegarden::RealTime &playLatency,
             const Rosegarden::RealTime &fetchLatency);

    // recording
    int record(const Rosegarden::RealTime &position,
               const Rosegarden::RealTime &playLatency,
               const Rosegarden::RealTime &fetchLatency,
               const int &recordMode);

    // looping
    void setLoop(const Rosegarden::RealTime &loopStart,
                const Rosegarden::RealTime &loopEnd);


    // Play wrapper for DCOP
    //
    virtual int play(const long &timeSec,
                     const long &timeUsec,
                     const long &playLatencySec,
                     const long &playLatencyUSec,
                     const long &fetchLatencySec,
                     const long &fetchLatencyUSec);

    // Record wrapper for DCOP
    //
    virtual int record(const long &timeSec,
                       const long &timeUSec,
                       const long &playLatencySec,
                       const long &playLatencyUSec,
                       const long &fetchLatencySec,
                       const long &fetchLatencyUSec,
                       const int &recordMode);

    
    // Jump to a pointer in the playback (uses longs instead
    // of RealTime for DCOP)
    //
    //
    virtual void jumpTo(const long &posSec, const long &posUsec);

    // Set a loop on the Sequencer
    //
    virtual void setLoop(const long &loopStartSec, const long &loopStartUSec,
                         const long &loopEndSec, const long &loopEndUSec);
 
    // Return the Sound system status (audio/MIDI)
    //
    virtual int getSoundSystemStatus();

    // Add and delete Audio files on the sequencer
    //
    virtual int addAudioFile(const QString &fileName, const int &id);
    virtual int deleteAudioFile(const int &id);

    // Deletes all the audio files and clears down any flapping i/o handles
    //
    virtual void deleteAllAudioFiles();

    // stops the sequencer
    //
    virtual void stop();

public:

    void setStatus(const TransportStatus &status)
            { m_transportStatus = status; }
    TransportStatus getStatus() { return m_transportStatus; }
   
    // Process the first chunk of Sequencer events
    bool startPlaying();

    // Process all subsequent events
    bool keepPlaying();

    // Update internal clock and send GUI position pointer movement
    void updateClocks();

    // Sends status changes up to GUI
    void notifySequencerStatus();

    // These two methods process any pending MIDI or audio
    // and send them up to the gui for storage and display
    //
    void processRecordedMidi();
    void processRecordedAudio();

    // Called during stopped or playing operation to process
    // any pending incoming MIDI events that aren't being
    // recorded (i.e. for display in Transport or on Mixer)
    //
    void processAsynchronousEvents();

    // Are we looping?
    //
    bool isLooping() const { return (m_loopStart != m_loopEnd); }

private:

    // get events whilst handling loop
    //
    Rosegarden::MappedComposition*
        fetchEvents(const Rosegarden::RealTime &start,
                    const Rosegarden::RealTime &end);

    // just get a slice of events between markers
    //
    Rosegarden::MappedComposition* getSlice(const Rosegarden::RealTime &start,
                                            const Rosegarden::RealTime &end);

    Rosegarden::Sequencer *m_sequencer;
    TransportStatus m_transportStatus;

    // Position pointer
    Rosegarden::RealTime m_songPosition;
    Rosegarden::RealTime m_lastFetchSongPosition;

    // Latency - m_fetchLatency - when we should fetch new events and
    //                            spool them onto aRTS
    //
    //         - m_playLatency  - how long we add to all events to make
    //                            sure they play in a synchonised manner
    //                            (i.e. give them a chance to get into aRTS)
    //
    //         - m_readAhead    - how many events we read in one go
    //                            
    //
    // We can throttle these values internally at first, make them
    // user defineable or even auto-throttle them.
    //
    //
    Rosegarden::RealTime m_fetchLatency;
    Rosegarden::RealTime m_playLatency;
    Rosegarden::RealTime m_readAhead;

    Rosegarden::RealTime m_loopStart;
    Rosegarden::RealTime m_loopEnd;

};
 
#endif // _ROSEGARDEN_SEQUENCER_APP_H_
