// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
  Rosegarden-4 v0.1
  A sequencer and musical notation editor.

  This program is Copyright 2000-2001
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

using std::cout;
using std::cerr;
using std::endl;

static const char *description = I18N_NOOP("RosegardenSequencer");
    
static KCmdLineOptions options[] =
    {
        { "+[File]", I18N_NOOP("file to open"), 0 },
        { 0, 0, 0 }
        // INSERT YOUR COMMANDLINE OPTIONS HERE
    };

int main(int argc, char *argv[])
{

    KAboutData aboutData( "rosegardensequencer",
                          I18N_NOOP("RosegardenSequencer"),
                          VERSION, description, KAboutData::License_GPL,
                          "(c) 2000-2001, Guillaume Laurent, Chris Cannam, Richard Bown");
    aboutData.addAuthor("Guillaume Laurent, Chris Cannam, Richard Bown",0, "glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com");
    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

    KApplication app;
    RosegardenSequencerApp *roseSeq = 0;


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

    while(roseSeq->getStatus() != QUIT)
    {
        // process any pending events (5ms of events)
        //
        app.processEvents(5);

        // Update internal clock and send pointer position
        // change event to GUI - this is the heartbeat of
        // the Sequencer - it doesn't tick over without
        // this call.
        //
        //
        if (roseSeq->getStatus() == PLAYING ||
            roseSeq->getStatus() == RECORDING_MIDI ||
            roseSeq->getStatus() == RECORDING_AUDIO)
        {
            roseSeq->updateClocks();
        }

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
                        }
                        break;

                    case STOPPING:
                        roseSeq->setStatus(STOPPED);
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

        // Pause for breath while we're gnashing this loop (5 milliseconds)
        //
        // See the notes above the loop regarding getting the sequencer
        // to throttle - this could be done automatically or within
        // parameters sent down from the GUI.
        //
        usleep(5000);
    }

    return app.exec();

}
