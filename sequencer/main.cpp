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

#include <signal.h>
#include <iostream>
#include <unistd.h>
#include <sys/time.h>

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <dcopclient.h>

#include "Profiler.h"
#include "MappedComposition.h"
#include "rosegardendcop.h"
#include "rosegardensequencer.h"
#include "rosedebug.h"

#include "config.h"

using std::cout;
using std::cerr;
using std::endl;

static const char *description = I18N_NOOP("RosegardenSequencer");
static RosegardenSequencerApp *roseSeq = 0;

static KCmdLineOptions options[] =
    {
//        { "+[File]", I18N_NOOP("file to open"), 0 },
        // INSERT YOUR COMMANDLINE OPTIONS HERE
        { "+[playback_1 playback_2 capture_1 capture_2]",
            I18N_NOOP("JACK playback and capture ports"), 0 },
        { 0, 0, 0 }
    };

static void
cleanup()
{
    RosegardenSequencerApp *tmpRoseSeq = roseSeq;

    if (roseSeq) {
	roseSeq = 0;
        delete tmpRoseSeq;
    }
}    

static bool _mainThread = false; // set true later
static bool _exiting = false;
static sigset_t _signals;

static void
signalHandler(int /*sig*/)
{
    if (_mainThread) {
	_exiting = true;
	cleanup();
	exit(0);
    }
}

int main(int argc, char *argv[])
{
    // Block signals during startup, so that child threads (inheriting
    // this mask) ignore them; then after startup we can unblock them
    // for this thread only.  This trick picked up from the jackd code.
    sigemptyset (&_signals);
    sigaddset(&_signals, SIGHUP);
    sigaddset(&_signals, SIGINT);
    sigaddset(&_signals, SIGQUIT);
    sigaddset(&_signals, SIGPIPE);
    sigaddset(&_signals, SIGTERM);
    sigaddset(&_signals, SIGUSR1);
    sigaddset(&_signals, SIGUSR2);
    pthread_sigmask(SIG_BLOCK, &_signals, 0);

    KAboutData aboutData( "rosegardensequencer",
                          I18N_NOOP("RosegardenSequencer"),
                          VERSION, description, KAboutData::License_GPL,
                          "(c) 2000-2004, Guillaume Laurent, Chris Cannam, Richard Bown");
    aboutData.addAuthor("Guillaume Laurent, Chris Cannam, Richard Bown",0, "glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com");
    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

    // Parse cmd line args
    //
    /*KCmdLineArgs *args =*/ KCmdLineArgs::parsedArgs();
    KApplication app;

    if (app.isRestored())
    {
	app.quit(); // don't do session restore -- GUI will start a sequencer
    }
    else
    {
        roseSeq = new RosegardenSequencerApp;
    }

    QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

    app.disableSessionManagement(); // we don't want to be
                                    // saved/restored by session
                                    // management, only run by the GUI

    // Started OK
    //
    SEQUENCER_DEBUG << "RosegardenSequencer - started OK" << endl;
	
    // Register signal handlers and unblock signals
    //
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGHUP, signalHandler);
    signal(SIGQUIT, signalHandler);
    pthread_sigmask(SIG_UNBLOCK, &_signals, 0);

    // Now we can enter our specialised event loop.
    // For each pass through we wait for some pending
    // events.  We check status on the way through and
    // act accordingly.  DCOP events fire back and
    // forth processed in the event loop changing 
    // state and hopefully controlling and providing
    // feedback.  We also put in some sleep time to
    // make sure the loop doesn't eat up all the
    // processor - we're not in that much of a rush!
    //
    TransportStatus lastSeqStatus = roseSeq->getStatus();

    _mainThread = true;

    Rosegarden::RealTime sleepTime = Rosegarden::RealTime(0, 10000000);

    while (!_exiting && roseSeq && roseSeq->getStatus() != QUIT)
    {
	bool atLeisure = true;

	switch(roseSeq->getStatus())
	{
	case STARTING_TO_PLAY:
	    if (!roseSeq->startPlaying())
	    {
		// send result failed and stop Sequencer
		roseSeq->setStatus(STOPPING);
	    }
	    else
	    {
		roseSeq->setStatus(PLAYING);
	    }
	    break;

	case PLAYING:
	    if (!roseSeq->keepPlaying())
	    {
		// there's a problem or the piece has
		// finished - so stop playing
		roseSeq->setStatus(STOPPING);
	    }
	    else
	    {
		// process any async events
		//
		roseSeq->processAsynchronousEvents();
	    }
	    break;

	case STARTING_TO_RECORD_MIDI:
	    if (!roseSeq->startPlaying())
	    {
		roseSeq->setStatus(STOPPING);
	    }
	    else
	    {
		roseSeq->setStatus(RECORDING_MIDI);
	    }
	    break;

	case STARTING_TO_RECORD_AUDIO:
	    if (!roseSeq->startPlaying())
	    {
		roseSeq->setStatus(STOPPING);
	    }
	    else
	    {
		roseSeq->setStatus(RECORDING_AUDIO);
	    }
	    break;

	case RECORDING_MIDI:
	    if (!roseSeq->keepPlaying())
	    {
		// there's a problem or the piece has
		// finished - so stop playing
		roseSeq->setStatus(STOPPING);
	    }
	    else
	    {
		// Now process any incoming MIDI events
		// and return them to the gui
		//
		roseSeq->processRecordedMidi();
	    }
	    break;

	case RECORDING_AUDIO:
	    if (!roseSeq->keepPlaying())
	    {
		// there's a problem or the piece has
		// finished - so stop playing
		roseSeq->setStatus(STOPPING);
	    }
	    else
	    {
		// Now process any incoming audio
		// and return it to the gui
		//
		roseSeq->processRecordedAudio();

		// Still process these so we can send up
		// audio levels as MappedEvents
		//
		roseSeq->processAsynchronousEvents();
	    }
	    break;

	case STOPPING:
	    // There's no call to roseSeq to actually process the
	    // stop, because this arises from a DCOP call from the GUI
	    // direct to roseSeq to start with
	    roseSeq->setStatus(STOPPED);
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
	    roseSeq->processAsynchronousEvents();
	    break;
	}

        // Update internal clock and send pointer position
        // change event to GUI - this is the heartbeat of
        // the Sequencer - it doesn't tick over without
        // this call.
        //
        // Also attempt to send the MIDI clock at this point.
        //
        roseSeq->updateClocks();

	// we want to process transport changes immediately
        if (roseSeq->checkExternalTransport()) {
	    atLeisure = false;
	} else if (lastSeqStatus != roseSeq->getStatus()) {
	    SEQUENCER_DEBUG << "Sequencer status changed from " << lastSeqStatus << " to " << roseSeq->getStatus() << endl;
            roseSeq->notifySequencerStatus();
	    lastSeqStatus = roseSeq->getStatus();
	    atLeisure = false;
	}

	app.processEvents();
	if (atLeisure) roseSeq->sleep(sleepTime);
    }

    int rv = app.exec();
    cleanup();
    SEQUENCER_DEBUG << "Toodle-pip." << endl;
    return rv;
}
