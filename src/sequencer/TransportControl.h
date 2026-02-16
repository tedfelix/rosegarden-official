/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_TRANSPORTCONTROL_H
#define RG_TRANSPORTCONTROL_H

#include "base/RealTime.h"

#ifdef HAVE_LIBJACK
#include <jack/jack.h>
#endif

namespace Rosegarden {

class TransportControl
{
public:

    // Singleton
    static TransportControl *getInstance();
    ~TransportControl();
    TransportControl(const TransportControl&) = delete;
    TransportControl& operator=(const TransportControl&) = delete;

    int play(RealTime startPos);
    void playingStarted();
    void stop(bool autoStop);
    void jumpTo(RealTime time);
    int record();

#ifdef HAVE_LIBJACK
    int processCallback(jack_nframes_t nframes);
    int syncCallback(jack_transport_state_t state,
                     const jack_position_t *pos) const;
#endif

private:
    TransportControl();

#ifdef HAVE_LIBJACK
    // TransportControl has its own jack client.
    jack_client_t* m_client;
    jack_transport_state_t m_state;
    RealTime m_allowedDelta;
    bool m_waitingForStart;
#endif
};

}

#endif // RG_TRANSPORTCONTROL_H
