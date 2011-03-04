/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef JACKTRACKER_H
#define JACKTRACKER_H

#include <jack/jack.h>
#include <jack/ringbuffer.h>

namespace Rosegarden
{


/**
 * \brief Stores audio data from jack into a ring buffer
 *
 * This is part of the Glasgow Center for Music Technology's
 * "Rosegarden Codicil" project.
 * http://www.n-ism.org/Projects/microtonalism.php
 *
 * \author Doug McGilvray (original author)
 * \author Graham Percival (extensively rewritten to match Rosegarden standards)
 * \date 2004, rewrite 2009
 *
 */
class JackCaptureClient {
public:
    JackCaptureClient(const char *captureClientName, int fs);
    ~JackCaptureClient();

    // control processing
    void startProcessing();
    void stopProcessing();

    // getting info
    bool getFrame( float *frame, size_t captureSize );
    bool isConnected() {
        return m_isConnected;
    }
    jack_nframes_t getSampleRate() {
        return m_jackSampleRate;
    }

protected:
    // basic setup + init
    void setFrameSize(int nextFrameSize);
    const char **getPorts();
    const char* getCapturePortName();
    void setupPorts(const char *portName, const char *captureClientName);

    // control processing
    static void jackShutdown(void *arg);

    // actual realtime portion
    static int process(jack_nframes_t nframes, void *arg);

private:
    bool                m_isConnected;
    volatile bool       m_processing;

    jack_client_t      *client;
    jack_port_t        *inPort;
    jack_port_t        *m_capturePort;

    size_t              m_jackBufferSize;
    size_t              m_jackSampleSize;
    jack_nframes_t      m_jackSampleRate;
    jack_ringbuffer_t  *m_jackRingBuffer;

    size_t              m_frameSize;
};


}
#endif
