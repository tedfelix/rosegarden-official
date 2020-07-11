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

#include <vector>
#include <set>

#include <QSharedPointer>

#include "base/Instrument.h"
#include "MappedCommon.h"

#ifndef RG_ALSAPORT_H
#define RG_ALSAPORT_H

#ifdef HAVE_ALSA
#include <alsa/asoundlib.h> // ALSA

namespace Rosegarden
{


struct ClientPortPair
{
    ClientPortPair() :
        client(0),
        port(0)
    { }

    ClientPortPair(int i_client, int i_port) :
        client(i_client),
        port(i_port)
    { }

    int client;
    int port;

    bool operator==(const ClientPortPair &rhs)
    {
        return (client == rhs.client  &&  port == rhs.port);
    }

    bool operator!=(const ClientPortPair &rhs)
    {
        return (client != rhs.client  ||  port != rhs.port);
    }
};

/**
 * Use this to hold all client information so that we can sort it
 * before generating devices - we want to put non-duplex devices
 * at the front of any device list (makes thing much easier at the
 * GUI and we already have some backwards compatibility issues with
 * this).
 */
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

/// Sort by checking direction
struct AlsaPortCmp
{
    bool operator()(QSharedPointer<AlsaPortDescription> a1,
                    QSharedPointer<AlsaPortDescription> a2);
};


}

#endif // HAVE_ALSA

#endif // RG_ALSAPORT_H

