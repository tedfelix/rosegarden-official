/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_EXTERNAL_TRANSPORT_H
#define RG_EXTERNAL_TRANSPORT_H

namespace Rosegarden {

/**
 * Simple interface that we can pass to low-level audio code and on
 * which it can call back when something external requests a transport
 * change.  The callback is asynchronous, and there's a method for the
 * low-level code to use to find out whether its request has finished
 * synchronising yet.
 *
 * (Each of the transportXX functions returns a token which can then
 * be passed to isTransportSyncComplete.)
 */

class ExternalTransport
{
public:
    typedef unsigned long TransportToken;

    enum TransportRequest {
        TransportNoChange,
        TransportStop,
        TransportStart,
        TransportPlay,
        TransportRecord,
        TransportJumpToTime, // time arg required
        TransportStartAtTime, // time arg required
        TransportStopAtTime // time arg required
    };

    virtual ~ExternalTransport() {}

    virtual TransportToken transportChange(TransportRequest) = 0;
    virtual TransportToken transportJump(TransportRequest, RealTime) = 0;

    virtual bool isTransportSyncComplete(TransportToken token) = 0;

    // The value returned here is a constant (within the context of a
    // particular ExternalTransport object) that is guaranteed never
    // to be returned by any of the transport request methods.
    virtual TransportToken getInvalidTransportToken() const = 0;
};

}

#endif

