/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
  Rosegarden
  A sequencer and musical notation editor.
  Copyright 2000-2018 the Rosegarden development team.
 
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#include "RosegardenSequencer.h"
#include "SequencerThread.h"

#include <iostream>
#include <unistd.h>
#include <sys/time.h>

#include <QDateTime>

#include "base/Profiler.h"
#include "sound/MappedEventList.h"

#include "misc/Debug.h"

namespace Rosegarden
{

void
SequencerThread::run()
{
    SEQUENCER_DEBUG << "SequencerThread::run()";

    RosegardenSequencer &seq = *RosegardenSequencer::getInstance();

    TransportStatus lastSeqStatus = seq.getStatus();

    RealTime sleepTime = RealTime(0, 10000000);

    QTime timer;
    timer.start();

    bool exiting = false;

    seq.lock();

    while (!exiting) {

        bool atLeisure = true;

        //SEQUENCER_DEBUG << "Sequencer status is " << seq.getStatus();

        switch (seq.getStatus()) {

        case QUIT:
            exiting = true;
            break;

        case STARTING_TO_PLAY:
            if (!seq.startPlaying()) {
                // send result failed and stop Sequencer
                seq.setStatus(STOPPING);
            } else {
                seq.setStatus(PLAYING);
            }
            break;

        case PLAYING:
            if (!seq.keepPlaying()) {
                // there's a problem or the piece has
                // finished - so stop playing
                seq.setStatus(STOPPING);
            } else {
                // process any async events
                //
                seq.processAsynchronousEvents();
            }
            break;

        case STARTING_TO_RECORD:
            if (!seq.startPlaying()) {
                seq.setStatus(STOPPING);
            } else {
                seq.setStatus(RECORDING);
            }
            break;

        case RECORDING:
            if (!seq.keepPlaying()) {
                // there's a problem or the piece has
                // finished - so stop playing
                seq.setStatus(STOPPING);
            } else {
                // Now process any incoming MIDI events
                // and return them to the gui
                //
                seq.processRecordedMidi();

                // Now process any incoming audio
                // and return it to the gui
                //
                seq.processRecordedAudio();

                // Still process these so we can send up
                // audio levels as MappedEvents
                //
                // Bug #3542166.  This line can occasionally steal MIDI
                // events that are needed by processRecordedMidi().
                // Need to track down what the above "audio levels" comment
                // means and whether it is a serious issue.  If so, we need
                // to address it in a different way.  This line probably
                // never did anything as by the time it was run,
                // processRecordedMidi() would have cleaned out all the
                // incoming events.
                //seq.processAsynchronousEvents();
            }
            break;

        case STOPPING:
            // There's no call to RosegardenSequencer to actually process the
            // stop, because this arises from a call from the GUI
            // direct to RosegardenSequencer to start with
            seq.setStatus(STOPPED);

            SEQUENCER_DEBUG << "SequencerThread::run() - Stopped";
            break;

        case RECORDING_ARMED:
            SEQUENCER_DEBUG << "SequencerThread::run() - Sequencer can't enter \"RECORDING_ARMED\" state - internal error";
            break;

        case STOPPED:
        default:
            seq.processAsynchronousEvents();
            break;
        }

        // Update internal clock and send pointer position
        // change event to GUI - this is the heartbeat of
        // the Sequencer - it doesn't tick over without
        // this call.
        //
        // Also attempt to send the MIDI clock at this point.
        //
        seq.updateClocks();

        if (lastSeqStatus != seq.getStatus()) {
            SEQUENCER_DEBUG << "Sequencer status changed from " << lastSeqStatus << " to " << seq.getStatus();
            lastSeqStatus = seq.getStatus();
            atLeisure = false;
        }

        if (timer.elapsed() > 3000) {
            seq.checkForNewClients();
            timer.restart();
        }

        seq.unlock();

        // permitting synchronised calls from the gui or wherever to
        // be made now

        // If the sequencer status hasn't changed, sleep for a bit
        if (atLeisure)
            seq.sleep(sleepTime);

        seq.lock();
    }

    seq.unlock();
}

}
