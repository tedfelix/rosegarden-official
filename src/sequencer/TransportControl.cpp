/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[TransportControl]"
//#define RG_NO_DEBUG_PRINT

#include "TransportControl.h"
#include "RosegardenSequencer.h"

#include "base/Composition.h"
#include "document/RosegardenDocument.h"
#include "misc/Debug.h"
#include "misc/Preferences.h"

#ifdef HAVE_LIBJACK
namespace
{

    int syncCallbackC(jack_transport_state_t state,
                      jack_position_t *pos,
                      void *arg)
    {
        Rosegarden::TransportControl* tc =
            static_cast<Rosegarden::TransportControl*>(arg);
        return tc->syncCallback(state, pos);
    }

}
#endif

namespace Rosegarden
{


TransportControl* TransportControl::getInstance()
{
    // Guaranteed in C++11 to be lazy initialized and thread-safe.
    // See ISO/IEC 14882:2011 6.7(4).
    static TransportControl instance;

    return &instance;
}

TransportControl::TransportControl()
#ifdef HAVE_LIBJACK
    : m_oldState(JackTransportStopped),
      m_allowedDelta(RealTime::fromSeconds(0.02))
#endif
{
#ifdef HAVE_LIBJACK
    RG_DEBUG << "ctor";
    jack_status_t status;
    m_client = jack_client_open("RosegardenTransportClient",
                                JackNoStartServer,
                                &status,
                                nullptr);
    Q_ASSERT(m_client != nullptr);
    jack_activate(m_client);
    jack_set_sync_callback(m_client, syncCallbackC, this);
    m_waitingForStart = false;
#endif
}

TransportControl::~TransportControl()
{
    RG_DEBUG << "dtor";
#ifdef HAVE_LIBJACK
    jack_client_close(m_client);
#endif
}

int TransportControl::play(RealTime startPos)
{
#ifdef HAVE_LIBJACK
    int result = true;
    if (Preferences::getUseJackTransport()) {
        unsigned int sampleRate =
            RosegardenSequencer::getInstance()->getSampleRate();
        long frame = RealTime::realTime2Frame(startPos, sampleRate);
        RG_DEBUG << "play jackTransport" << startPos << sampleRate << frame;
        jack_transport_locate(m_client, frame);
        jack_transport_start(m_client);
        m_waitingForStart = true;
    } else {
        RG_DEBUG << "play internal" << startPos;
        result = RosegardenSequencer::getInstance()->play(startPos);
    }
    return result;
#else
    return RosegardenSequencer::getInstance()->play(startPos);
#endif
}

void TransportControl::playingStarted()
{
    RG_DEBUG << "playingStarted";
    m_waitingForStart = false;
}

void TransportControl::stop(bool autoStop)
{
#ifdef HAVE_LIBJACK
    if (Preferences::getUseJackTransport()) {
        bool jackStopAtAutoStop = Preferences::getJACKStopAtAutoStop();
        RG_DEBUG << "stop jackTransport" << autoStop << jackStopAtAutoStop;
        if (jackStopAtAutoStop || ! autoStop) {
            jack_transport_stop(m_client);
        } else {
            // let jack continue running but still tell the sequnecer to stop
            RosegardenSequencer::getInstance()->stop(autoStop);
        }
    } else {
        RG_DEBUG << "stop internal" << autoStop;
        RosegardenSequencer::getInstance()->stop(autoStop);
    }
#else
    RosegardenSequencer::getInstance()->stop(autoStop);
#endif
}

void TransportControl::jumpTo(RealTime time)
{
    RG_DEBUG << "jumpTo" << time;
#ifdef HAVE_LIBJACK
    if (Preferences::getUseJackTransport()) {
        unsigned int sampleRate =
            RosegardenSequencer::getInstance()->getSampleRate();
        long frame = RealTime::realTime2Frame(time, sampleRate);
        jack_transport_locate(m_client, frame);
    } else {
        RG_DEBUG << "jumpTo internal" << time;
        RosegardenSequencer::getInstance()->jumpTo(time);
    }
#else
    RosegardenSequencer::getInstance()->jumpTo(time);
#endif
}

int TransportControl::record()
{
    RG_DEBUG << "record";
    return 0;
}

#ifdef HAVE_LIBJACK
int TransportControl::syncCallback(jack_transport_state_t state,
                                   const jack_position_t *pos) const
{
    RG_DEBUG << "syncCallback" << state << pos->usecs << m_waitingForStart;

    if (m_waitingForStart) return 0;

    return 1;
}

#endif

void TransportControl::tick()
{
#ifdef HAVE_LIBJACK
    jack_position_t pos ;
    jack_transport_state_t state = jack_transport_query(m_client, &pos);

    int frame = pos.frame;
    unsigned int sampleRate =
        RosegardenSequencer::getInstance()->getSampleRate();
    RealTime jackTime = RealTime::frame2RealTime(frame, sampleRate);
    RealTime songPosition =
        RosegardenSequencer::getInstance()->getSongPosition();
    RosegardenDocument* doc = RosegardenDocument::currentDocument;
    if (! doc) return;
    const Composition& comp = doc->getComposition();
    timeT endTime = comp.getEndMarker();
    // If "stop at end of last Segment" is enabled, use the latest
    // Segment end time.
    if (Preferences::getStopAtSegmentEnd())
        endTime = comp.getDuration(true);
    RealTime compEndTime =
        comp.getElapsedRealTime(endTime);
    RealTime delta = jackTime - songPosition;
    RG_DEBUG << "delta" << jackTime << songPosition <<
        compEndTime << delta;
    if (delta > m_allowedDelta || delta < -m_allowedDelta) {
        if (jackTime <= compEndTime) {
            RG_DEBUG << "jump" << jackTime << songPosition <<
                compEndTime << delta;
            RosegardenSequencer::getInstance()->jumpTo(jackTime);
        }
    }
    //RG_DEBUG << "tick" << state << m_oldState;
    if (state != m_oldState) {
        QString sstr("unknown");
        if (state == JackTransportStopped) sstr = "stopped";
        if (state == JackTransportRolling) sstr = "rolling";
        if (state == JackTransportStarting) sstr = "starting";
        RG_DEBUG << "jack state change" << sstr;
        if (state == JackTransportStarting) {
            // no start if we are after composition end
            if (jackTime < compEndTime) {
                RosegardenSequencer::getInstance()->play(jackTime);
            }
        }
        if (state == JackTransportStopped) {
            RosegardenSequencer::getInstance()->stop(false);
        }

        m_oldState = state;
    }

#endif
}


}
