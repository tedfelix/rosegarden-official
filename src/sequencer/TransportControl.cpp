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

    int processCallbackC(jack_nframes_t nframes, void *arg)
    {
        Rosegarden::TransportControl* tc =
            static_cast<Rosegarden::TransportControl*>(arg);
        return tc->processCallback(nframes);
    }

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
    : m_state(JackTransportStopped),
      m_allowedDelta(RealTime::fromSeconds(0.05)),
      m_waitingForStart(false),
      m_waitingForStartJack(false)
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
    jack_set_process_callback(m_client, processCallbackC, this);
    jack_set_sync_callback(m_client, syncCallbackC, this);
    jack_set_sync_timeout(m_client, 2000000);
    jack_activate(m_client);
#endif
}

TransportControl::~TransportControl()
{
    RG_DEBUG << "dtor";
#ifdef HAVE_LIBJACK
    jack_client_close(m_client);
#endif
}

void TransportControl::tick()
{
    RosegardenSequencer& seq = *RosegardenSequencer::getInstance();
    //RG_DEBUG << "tick" << seq.getStatus();
    switch (seq.getStatus()) {
    case STARTING_TO_PLAY:
        if (Preferences::getUseJackTransport()) {
            // the sequncer is ready to play but we cannot set the
            // state to PLAYING yet because there may be a slow
            // starter. Wait for jack rolling
            m_waitingForStartJack = true;
            sequencerPlayReady();
        } else {
            if (!seq.startPlaying()) {
                // send result failed and stop Sequencer
                seq.setStatus(STOPPING);
            } else {
                seq.setStatus(PLAYING);
            }
        }
        break;
    case QUIT:
    case PLAYING:
    case STARTING_TO_RECORD:
        if (Preferences::getUseJackTransport()) {
            // the sequncer is ready to record but we cannot set the
            // state to RECORDING yet because there may be a slow
            // starter. Wait for jack rolling
            m_waitingForStartJack = true;
            sequencerPlayReady();
        } else {
            if (!seq.startPlaying()) {
                seq.setStatus(STOPPING);
            } else {
                seq.setStatus(RECORDING);
            }
        }
        break;

    case RECORDING:
    case STOPPING:
    case RECORDING_ARMED:
    case STOPPED:
    default:
        break;
    }

}

int TransportControl::play(RealTime startPos)
{
#ifdef HAVE_LIBJACK
    int result = true;
    if (Preferences::getUseJackTransport()) {
        unsigned int sampleRate =
            RosegardenSequencer::getInstance()->getSampleRate();
        unsigned long frame = RealTime::realTime2Frame(startPos, sampleRate);
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

void TransportControl::sequencerPlayReady()
{
    RG_DEBUG << "sequencerPlayReady";
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
        // if we are in record count in the time may be 0. We cannot
        // use -ve time in jack so we have to set zero here but still
        // tell the sequencer to go to the -ve time.
        if (time < RealTime::zero()) {
            // tell jack to go to zero
            jack_transport_locate(m_client, 0);
            // tell the sequencer to go to the -ve time
            RosegardenSequencer::getInstance()->jumpTo(time);
        } else {
            unsigned int sampleRate =
                RosegardenSequencer::getInstance()->getSampleRate();
            unsigned long frame = RealTime::realTime2Frame(time, sampleRate);
            RG_DEBUG << "jack_transport_locate" << frame;
            jack_transport_locate(m_client, frame);
        }
    } else {
        RG_DEBUG << "jumpTo internal" << time;
        RosegardenSequencer::getInstance()->jumpTo(time);
    }
#else
    RosegardenSequencer::getInstance()->jumpTo(time);
#endif
}

int TransportControl::record(const RealTime &time, long recordMode)
{
    RG_DEBUG << "record" << time << recordMode;
    bool playRequested = false;
    int ret = RosegardenSequencer::getInstance()->record
        (time, recordMode, playRequested);
    if (playRequested) {
        RG_DEBUG << "record - play requested";
        ret = play(time);
    }
    return ret;
}

#ifdef HAVE_LIBJACK
int TransportControl::syncCallback(jack_transport_state_t state,
                                   const jack_position_t *pos) const
{
    static bool waiting = false;
    if (m_waitingForStart != waiting) {
        RG_DEBUG << "syncCallback" << state << pos->usecs << m_waitingForStart;
        waiting = m_waitingForStart;
    }

    if (m_waitingForStart) return 0;

    return 1;
}

#endif

#ifdef HAVE_LIBJACK
int TransportControl::processCallback(jack_nframes_t)
{
    //RG_DEBUG << "processCallback";
    if (!Preferences::getUseJackTransport()) return 0;
    jack_position_t pos ;
    jack_transport_state_t state = jack_transport_query(m_client, &pos);

    RosegardenSequencer& seq = *RosegardenSequencer::getInstance();

    unsigned int frame = pos.frame;
    unsigned int sampleRate = seq.getSampleRate();
    RealTime jackTime = RealTime::frame2RealTime(frame, sampleRate);
    RealTime songPosition = seq.getSongPosition();
    RosegardenDocument* doc = RosegardenDocument::currentDocument;
    if (! doc) return 0;
    const Composition& comp = doc->getComposition();
    timeT endTime = comp.getEndMarker();
    // If "stop at end of last Segment" is enabled, use the latest
    // Segment end time.
    if (Preferences::getStopAtSegmentEnd())
        endTime = comp.getDuration(true);
    RealTime compEndTime =
        comp.getElapsedRealTime(endTime);
    RealTime delta = jackTime - songPosition;
    //RG_DEBUG << "delta" << jackTime << songPosition <<
    //  compEndTime << delta;
    if (delta > m_allowedDelta || delta < -m_allowedDelta) {
        if (jackTime <= compEndTime) {
            RG_DEBUG << "jump" << jackTime << songPosition <<
                compEndTime << delta << frame << sampleRate;
            seq.jumpTo(jackTime);
        }
    }
    //RG_DEBUG << "processCallback state" << state << m_state;

    if (m_waitingForStartJack && state == JackTransportRolling) {
        // now we are ready to play or record and everyone else too
        if (seq.getStatus() == STARTING_TO_PLAY) {
            if (!seq.startPlaying()) {
                // send result failed and stop Sequencer
                seq.setStatus(STOPPING);
            } else {
                seq.setStatus(PLAYING);
            }
        }
        if (seq.getStatus() == STARTING_TO_RECORD) {
            if (!seq.startPlaying()) {
                seq.setStatus(STOPPING);
            } else {
                seq.setStatus(RECORDING);
            }
        }
        m_waitingForStartJack = false;
    }

    if (state != m_state) {
        QString sstr("unknown");
        if (state == JackTransportStopped) sstr = "stopped";
        if (state == JackTransportRolling) sstr = "rolling";
        if (state == JackTransportStarting) sstr = "starting";
        RG_DEBUG << "jack state change" << sstr;
        if (state == JackTransportStarting) {
            // no start if we are after composition end
            if (jackTime < compEndTime) {
                seq.play(jackTime);
            }
        }
        if (state == JackTransportStopped) {
            seq.stop(false);
        }

        m_state = state;
    }
    return 0;
}
#endif


}
