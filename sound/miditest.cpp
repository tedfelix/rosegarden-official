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

// TEST APPLICATION - this is only used for testing out
// bits and bobs of the sound system from time to time.
//

#include <iostream>
#include "AlsaDriver.h"
#include "ArtsDriver.h"

using std::vector;
using std::endl;
using std::cout;
using Rosegarden::AlsaDriver;
using Rosegarden::ArtsDriver;

int
main(int /*argc*/, char ** /*argv*/)
{
    AlsaDriver *alsaDriver = new AlsaDriver();
    ArtsDriver *artsDriver = new ArtsDriver();

    /*
    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;
    snd_seq_system_info_t *sysinfo;
    int  client;
    int  port;
    int  err;
    unsigned int cap;
    snd_seq_t *handle;

    err = snd_seq_open(&handle,
                       "hw",   // why hw always?
                       SND_SEQ_OPEN_INPUT,
                       SND_SEQ_NONBLOCK);

    if (err < 0)
    {
        std::cout << "Could not open sequencer - " << snd_strerror(errno)
                  << std::endl;
    }

    //snd_seq_drop_output(handle);

    snd_seq_client_info_alloca(&cinfo);
    snd_seq_client_info_set_client(cinfo, -1);

    while (snd_seq_query_next_client(handle, cinfo) >= 0)
    {
        client = snd_seq_client_info_get_client(cinfo);
        snd_seq_port_info_alloca(&pinfo);
        snd_seq_port_info_set_client(pinfo, client);

        snd_seq_port_info_set_port(pinfo, -1);
        while (snd_seq_query_next_port(handle, pinfo) >= 0)
        {
            int cap;
            cap = (SND_SEQ_PORT_CAP_SUBS_WRITE|SND_SEQ_PORT_CAP_WRITE);

            if ((snd_seq_port_info_get_capability(pinfo) & cap) == cap)
            {
                std::cout << snd_seq_port_info_get_client(pinfo) << " : "
                          << snd_seq_port_info_get_port(pinfo) << " : "
                          << snd_seq_client_info_get_name(cinfo) << " : "
                          << snd_seq_port_info_get_name(pinfo) 
                          << std::endl;
            }
        }
    }


    // close the handle
    //
    snd_seq_close(handle);
    */
}
