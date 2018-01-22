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

#include "AlsaPort.h"

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
#include "misc/Strings.h"

#ifdef HAVE_LIBJACK
#include <jack/types.h>
#include <unistd.h> // for usleep
#include <cmath>
#endif

namespace Rosegarden
{

AlsaPortDescription::AlsaPortDescription(Instrument::InstrumentType type,
        const std::string &name,
        int client,
        int port,
        unsigned int clientType,
        unsigned int portType,
        unsigned int capability,
        PortDirection direction):
        m_type(type),
        m_name(name),
        m_client(client),
        m_port(port),
        m_clientType(clientType),
        m_portType(portType),
        m_capability(capability),
        m_direction(direction)
{}


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


    // See comment in AlsaDriver::createMidiDevice -- the client
    // numbering scheme changed in ALSA driver 1.0.11rc1.
    // We now order:
    //
    // * Write-only software ports (client id 128+) sorted by client
    //   id then port id
    //
    // * Probable hardware ports (client id 16-127) sorted by
    //   direction (write, duplex, read) then client id (64+
    //   preferred) then port id
    //
    // * Read-write or read-only software ports (client id 128+)
    //   sorted by client id then port id
    //
    // * System ports (client id 0-15) sorted by client id then
    //   port id
    //
    // It's necessary to handle software ports ahead of
    // hardware/system ports, because we want to keep all the hardware
    // ports together (we don't want to change the priority of a
    // hardware port relative to a software port based on its client
    // ID) and we can't know for sure whether the 16-63 range are
    // hardware or system ports.

    enum Category {
        WRITE_ONLY_SOFTWARE,
        HARDWARE_PROBABLY,
        MIXED_SOFTWARE,
        SYSTEM
    };

    bool oldScheme = (SND_LIB_MAJOR == 0 ||
                      (SND_LIB_MAJOR == 1 &&
                       SND_LIB_MINOR == 0 &&
                       SND_LIB_SUBMINOR < 11));

    Category a1cat;
    if (a1->m_client < 16)
        a1cat = SYSTEM;
    else if (oldScheme && (a1->m_client < 64))
        a1cat = SYSTEM;
    else if (a1->m_client < 128)
        a1cat = HARDWARE_PROBABLY;
    else
        a1cat = MIXED_SOFTWARE;

    if (a1cat == MIXED_SOFTWARE) {
        if (a1->m_direction == WriteOnly)
            a1cat = WRITE_ONLY_SOFTWARE;
    }

    Category a2cat;
    if (a2->m_client < 16)
        a2cat = SYSTEM;
    else if (oldScheme && (a2->m_client < 64))
        a2cat = SYSTEM;
    else if (a2->m_client < 128)
        a2cat = HARDWARE_PROBABLY;
    else
        a2cat = MIXED_SOFTWARE;

    if (a2cat == MIXED_SOFTWARE) {
        if (a2->m_direction == WriteOnly)
            a2cat = WRITE_ONLY_SOFTWARE;
    }

    if (a1cat != a2cat)
        return int(a1cat) < int(a2cat);

    if (a1cat == HARDWARE_PROBABLY) {

        if (a1->m_direction == WriteOnly) {
            if (a2->m_direction != WriteOnly)
                return true;
        } else if (a1->m_direction == Duplex) {
            if (a2->m_direction == ReadOnly)
                return true;
        }

        int c1 = a1->m_client;
        int c2 = a2->m_client;
        if (c1 < 64)
            c1 += 1000;
        if (c2 < 64)
            c2 += 1000;
        if (c1 != c2)
            return c1 < c2;

    } else if (a1cat == SYSTEM) {

        int c1 = a1->m_client;
        int c2 = a2->m_client;
        if (c1 < 16)
            c1 += 1000;
        if (c2 < 16)
            c2 += 1000;
        if (c1 != c2)
            return c1 < c2;
    }

    if (a1->m_client != a2->m_client) {
        return a1->m_client < a2->m_client;
    } else {
        return a1->m_port < a2->m_port;
    }
}

}

#endif // HAVE_ALSA
