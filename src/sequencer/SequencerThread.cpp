/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2026 the Rosegarden development team.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "SequencerThread.h"

#include "misc/Debug.h"
#include "base/RealTime.h"
#include "RosegardenSequencer.h"
#include "TransportControl.h"
#include "gui/application/TransportStatus.h"

#include <QElapsedTimer>

namespace Rosegarden
{


void
SequencerThread::run()
{
    RG_DEBUG << "run()";

    RosegardenSequencer &seq = *RosegardenSequencer::getInstance();

    TransportStatus lastSeqStatus = seq.getStatus();

    const RealTime sleepTime = RealTime::fromMilliseconds(10);

    QElapsedTimer timer;
    timer.start();

    bool exiting = false;

    seq.lock();

    while (!exiting) {

        TransportControl::getInstance()->tick();

        bool atLeisure = true;

        //RG_DEBUG << "run(): Sequencer status is " << seq.getStatus();

        if (seq.getStatus() == QUIT) {
            exiting = true;
        }

        // Update internal clock and send pointer position
        // change event to GUI - this is the heartbeat of
        // the Sequencer - it doesn't tick over without
        // this call.
        //
        // Also attempt to send the MIDI clock at this point.
        //
        seq.updateClocks();

        // If the sequencer status has changed...
        if (lastSeqStatus != seq.getStatus()) {
            RG_DEBUG << "run(): Sequencer status changed from " << lastSeqStatus << " to " << seq.getStatus();
            lastSeqStatus = seq.getStatus();

            // Immediately check for another change.
            // We might be in one of the "STARTING" or "STOPPING" states and
            // we need to immediately go to the next state.
            atLeisure = false;
        }

        // Every second
        if (timer.elapsed() > 1000) {
            seq.checkForNewClients();
            timer.restart();
        }

        seq.unlock();

        // permitting synchronised calls from the gui or wherever to
        // be made now

        // If the sequencer status hasn't changed, sleep for a bit
        if (atLeisure) {
            // ??? Could we use a QWaitCondition/QMutex here to reduce the
            //     (upwards of sleepTime) delay between pressing play and
            //     playing?
            seq.sleep(sleepTime);
        }

        seq.lock();
    }

    seq.unlock();
}


}
