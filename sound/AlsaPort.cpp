// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4
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

#include "AlsaPort.h"

#include <iostream>
#include <cstdlib>
#include <cstdio>

#ifdef HAVE_ALSA

// ALSA
#include <alsa/asoundlib.h>
#include <alsa/seq_event.h>
#include <alsa/version.h>

#include "MappedInstrument.h"
#include "Midi.h"
#include "WAVAudioFile.h"
#include "MappedStudio.h"
#include "rosestrings.h"

#ifdef HAVE_LIBJACK
#include <jack/types.h>
#include <unistd.h> // for usleep
#include <cmath>
#endif

namespace Rosegarden
{

#ifdef NOT_DEFINED
// --------- AlsaPort ------------
//
AlsaPort::AlsaPort(InstrumentId startId,
                   InstrumentId endId,
                   const std::string &name,
                   int client,
                   int port,
                   PortDirection direction):
        m_startId(startId),
        m_endId(endId),
        m_name(name),
        m_client(client),
        m_port(port),
        m_direction(direction),
        m_type(0)
{
}

AlsaPort::~AlsaPort()
{
}
#endif

AlsaPortDescription::AlsaPortDescription(Instrument::InstrumentType type,
                                         const std::string &name,
                                         int client,
                                         int port,
                                         PortDirection direction):
        m_type(type),
        m_name(name),
        m_client(client),
        m_port(port),
        m_direction(direction)
{
}


bool
AlsaPortCmp::operator()(AlsaPortDescription *a1, AlsaPortDescription *a2)
{
    // Ordering for ALSA ports in the list:
    //
    // * Hardware ports (client id 64-127) sorted by direction
    //   (write, duplex, read) then client id then port id
    //
    // * Software ports (client id 128+) sorted by client id
    //   then port id
    // 
    // * System ports (client id 0-63) sorted by client id then
    //   port id

    const int HARDWARE = 1, SOFTWARE = 2, SYSTEM = 3;

    int a1type = (a1->m_client < 64  ? SYSTEM :
		  a1->m_client < 128 ? HARDWARE : SOFTWARE);

    int a2type = (a2->m_client < 64  ? SYSTEM :
		  a2->m_client < 128 ? HARDWARE : SOFTWARE);

    if (a1type != a2type) return a1type < a2type;

    if (a1type == HARDWARE) {
	if (a1->m_direction == WriteOnly) {
	    if (a2->m_direction != WriteOnly) return true;
	} else if (a1->m_direction == Duplex) {
	    if (a2->m_direction == ReadOnly) return true;
	}
    }

    if (a1->m_client != a2->m_client) {
	return a1->m_client < a2->m_client;
    } else {
	return a1->m_port   < a2->m_port;
    }
}

}

#endif // HAVE_ALSA
