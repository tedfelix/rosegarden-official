// -*- c-basic-offset: 4 -*-

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

#ifndef _JACKDRIVER_H_
#define _JACKDRIVER_H_

#include "config.h"

#ifdef HAVE_ALSA
#ifdef HAVE_LIBJACK

#ifdef HAVE_LADSPA
#include "LADSPAPluginInstance.h"
#endif

#include <jack/jack.h>
#include "SoundDriver.h"
#include "Instrument.h"
#include "RealTime.h"
#include "ExternalTransport.h"

namespace Rosegarden
{

class AlsaDriver;
class AudioMixer;
class AudioFileReader;
class AudioFileWriter;

class JackDriver
{
public:
    // convenience
    typedef jack_default_audio_sample_t sample_t;

    JackDriver(AlsaDriver *alsaDriver);
    virtual ~JackDriver();

    bool isOK() const { return m_ok; }

    bool isTransportEnabled() { return m_jackTransportEnabled; }
    bool isTransportMaster () { return m_jackTransportMaster; }

    void setTransportEnabled(bool e) { m_jackTransportEnabled = e; }
    void setTransportMaster (bool m) { m_jackTransportMaster  = m; }

    // start returns false if sound driver should wait for it to call
    // back on startClocksApproved before starting clocks
    bool start();
    void stop();

    RealTime getAudioPlayLatency() const { return m_audioPlayLatency; }
    RealTime getAudioRecordLatency() const { return m_audioRecordLatency; }

    // Plugin instance management
    //
    virtual void setPluginInstance(InstrumentId id,
                                   unsigned long pluginId,
                                   int position);

    virtual void removePluginInstance(InstrumentId id, int position);

    // Remove all plugin instances
    //
    virtual void removePluginInstances();

    virtual void setPluginInstancePortValue(InstrumentId id,
                                            int position,
                                            unsigned long portNumber,
                                            float value);

    virtual void setPluginInstanceBypass(InstrumentId id,
                                         int position,
                                         bool value);

    virtual unsigned int getSampleRate() const { return m_sampleRate; }
    virtual unsigned int getBufferSize() const { return m_bufferSize; }

    // A new audio file for storage of our recorded samples - the
    // file stays open so we can append samples at will.  We must
    // explicitly close the file eventually though to make sure
    // the integrity is correct (sample sizes must be written).
    //
    bool createRecordFile(const std::string &fileName);
    bool closeRecordFile(AudioFileId &returnedId);

    // Locks used by the disk thread and mix thread.  The AlsaDriver
    // should hold these locks whenever it wants to modify its audio
    // play queue -- at least when adding or removing files or
    // resetting status; it doesn't need to hold the locks when
    // incrementing their statuses or simply reading them.

    int getAudioQueueLocks();
    int tryAudioQueueLocks();
    int releaseAudioQueueLocks();

    void prebufferAudio();
    void flushAudio(); // when stopping
    void kickAudio();

    RealTime getNextSliceStart(const RealTime &now) const;

protected:

    // static methods for JACK process thread:
    static int   jackProcessStatic(jack_nframes_t nframes, void *arg);
    static int   jackBufferSize(jack_nframes_t nframes, void *arg);
    static int   jackSampleRate(jack_nframes_t nframes, void *arg);
    static void  jackShutdown(void *arg);
    static int   jackGraphOrder(void *);
    static int   jackXRun(void *);

    // static JACK transport callbacks
    static int   jackSyncCallback(jack_transport_state_t,
				  jack_position_t *, void *);
    static int   jackTimebaseCallback(jack_transport_state_t,
				      jack_nframes_t,
				      jack_position_t *,
				      int,
				      void *);

    // jackProcessStatic delegates to this
    int          jackProcess(jack_nframes_t nframes);

    // other helper methods:

    void initialise();

    // Create a set of JACK input ports (and for the moment)
    // we do default connections to JACK terminal ports.
    //
    // If the deactivate flag is set then we deactivate and reactivate
    // the client when modifying the number of input ports.
    //
    void createInputPorts(unsigned int ports, bool deactivate);

    // data members:

    jack_client_t               *m_client;

    std::vector<jack_port_t *>   m_inputPorts;
    std::vector<jack_port_t *>   m_outputInstruments;
    std::vector<jack_port_t *>   m_outputSubmasters;
    std::vector<jack_port_t *>   m_outputMasters;

    jack_nframes_t               m_bufferSize;
    jack_nframes_t               m_sampleRate;

    sample_t                    *m_tempOutBuffer;

    bool                         m_jackTransportEnabled;
    bool                         m_jackTransportMaster;

    RealTime                     m_audioPlayLatency;
    RealTime                     m_audioRecordLatency;

    bool                         m_waiting;
    jack_transport_state_t       m_waitingState;
    ExternalTransport::TransportToken m_waitingToken;

    AudioMixer                  *m_mixer;
    AudioFileReader             *m_fileReader;
    AudioFileWriter             *m_fileWriter;
    AlsaDriver                  *m_alsaDriver;

    bool                         m_ok;
};


}

#endif
#endif

#endif

