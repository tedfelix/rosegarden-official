// -*- c-basic-offset: 4 -*-
/*
    Rosegarden
    A sequencer and musical notation editor.

    This program is Copyright 2000-2007
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

#ifndef _EXTERNAL_TRANSPORT_H_
#define _EXTERNAL_TRANSPORT_H_

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

