/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
  Rosegarden
  A sequencer and musical notation editor.
  Copyright 2000-2008 the Rosegarden development team.
 
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

#include <klocale.h>

#include <qdatetime.h>

#include "base/Profiler.h"
#include "sound/MappedComposition.h"

#include "misc/Debug.h"

using std::cout;
using std::cerr;
using std::endl;

namespace Rosegarden
{

void
SequencerThread::run()
{
    RosegardenSequencer &seq = *RosegardenSequencer::getInstance();

    TransportStatus lastSeqStatus = seq.getStatus();

    RealTime sleepTime = RealTime(0, 10000000);

    QTime timer;

    bool exiting = false;

    seq.lock();

    while (!exiting) {

        bool atLeisure = true;

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
                seq.processAsynchronousEvents();
            }
            break;

        case STOPPING:
            // There's no call to roseSeq to actually process the
            // stop, because this arises from a call from the GUI
            // direct to roseSeq to start with
            seq.setStatus(STOPPED);
            SEQUENCER_DEBUG << "RosegardenSequencer - Stopped" << endl;
            break;

        case RECORDING_ARMED:
            SEQUENCER_DEBUG << "RosegardenSequencer - "
			    << "Sequencer can't enter \""
			    << "RECORDING_ARMED\" state - "
			    << "internal error"
			    << endl;
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

        // we want to process transport changes immediately
/*!!!
        if (seq.checkExternalTransport()) {
            atLeisure = false;
        } else */
        if (lastSeqStatus != seq.getStatus()) {
            SEQUENCER_DEBUG << "Sequencer status changed from " << lastSeqStatus << " to " << seq.getStatus() << endl;
//!!!            seq.notifySequencerStatus();
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

	if (atLeisure)
            seq.sleep(sleepTime);

	seq.lock();
    }

    seq.unlock();
}

}
