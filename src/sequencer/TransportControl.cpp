/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2026 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_NO_DEBUG_PRINT

#include "TransportControl.h"
#include "RosegardenSequencer.h"

#include "misc/Debug.h"
#include "misc/Preferences.h"
#include "sound/SequencerDataBlock.h"

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
        :
        m_state(JackTransportStopped),
        m_allowedDelta(RealTime::fromMilliseconds(0.1)),
        m_waitingForStartJack(false),
        m_countIn(false),
        m_resetPlaybackOnJump(true),
        m_jackAvailable(false),
        m_jackDecoupled(false)
#endif
{
#ifdef HAVE_LIBJACK
    RG_DEBUG << "ctor";
    jack_status_t status;
    m_client = jack_client_open("RosegardenTransportClient",
                                JackNoStartServer,
                                &status,
                                nullptr);
    if (m_client == nullptr) {
        RG_WARNING << "TransportControl ctor jack not available";
        // m_jackAvailable stays false
        return;
    }
    m_jackAvailable = true;
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
    // called from sequencer thread
    RosegardenSequencer& seq = *RosegardenSequencer::getInstance();
    //RG_DEBUG << "tick" << seq.getStatus();
#ifdef HAVE_LIBJACK
    if (Preferences::getUseJackTransport() &&
        Preferences::getUseNewJackTransport() &&
        m_jackAvailable) {
        // logic with jack transport
        switch (seq.getStatus()) {
        case STARTING_TO_PLAY:
            // the sequncer is ready to play but we cannot set the
            // state to PLAYING yet because there may be a slow
            // starter. Wait for jack rolling
            m_waitingForStartJack = true;
            break;
        case QUIT:
            break;
        case PLAYING:
            if (!seq.keepPlaying()) {
                // there's a problem or the piece has
                // finished - so stop playing
                seq.setStatus(STOPPING);
            } else {
                // process any async events
                //
                seq.processAsynchronousEvents();
            }
            break;
        case STARTING_TO_RECORD:
            if (m_countIn) {
                // just go to recording
                if (!seq.startPlaying()) {
                    seq.setStatus(STOPPING);
                } else {
                    seq.setStatus(RECORDING);
                }
            } else {
                // the sequncer is ready to record but we cannot set the
                // state to RECORDING yet because there may be a slow
                // starter. Wait for jack rolling
                m_waitingForStartJack = true;
            }
            break;

        case RECORDING:
            RG_DEBUG << "recording" << m_countIn <<
                SequencerDataBlock::getInstance()->getPositionPointer();
            if (m_countIn) {
                // if the time is (just before) 0 start jack rolling
                RealTime position =
                    SequencerDataBlock::getInstance()->getPositionPointer();
                if (position >= RealTime(0, -500000))
                    {
                        RG_DEBUG << "count in end - starting jack";
                        jack_transport_start(m_client);
                        m_countIn = false;
                    }
            }
            if (!seq.keepPlaying()) {
                // there's a problem or the piece has
                // finished - so stop playing
                seq.setStatus(STOPPING);
            } else {
                // Now process any incoming MIDI events
                // and return them to the gui
                //
                seq.processRecordedMidi();

                // Now process any incoming audio
                // and return it to the gui
                //
                seq.processRecordedAudio();

                // Still process these so we can send up
                // audio levels as MappedEvents
                //
                // Bug #1348 MIDI Recording Drops Notes (was #3542166).
                // This line can occasionally steal MIDI
                // events that are needed by processRecordedMidi().
                // Need to track down what the above "audio levels" comment
                // means and whether it is a serious issue.  If so, we need
                // to address it in a different way.  This line probably
                // never did anything as by the time it was run,
                // processRecordedMidi() would have cleaned out all the
                // incoming events.
                //seq.processAsynchronousEvents();
            }
            break;
        case STOPPING:
            // There's no call to RosegardenSequencer to actually process the
            // stop, because this arises from a call from the GUI
            // direct to RosegardenSequencer to start with
            seq.setStatus(STOPPED);

            RG_DEBUG << "run() - Stopped";
            break;
        case RECORDING_ARMED:
            RG_DEBUG << "run() - Sequencer can't enter \"RECORDING_ARMED\" state - internal error";
            break;
        case STOPPED:
        default:
            seq.processAsynchronousEvents();
            break;
        }
    }
#endif // HAVE_LIBJACK

    bool withJackTransport = false;
#ifdef HAVE_LIBJACK
    withJackTransport = (Preferences::getUseJackTransport() &&
                         Preferences::getUseNewJackTransport() &&
                         m_jackAvailable);
#endif
    if (! withJackTransport) {
        // logic without jack transport
        switch (seq.getStatus()) {
        case STARTING_TO_PLAY:
            if (!seq.startPlaying()) {
                // send result failed and stop Sequencer
                seq.setStatus(STOPPING);
            } else {
                seq.setStatus(PLAYING);
            }
            break;
        case QUIT:
            break;
        case PLAYING:
            if (!seq.keepPlaying()) {
                // there's a problem or the piece has
                // finished - so stop playing
                seq.setStatus(STOPPING);
            } else {
                // process any async events
                //
                seq.processAsynchronousEvents();
            }
            break;
        case STARTING_TO_RECORD:
            if (!seq.startPlaying()) {
                seq.setStatus(STOPPING);
            } else {
                seq.setStatus(RECORDING);
            }
            break;

        case RECORDING:
            if (!seq.keepPlaying()) {
                // there's a problem or the piece has
                // finished - so stop playing
                seq.setStatus(STOPPING);
            } else {
                // Now process any incoming MIDI events
                // and return them to the gui
                //
                seq.processRecordedMidi();

                // Now process any incoming audio
                // and return it to the gui
                //
                seq.processRecordedAudio();

                // Still process these so we can send up
                // audio levels as MappedEvents
                //
                // Bug #1348 MIDI Recording Drops Notes (was #3542166).
                // This line can occasionally steal MIDI
                // events that are needed by processRecordedMidi().
                // Need to track down what the above "audio levels" comment
                // means and whether it is a serious issue.  If so, we need
                // to address it in a different way.  This line probably
                // never did anything as by the time it was run,
                // processRecordedMidi() would have cleaned out all the
                // incoming events.
                //seq.processAsynchronousEvents();
            }
            break;
        case STOPPING:
            // There's no call to RosegardenSequencer to actually process the
            // stop, because this arises from a call from the GUI
            // direct to RosegardenSequencer to start with
            seq.setStatus(STOPPED);

            RG_DEBUG << "run() - Stopped";
            break;
        case RECORDING_ARMED:
            RG_DEBUG << "run() - Sequencer can't enter \"RECORDING_ARMED\" state - internal error";
            break;
        case STOPPED:
        default:
            seq.processAsynchronousEvents();
            break;
        }
    }
}

int TransportControl::play(RealTime startPos)
{
    // called from gui thread
#ifdef HAVE_LIBJACK
    int result = true;
    if (Preferences::getUseJackTransport() &&
        Preferences::getUseNewJackTransport() &&
        m_jackAvailable) {
        unsigned int sampleRate =
            RosegardenSequencer::getInstance()->getSampleRate();
        if (startPos < RealTime::zero()) {
            // This is a tricky case - it happens if we are recording
            // from near the beginning of the piece with count in. We
            // must start without jack and try to start jack rolling
            // when the time gets to 0
            RG_DEBUG << "play start time -ve play inetrnal";
            m_countIn = true;
            result = RosegardenSequencer::getInstance()->play(startPos);
        } else {
            // start using jack transport
            unsigned long frame =
                RealTime::realTime2Frame(startPos, sampleRate);
            RG_DEBUG << "play jackTransport" << startPos << sampleRate << frame;
            jack_transport_locate(m_client, frame);
            jack_transport_start(m_client);
        }
    } else {
        RG_DEBUG << "play internal" << startPos;
        result = RosegardenSequencer::getInstance()->play(startPos);
    }
    return result;
#else
    return RosegardenSequencer::getInstance()->play(startPos);
#endif
}

void TransportControl::stop(bool autoStop)
{
    // called from gui thread
#ifdef HAVE_LIBJACK
    RG_DEBUG << "stop" << autoStop;
    if ( autoStop) m_jackDecoupled = false;

    if (Preferences::getUseJackTransport() &&
        Preferences::getUseNewJackTransport() &&
        m_jackAvailable) {
        if (m_countIn) {
            // we are still in the count in and jack is not rolling
            RosegardenSequencer::getInstance()->stop(autoStop);
            m_countIn = false;
        } else {
            bool jackStopAtAutoStop = Preferences::getJACKStopAtAutoStop();
            RG_DEBUG << "stop jackTransport" << autoStop << jackStopAtAutoStop;
            if (jackStopAtAutoStop || ! autoStop) {
                RG_DEBUG << "stop - stop jack transport";
                jack_transport_stop(m_client);
            } else {
                // let jack continue running but still tell the
                // sequnecer to stop. After this Rosegarden and jack
                // are decoupled until either Rosegarden is stopped
                // (withot autoStop) or jack transport is stopped
                RG_DEBUG << "stop - stop but leave jack running";
                m_jackDecoupled = true;
                RosegardenSequencer::getInstance()->stop(autoStop);
            }
        }
    } else {
        RG_DEBUG << "stop internal" << autoStop;
        RosegardenSequencer::getInstance()->stop(autoStop);
    }
#else
    RosegardenSequencer::getInstance()->stop(autoStop);
#endif
}

void TransportControl::jumpTo(RealTime time, bool reset)
{
    // called from gui and sequencer threads
    RG_DEBUG << "jumpTo" << time << reset;
#ifdef HAVE_LIBJACK
    if (Preferences::getUseJackTransport() &&
        Preferences::getUseNewJackTransport() &&
        m_jackAvailable &&
        ! m_jackDecoupled) {
        m_resetPlaybackOnJump = reset;
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
        RosegardenSequencer::getInstance()->jumpTo(time, reset);
    }
#else
    RosegardenSequencer::getInstance()->jumpTo(time);
#endif
}

int TransportControl::record(const RealTime &time, long recordMode)
{
    // called from gui thread
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

bool TransportControl::jackAvailable() const
{
    // called from sequencer thread
#ifdef HAVE_LIBJACK
    return m_jackAvailable;
#else
    return false;
#endif
}

#ifdef HAVE_LIBJACK
int TransportControl::syncCallback(jack_transport_state_t state,
                                   const jack_position_t *pos) const
{
    // called from jack thread
    RG_DEBUG << "syncCallback" <<
        Preferences::getUseJackTransport() <<
        Preferences::getUseNewJackTransport();
    if (! Preferences::getUseJackTransport() ||
        ! Preferences::getUseNewJackTransport() ||
        ! m_jackAvailable) return 1;

    RosegardenSequencer& seq = *RosegardenSequencer::getInstance();
    TransportStatus seqStatus = seq.getStatus();
    unsigned int frame = pos->frame;
    unsigned int sampleRate = seq.getSampleRate();
    RealTime jackTime = RealTime::frame2RealTime(frame, sampleRate);
    RealTime songPosition = seq.getSongPosition();
    RealTime delta = jackTime - songPosition;
    RG_DEBUG << "syncCallback" << m_state << state << pos->frame <<
        jackTime << songPosition << delta << seqStatus;

    if ((delta > m_allowedDelta || delta < -m_allowedDelta) && // jump requested
        seqStatus != STARTING_TO_RECORD &&
        seqStatus != RECORDING) { // no jump while recording
        RG_DEBUG << "syncCallback jump requested" << songPosition <<
            "->" << jackTime;
        seq.jumpTo(jackTime, m_resetPlaybackOnJump);
    }

    return 1;
}

#endif

#ifdef HAVE_LIBJACK
int TransportControl::processCallback(jack_nframes_t)
{
    // called from jack thread
    //RG_DEBUG << "processCallback";

    if (! Preferences::getUseJackTransport() ||
        ! Preferences::getUseNewJackTransport() ||
        ! m_jackAvailable) return 0;

    jack_position_t pos ;
    jack_transport_state_t state = jack_transport_query(m_client, &pos);

    RosegardenSequencer& seq = *RosegardenSequencer::getInstance();
    TransportStatus seqStatus = seq.getStatus();

    if (m_waitingForStartJack && state == JackTransportRolling) {
        // now we are ready to play or record and everyone else too
        if (seqStatus == STARTING_TO_PLAY) {
            if (!seq.startPlaying()) {
                // send result failed and stop Sequencer
                seq.setStatus(STOPPING);
            } else {
                seq.setStatus(PLAYING);
            }
        }
        if (seqStatus == STARTING_TO_RECORD) {
            if (!seq.startPlaying()) {
                seq.setStatus(STOPPING);
            } else {
                seq.setStatus(RECORDING);
            }
        }
        m_waitingForStartJack = false;
    }

    if (state != m_state) {
#ifndef NDEBUG
        QString sstr("unknown");
        if (state == JackTransportStopped) sstr = "stopped";
        if (state == JackTransportRolling) sstr = "rolling";
        if (state == JackTransportStarting) sstr = "starting";
        RG_DEBUG << "jack state change" << sstr;
#endif
        if (state == JackTransportStarting) {
            // start even if we are after composition end so jack
            // starts rolling. If we are beyond end Rosegarden will
            // stop again immediately
            if (seqStatus == STOPPED) {
                unsigned int frame = pos.frame;
                // LOCKED
                unsigned int sampleRate = seq.getSampleRate();
                RealTime jackTime = RealTime::frame2RealTime(frame, sampleRate);
                // LOCKED
                seq.play(jackTime);
            }
        }
        if (state == JackTransportStopped) {
            m_jackDecoupled = false;
            seq.stop(false);
        }

        m_state = state;
    }
    return 0;
}
#endif


}
