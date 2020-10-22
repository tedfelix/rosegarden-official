/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_JACKDRIVER_H
#define RG_JACKDRIVER_H

#ifdef HAVE_ALSA
#ifdef HAVE_LIBJACK

#include "RunnablePluginInstance.h"
#include <jack/jack.h>
#include "SoundDriver.h"
#include "base/Instrument.h"
#include "base/RealTime.h"
#include "sequencer/RosegardenSequencer.h"

#include <QStringList>

namespace Rosegarden
{

class AlsaDriver;
class AudioBussMixer;
class AudioInstrumentMixer;
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
    bool isTransportSource () { return m_jackTransportSource; }

    void setTransportEnabled(bool e) { m_jackTransportEnabled = e; }
    void setTransportSource (bool m) { m_jackTransportSource  = m; }

    // These methods call back on the sound driver if necessary to
    // establish the current transport location to start at or
    // relocate to.  startTransport and relocateTransport return true
    // if they have completed and the sound driver can safely call
    // startClocks; false if the sound driver should wait for the JACK
    // driver to call back on startClocksApproved before starting.
    bool startTransport();
    bool relocateTransport();
    void stopTransport();

    RealTime getAudioPlayLatency() const;
    RealTime getAudioRecordLatency() const;
    RealTime getInstrumentPlayLatency(InstrumentId) const;
    RealTime getMaximumPlayLatency() const;

    // Plugin instance management
    //
    virtual void setPluginInstance(InstrumentId id,
                                   QString identifier,
                                   int position);

    virtual void removePluginInstance(InstrumentId id, int position);

    virtual void setPluginInstancePortValue(InstrumentId id,
                                            int position,
                                            unsigned long portNumber,
                                            float value);

    virtual float getPluginInstancePortValue(InstrumentId id,
                                             int position,
                                             unsigned long portNumber);

    virtual void setPluginInstanceBypass(InstrumentId id,
                                         int position,
                                         bool value);

    virtual QStringList getPluginInstancePrograms(InstrumentId id,
                                                  int position); 

    virtual QString getPluginInstanceProgram(InstrumentId id,
                                             int position); 
  
    virtual QString getPluginInstanceProgram(InstrumentId id,
                                             int position,
                                             int bank,
                                             int program); 
  
    virtual unsigned long getPluginInstanceProgram(InstrumentId id,
                                                   int position,
                                                   QString name);
  
    virtual void setPluginInstanceProgram(InstrumentId id,
                                          int position,
                                          QString program);

    virtual QString configurePlugin(InstrumentId id,
                                    int position, 
                                    QString key, QString value);

    virtual RunnablePluginInstance *getSynthPlugin(InstrumentId id);

    virtual void clearSynthPluginEvents(); // when stopping

    virtual unsigned int getSampleRate() const { return m_sampleRate; }
    virtual unsigned int getBufferSize() const { return m_bufferSize; }

    // A new audio file for storage of our recorded samples - the
    // file stays open so we can append samples at will.  We must
    // explicitly close the file eventually though to make sure
    // the integrity is correct (sample sizes must be written).
    //
    bool openRecordFile(InstrumentId id,
                        const QString &fileName);
    bool closeRecordFile(InstrumentId id,
                         AudioFileId &returnedId);

    // Set or change the number of audio inputs and outputs.
    // The first of these is slightly misnamed -- the submasters
    // argument controls the number of busses, not ports (which
    // may or may not exist depending on the setAudioPorts call).
    //
    void setAudioPorts(bool faderOuts, bool submasterOuts);

    // Locks used by the disk thread and mix thread.  The AlsaDriver
    // should hold these locks whenever it wants to modify its audio
    // play queue -- at least when adding or removing files or
    // resetting status; it doesn't need to hold the locks when
    // incrementing their statuses or simply reading them.
    //
    int getAudioQueueLocks();
    int tryAudioQueueLocks();
    int releaseAudioQueueLocks();

    void prepareAudio(); // when repositioning etc
    void prebufferAudio(); // when starting playback (incorporates prepareAudio)
    void kickAudio(); // for paranoia only

    // Because we don't want to do any lookups that might involve
    // locking etc from within the JACK process thread, we instead
    // call this regularly from the ALSA driver thread -- it looks up
    // various bits of data such as the master fader and monitoring
    // levels, number of inputs etc and either processes them or
    // writes them into simple records in the JACK driver for process
    // to read.  Actually quite a lot of work.
    //
    void updateAudioData();

    // Similarly, set data on the buss mixer to avoid the buss mixer
    // having to call back on the mapped studio to discover it
    // 
    void setAudioBussLevels(int bussNo, float dB, float pan);

    // Likewise for instrument mixer
    // 
    void setAudioInstrumentLevels(InstrumentId instrument, float dB, float pan);

    // Called from AlsaDriver to indicate that an async MIDI event is
    // being sent to a soft synth.  JackDriver uses this to suggest
    // that it needs to start processing soft synths, if it wasn't
    // already.  It will switch this off again itself when things
    // fall silent.
    // 
    void setHaveAsyncAudioEvent() { m_haveAsyncAudioEvent = true; }

    RealTime getNextSliceStart(const RealTime &now) const;

    // For audit purposes only.
    size_t getFramesProcessed() const { return m_framesProcessed; }

    // Reinitialise if we've been kicked off JACK -- if we can
    // 
    void restoreIfRestorable();

    // Report back to GUI via the AlsaDriver
    //
    void reportFailure(MappedEvent::FailureCode code);

protected:

    // static methods for JACK process thread:
    static int   jackProcessStatic(jack_nframes_t nframes, void *arg);
    static int   jackBufferSize(jack_nframes_t nframes, void *arg);
    static int   jackSampleRate(jack_nframes_t nframes, void *arg);
    static void  jackShutdown(void *arg);
    static int   jackXRun(void *);

    // static JACK transport callbacks
    static int   jackSyncCallback(jack_transport_state_t,
                                  jack_position_t *, void *);
//    static int   jackTimebaseCallback(jack_transport_state_t,
//                                      jack_nframes_t,
//                                      jack_position_t *,
//                                      int,
//                                      void *);

    // jackProcessStatic delegates to this
    int          jackProcess(jack_nframes_t nframes);
    int          jackProcessRecord(InstrumentId id,
                                   jack_nframes_t nframes,
                                   sample_t *, sample_t *, bool);
    // write silence to all ports
    int          jackProcessEmpty(jack_nframes_t nframes);

    // other helper methods:

    void initialise(bool reinitialise = false);

    bool createMainOutputs();
    bool createFaderOutputs(int audioPairs, int synthPairs);
    bool createSubmasterOutputs(int pairs);
    bool createRecordInputs(int pairs);

    bool relocateTransportInternal(bool alsoStart);

    // data members:

    // Client handle from jack_client_open()
    jack_client_t               *m_client;

    std::vector<jack_port_t *>   m_inputPorts;
    std::vector<jack_port_t *>   m_outputInstruments;
    std::vector<jack_port_t *>   m_outputSubmasters;
    std::vector<jack_port_t *>   m_outputMonitors;
    std::vector<jack_port_t *>   m_outputMasters;

    jack_nframes_t               m_bufferSize;
    jack_nframes_t               m_sampleRate;

    sample_t                    *m_tempOutBuffer;

    bool                         m_jackTransportEnabled;
    bool                         m_jackTransportSource;

    bool                         m_waiting;
    jack_transport_state_t       m_waitingState;
    RosegardenSequencer::TransportToken m_waitingToken;
    int                          m_ignoreProcessTransportCount;

    AudioBussMixer              *m_bussMixer;
    AudioInstrumentMixer        *m_instrumentMixer;
    AudioFileReader             *m_fileReader;
    AudioFileWriter             *m_fileWriter;
    AlsaDriver                  *m_alsaDriver;

    float                        m_masterLevel;
    unsigned long                m_directMasterAudioInstruments; // bitmap
    unsigned long                m_directMasterSynthInstruments;
    std::map<InstrumentId, RealTime> m_instrumentLatencies;
    RealTime                     m_maxInstrumentLatency;
    bool                         m_haveAsyncAudioEvent;

    struct RecordInputDesc {
        int   input;
        int   channel;
        float level;
        RecordInputDesc(int i = 1000, int c = -1, float l = 0.0f) :
            input(i), channel(c), level(l) { }
    };
    typedef std::map<InstrumentId, RecordInputDesc> RecordInputMap;
    RecordInputMap               m_recordInputs;

    time_t                       m_kickedOutAt;
    size_t                       m_framesProcessed;

    // initialise() has completed successfully, and there are no other issues
    bool                         m_ok;
};


}

#endif
#endif

#endif

