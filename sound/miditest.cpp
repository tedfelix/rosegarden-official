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

// TEST APPLICATION - this is only used for testing out
// bits and bobs of the sound system from time to time.
//

#include <alsa/asoundlib.h>
#include <alsa/seq_event.h>

#include <iostream>
#include "AlsaDriver.h"
#include "ArtsDriver.h"

#include <unistd.h>

using std::vector;
using std::endl;
using std::cout;
using Rosegarden::AlsaDriver;
using Rosegarden::ArtsDriver;

int
main(int /*argc*/, char ** /*argv*/)
{
    /*
    ArtsDriver *arts = new ArtsDriver();
    arts->initialiseMidi();
    arts->initialiseAudio();
    Rosegarden::MappedComposition *mC;

    while (true)
    {
       mC = arts->getMappedComposition(Rosegarden::RealTime(0, 40));
       cout << "COMPOSITION = " << mC->size() << endl;
       sleep(1);
    }

    */
    
    int destclient = 65;
    int destport = 0;

    //AlsaDriver *alsaDriver = new AlsaDriver();
    //ArtsDriver *artsDriver = new ArtsDriver();
    snd_seq_t *seq;

    snd_seq_open(&seq,"hw",SND_SEQ_OPEN_DUPLEX,0);
    snd_seq_nonblock(seq,0);
    int client=snd_seq_client_id(seq);
    int port = snd_seq_create_simple_port(seq, "RG test port",
                                          SND_SEQ_PORT_CAP_READ |
                 SND_SEQ_PORT_CAP_WRITE,SND_SEQ_PORT_TYPE_MIDI_GENERIC);

    int queue=snd_seq_alloc_queue(seq);
    //snd_seq_connect_to() and snd_seq_connect_from();
    if(snd_seq_connect_to(seq, port, destclient, destport) < 0)
    {
        std::cerr << "Can't connect to output port" << std::endl;
        exit(1);
    }

    /*
    snd_seq_set_client_pool_output(seq, 2);
    snd_seq_set_client_pool_input(seq, 20);
    snd_seq_set_client_pool_output_room(seq, 20);

    // tempo
    //
    snd_seq_queue_tempo_t *tempo;
    snd_seq_queue_tempo_alloca(&tempo);
    memset(tempo, 0, snd_seq_queue_tempo_sizeof());
    snd_seq_queue_tempo_set_ppq(tempo, 960);
    snd_seq_queue_tempo_set_tempo(tempo, 60*1000000 / 120);
    if (snd_seq_set_queue_tempo(seq, queue, tempo) < 0)
    {
        std::cerr << "Can't set tempo" << std::endl;
        exit(1);
    }
    */


    // start the queue
    //
    if (snd_seq_start_queue(seq,queue,NULL) < 0)
    {
        std::cerr << "Can't start queue" << endl;
        exit(1);
    }

    snd_seq_drain_output(seq);

    int i = 1;

    snd_seq_event_t *event= new snd_seq_event_t();
    snd_seq_ev_clear(event);
    snd_seq_real_time_t time = {0, 0};

    snd_seq_ev_set_source(event, port);
    snd_seq_ev_set_dest(event,
                        destclient,
                        destport);
    snd_seq_ev_schedule_real(event, queue, 0, &time);

    snd_seq_ev_set_pgmchange(event, 0, 34);
    snd_seq_event_output(seq, event);

    while (true)
    {
        snd_seq_ev_set_note(event, 0, 70, 120, 500);
        snd_seq_event_output(seq, event);
        snd_seq_drain_output(seq);

        cout << "PENDING EVENTS = " << snd_seq_event_output_pending(seq)
             << endl;

        sleep(1);
        i++;

        // drain
    }
}
