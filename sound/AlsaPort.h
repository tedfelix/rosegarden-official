// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2003
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
			unsigned int clientType,
			unsigned int portType,
			unsigned int capability,
                        PortDirection direction);

    Instrument::InstrumentType m_type;
    std::string                m_name;
    int                        m_client;
    int                        m_port;
    unsigned int               m_clientType;
    unsigned int               m_portType;
    unsigned int               m_capability;
    PortDirection              m_direction; // or can deduce from capability

    bool isReadable()  { return m_direction == ReadOnly ||
			        m_direction == Duplex; }

    bool isWriteable() { return m_direction == WriteOnly ||
			        m_direction == Duplex; }

};

// Sort by checking direction
//
struct AlsaPortCmp
{
    bool operator()(AlsaPortDescription *a1,
                    AlsaPortDescription *a2);
};


};

#endif // HAVE_ALSA

#endif // _RG_ALSA_PORT_H_

