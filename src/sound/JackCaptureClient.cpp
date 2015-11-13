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

#include "JackCaptureClient.h"

#include <iostream>

#define DEBUG_JACK_CAPTURE_CLIENT 0

namespace Rosegarden
{


/*************************************
  CONSTRUCTOR, DESTRUCTOR, AND INIT
 *************************************/
JackCaptureClient::JackCaptureClient( const char *captureClientName, int fs ) :
        m_isConnected(false),
        m_processing(false),
        m_jackRingBuffer(NULL),
        m_frameSize(fs)
{
    // Try to connect to Jack server
    if ( (client = jack_client_new(captureClientName)) == 0 ) {
        return;
    }
#if DEBUG_JACK_CAPTURE_CLIENT
    std::cout << "Registered as Jack client" << std::endl;
#endif

    // get stream info
    m_jackSampleRate = jack_get_sample_rate( client );
    m_jackBufferSize = jack_get_buffer_size( client );
    m_jackSampleSize = sizeof(jack_default_audio_sample_t);
#if DEBUG_JACK_CAPTURE_CLIENT
    std::cout << "Sample Rate " << m_jackSampleRate << std::endl;
    std::cout << "Max buffer size " << m_jackBufferSize << std::endl;
    std::cout << "Sample size (bytes) " << m_jackSampleSize << std::endl;
#endif

    //setup ringbuffer
    setFrameSize(m_frameSize);

    //register process and shutdown calls
    jack_set_process_callback(client, &process, (void*)this);
    jack_on_shutdown(client, jackShutdown, (void*)this);
#if DEBUG_JACK_CAPTURE_CLIENT
    std::cout << "Process and shutdown calls registered\n";
#endif

    // Activate
    if ( jack_activate(client) ) {
        std::cout << "Can't activate client\n";
        throw("Cannot activate client");
    }
#if DEBUG_JACK_CAPTURE_CLIENT
    std::cout << "Activated\n";
#endif

    //set default port to the first available port
    const char **ports = getPorts();
    setupPorts(ports[0], captureClientName);

    m_isConnected = true;
}

JackCaptureClient::~JackCaptureClient()
{
    stopProcessing();
    jack_client_close(client);
    if (m_jackRingBuffer) jack_ringbuffer_free(m_jackRingBuffer);
}

void
JackCaptureClient::setFrameSize(int nextFrameSize)
{
    m_frameSize = nextFrameSize + 1;

#if DEBUG_JACK_CAPTURE_CLIENT
    std::cout << "CaptureClient: setting framesize to at least "
              << m_frameSize << std::endl;
#endif

    if (m_processing) {
        std::cout << "CaptureClient: Procesing, can't change framesize"
                  << std::endl;
        return;
    }

    if (m_jackRingBuffer) jack_ringbuffer_free(m_jackRingBuffer);

    // framesize must be larger than size of max buffer size (m_jackBufferSize)
    // else a complete jack frame can never be written to ringbuffer
    if ( m_frameSize < m_jackBufferSize ) m_frameSize = m_jackBufferSize+1;

    size_t m_jackRingBufferSize = m_jackSampleSize * m_frameSize;
    m_jackRingBuffer = jack_ringbuffer_create( m_jackRingBufferSize );
    jack_ringbuffer_reset( m_jackRingBuffer );
#if DEBUG_JACK_CAPTURE_CLIENT
    std::cout << "Created ringbuffer, write space: "
              << jack_ringbuffer_write_space(m_jackRingBuffer)
              << "read space: "
              << jack_ringbuffer_read_space(m_jackRingBuffer)
              << " m_jackSampleSize " << m_jackSampleSize
              << std::endl;;
#endif
}

const char
**JackCaptureClient::getPorts()
{
    return jack_get_ports( client, NULL, NULL, JackPortIsOutput );
}

const char*
JackCaptureClient::getCapturePortName()
{
    return jack_port_name( m_capturePort);
}

void
JackCaptureClient::setupPorts(const char *portName,
                              const char *captureClientName)
{
#if DEBUG_JACK_CAPTURE_CLIENT
    std::cout << "Connecting ports...\n";
#endif

    // register port
    std::string inPortName = captureClientName;
    inPortName.append(" In");

#if DEBUG_JACK_CAPTURE_CLIENT
    std::cout << "Registering input port as:" << inPortName << std::endl;
#endif
    inPort = jack_port_register(client, inPortName.c_str(),
                                JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0 );
    if (inPort == 0) {
        std::cout << "Cannot open Jack port";
    }

    if ( jack_port_connected(inPort) ) {
#if DEBUG_JACK_CAPTURE_CLIENT
        std::cout << "Disconnecting ports\n";
#endif

        const char **connectedPorts = jack_port_get_connections(inPort);
        int i=0;
        while (connectedPorts[i] != NULL)
        {
#if DEBUG_JACK_CAPTURE_CLIENT
            std::cout << "disconnecting from " << connectedPorts[i]
                      << std::endl;
#endif
            jack_port_disconnect(client, inPort);
            i++;
        }
    }

    m_capturePort = jack_port_by_name(client, portName);

#if DEBUG_JACK_CAPTURE_CLIENT
    std::cout << std::endl << "Recording from "
              << jack_port_name(m_capturePort) << std::endl;
#endif

    // Connect ports
    if (jack_connect( client, portName, jack_port_name(inPort) ) < 0 )
    {
        std::cout << "------------------------------" << std::endl
                  << "Jack Client: cant connect port" << std::endl
                  << "------------------------------" << std::endl
                  << std::endl;
    }

#if DEBUG_JACK_CAPTURE_CLIENT
    std::cout << "Port connected" << std::endl;
#endif
}


/*********************************
        REALTIME OPERATIONS
 ********************************/
int
JackCaptureClient::process(jack_nframes_t nframes, void *arg)
{
    JackCaptureClient *jcc = (JackCaptureClient*)arg;
    if ( !jcc->m_processing ) {
        return 0;
    }

    jack_default_audio_sample_t *inSamp = (jack_default_audio_sample_t *)
                                          jack_port_get_buffer(
                                              jcc->m_capturePort,
                                              nframes);

    //check space to write
    uint writeSpace = jack_ringbuffer_write_space( jcc->m_jackRingBuffer );
    int writeSamples = writeSpace / jcc->m_jackSampleSize;

#if DEBUG_JACK_CAPTURE_CLIENT
    std::cout << "Want to write " << nframes << "frames\t"
              << writeSamples << " available\n";
#endif


    if ( writeSpace < (jcc->m_jackSampleSize * nframes) ) {
        unsigned int advance = nframes - writeSamples;
        unsigned int advanceBytes = advance * jcc->m_jackSampleSize;
        jack_ringbuffer_read_advance( jcc->m_jackRingBuffer, advanceBytes );
#if DEBUG_JACK_CAPTURE_CLIENT
        std::cout << "Advancing read pointer " << advance << "frames, "
                  << advanceBytes << " bytes\n";
        writeSpace = jack_ringbuffer_write_space( jcc->m_jackRingBuffer );
        writeSamples = writeSpace / jcc->m_jackSampleSize;
        std::cout << writeSamples << " frames now available\n";
#endif
    }


    size_t written = jack_ringbuffer_write(jcc->m_jackRingBuffer,
                                           (char*)(inSamp),
                                           jcc->m_jackSampleSize * nframes);
#if DEBUG_JACK_CAPTURE_CLIENT
    std::cout << "I've written " << written / jcc->m_jackSampleSize << " frames\n";
#else
    (void) written; // stops warning about unused variable
#endif

#if DEBUG_JACK_CAPTURE_CLIENT
    uint readSpace = jack_ringbuffer_read_space( jcc->m_jackRingBuffer );
    uint readSamples = readSpace / jcc->m_jackSampleSize;
    std::cout << "Now " << readSamples << " samples available to read\n";
    std::cout << std::endl;
#endif

    return 0;
}


/*********************************
    CONTROL REALTIME OPERATIONS
 ********************************/
void
JackCaptureClient::startProcessing()
{
    if (m_isConnected) {
        m_processing = true;
    }
}

void
JackCaptureClient::stopProcessing()
{
    m_processing = false;
}

// Method will be called if Jack shuts process down
void
JackCaptureClient::jackShutdown(void *arg)
{
    (void)arg;
#if DEBUG_JACK_CAPTURE_CLIENT
//    JackCaptureClient *jcc = (JackCaptureClient*)arg;
    std::cout << "Shutdown by Jack!!!!!!!!!" << std::endl;
#endif
}


/*********************************
          GET SAMPLE DATA
 ********************************/
bool
JackCaptureClient::getFrame(float *frame, size_t captureSize)
{
    size_t availableSize = jack_ringbuffer_read_space(m_jackRingBuffer)
                           / m_jackSampleSize;
    // ensure a full chunk is available & processing is allowed
    if (captureSize <= availableSize)
    {
        size_t read = jack_ringbuffer_read(m_jackRingBuffer,
                                           (char*)(frame),
                                           (sizeof(float)*captureSize) );
        (void) read; // stops warning about unused variable
#if DEBUG_JACK_CAPTURE_CLIENT
        std::cout << "JackCaptureClient::getFrame - Got frame!\n";
#endif
        return true;
    }
    else {
#if DEBUG_JACK_CAPTURE_CLIENT
        std::cout << "JackCaptureClient::getFrame - "
                  << availableSize << " samples available. "
                  << captureSize << " samples wanted"
                  << std::endl;
#endif
        return false;
    }
}


} // end namespace

