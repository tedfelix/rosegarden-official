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

// --------- AlsaPort ------------
//
AlsaPort::AlsaPort(InstrumentId startId,
                   InstrumentId endId,
                   const std::string &name,
                   int client,
                   int port,
                   bool duplex):
        m_startId(startId),
        m_endId(endId),
        m_name(name),
        m_client(client),
        m_port(port),
        m_duplex(duplex),
        m_type(0)
{
}

AlsaPort::~AlsaPort()
{
}


AlsaPortDescription::AlsaPortDescription(Instrument::InstrumentType type,
                                         const std::string &name,
                                         int client,
                                         int port,
                                         bool duplex):
        m_type(type),
        m_name(name),
        m_client(client),
        m_port(port),
        m_duplex(duplex)
{
}

}

#endif // HAVE_ALSA
