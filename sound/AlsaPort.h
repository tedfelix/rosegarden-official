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

#include <vector>
#include <set>
#include "config.h"
#include "Instrument.h"
#include "MappedCommon.h"

#ifndef _ALSAPORT_H_
#define _ALSAPORT_H_

#ifdef HAVE_ALSA
#include <alsa/asoundlib.h> // ALSA

namespace Rosegarden
{

// An AlsaPort represents one or more (usually 16) MappedInstruments.
//
//
class AlsaPort
{
public:
    AlsaPort(InstrumentId startId,
             InstrumentId endId,
             const std::string &name,
             int client,
             int port,
             PortDirection direction);
    ~AlsaPort();

    InstrumentId  m_startId;
    InstrumentId  m_endId;
    std::string   m_name;
    int           m_client;
    int           m_port;
    PortDirection m_direction; // read, write or duplex
    unsigned int  m_type;      // MIDI, synth or what?
};


typedef std::pair<int, int> ClientPortPair;

// Use this to hold all client information so that we can sort it
// before generating devices - we want to put non-duplex devices
// at the front of any device list (makes thing much easier at the
// GUI and we already have some backwards compatability issues with
// this).
//
class AlsaPortDescription
{
public:
    AlsaPortDescription(Instrument::InstrumentType type,
                        const std::string &name,
                        int client,
                        int port,
                        PortDirection direction);

    Instrument::InstrumentType m_type;
    std::string                m_name;
    int                        m_client;
    int                        m_port;
    PortDirection              m_direction;
};

// Sort by checking direction
//
struct AlsaPortCmp
{
    bool operator()(AlsaPortDescription *a1,
                    AlsaPortDescription *a2)
    {
        // Handle non-system clients by pushing them to the end of the
        // device list always.  This will keep devices in the order:
        //
        // o write-only system devices (soundcard synths)
        // o duplex system devices (MIDI ports)
        // o software devices (softsynths)
        //
        // I don't want to use the 128 here but this is at least
        // emperically correct for the moment and can't see a 
        // working alternative.
        //
        if (a1->m_client < 128 && a1->m_direction != a2->m_direction)
            return true;
        else
        {
            if (a1->m_client != a2->m_client)
                return a1->m_client < a2->m_client;
            else
                return a1->m_port < a2->m_port;
        }
    }
};


};

#endif // HAVE_ALSA

#endif // _RG_ALSA_PORT_H_

