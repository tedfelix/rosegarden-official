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


#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <dcopclient.h>
#include <iostream>
#include <unistd.h>

#include "rosegardensequencer.h"
#include <MappedComposition.h>
#include "rosegardendcop.h"

#include <signal.h>

using std::cout;
using std::cerr;
using std::endl;

static const char *description = I18N_NOOP("RosegardenSequencer");
static RosegardenSequencerApp *roseSeq = 0;
    
static KCmdLineOptions options[] =
    {
        { "+[File]", I18N_NOOP("file to open"), 0 },
        { 0, 0, 0 }
        // INSERT YOUR COMMANDLINE OPTIONS HERE
    };

static void
signalHandler(int /*sig*/)
{
    if (roseSeq)
        delete roseSeq;

    exit(0);
}

int main(int argc, char *argv[])
{

    KAboutData aboutData( "rosegardensequencer",
                          I18N_NOOP("RosegardenSequencer"),
                          VERSION, description, KAboutData::License_GPL,
                          "(c) 2000-2002, Guillaume Laurent, Chris Cannam, Richard Bown");
    aboutData.addAuthor("Guillaume Laurent, Chris Cannam, Richard Bown",0, "glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com");
    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

    KApplication app;


    if (argc > 1)
    {
        std::cerr
          << "RosegardenSequencer - doesn't accept any command line arguments"
          << std::endl
          << "RosegardenSequencer - exiting"
          << std::endl;

        exit(1);
    }

    if (app.isRestored())
    {
        RESTORE(RosegardenSequencerApp);
    }
    else
    {
        roseSeq = new RosegardenSequencerApp();

        // we don't show() the sequencer application as we're just taking
        // advantage of DCOP/KApplication and there's nothing to show().

        KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
        
        if (args->count())
        {
            //rosegardensequencer->openDocumentFile(args->arg(0));
        }
        else
        {
            // rosegardensequencer->openDocumentFile();
        }

        args->clear();
    }

    QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));
    //app.dcopClient()->setDefaultObject("RosegardenGUIIface");

    // Started OK
    //
    cout << "RosegardenSequencer - started OK" << endl;

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
    // Soon perhaps we'll put some throttling into
    // the loop sleep time(s) to allow for dropped
    // notes etc. .. ?
    //
    //
    TransportStatus lastSeqStatus = roseSeq->getStatus();

    // Register signal handler
    //
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGHUP, signalHandler);
    signal(SIGQUIT, signalHandler);

    // sleep time is 5000 microseconds
    //
    const int sequencerSleep = 5000;
    //int sendAliveCount = 0;

    while(roseSeq && roseSeq->getStatus() != QUIT)
    {
        // process any pending events (5ms of events)
        //
        app.processEvents(5);


        // Unset this flag - only gets set by call()'d operations
        // in the next section.  After which if this flag is set
        // we can tell the gui to send back down to us in safety.
        //
        roseSeq->setClearToSend(false);

        // Do the main conditional loop
        //
        if(roseSeq)
        {
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
                    roseSeq->setStatus(STOPPED);
                    break;

                case RECORDING_ARMED:
                    std::cerr << "RosegardenSequencer - "
                              << "Sequencer can't enter \""
                              << "RECORDING_ARMED\" state - internal error"
                              << std::endl;
                    break;

                case STOPPED:
                default:
                    roseSeq->processAsynchronousEvents();
                    break;
            }
        }

        if (lastSeqStatus != roseSeq->getStatus())
            roseSeq->notifySequencerStatus();

        lastSeqStatus = roseSeq->getStatus();

        // Update internal clock and send pointer position
        // change event to GUI - this is the heartbeat of
        // the Sequencer - it doesn't tick over without
        // this call.
        //
        // At the updateClocks() point any pending stop() from
        // the gui is processed.  The updateClocks() will 
        // make the gui check for a stop request and if so
        // immediately call this process back - we position
        // this call just before the sleep period deliberately
        // to catch this process. [rwb 7/02]
        //
        //
        if (roseSeq->getStatus() == PLAYING ||
            roseSeq->getStatus() == RECORDING_MIDI ||
            roseSeq->getStatus() == RECORDING_AUDIO)
        {
            roseSeq->updateClocks(roseSeq->clearToSend());
        }

        // Pause for breath while we're gnashing this loop (5 milliseconds)
        //
        // See the notes above the loop regarding getting the sequencer
        // to throttle - this could be done automatically or within
        // parameters sent down from the GUI.
        //
        usleep(sequencerSleep);

        /*
        // While we're sending the "alive" signal
        //
        if (roseSeq->sendAlive())
        {
           // increment counter
           sendAliveCount += sequencerSleep;

           // if we've waited a second then try (we're also processing
           // 5ms of Qt events don't forget)
           //
           if (sendAliveCount > 500000)
           {
                // send the alive signal
                roseSeq->sequencerAlive();

                // and reset counter
                sendAliveCount = 0;
           }
        }
        */
    }


    return app.exec();

}
