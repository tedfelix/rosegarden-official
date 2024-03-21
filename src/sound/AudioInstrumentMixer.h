/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef AUDIOINSTRUMENTMIXER_H
#define AUDIOINSTRUMENTMIXER_H

#include "AudioProcess.h"

#include <vector>


namespace Rosegarden
{


class AudioFileReader;
class AudioFileWriter;
class RunnablePluginInstance;


class AudioInstrumentMixer : public AudioThread
{
public:
    typedef std::vector<RunnablePluginInstance *> PluginList;

    AudioInstrumentMixer(SoundDriver *driver,
                         AudioFileReader *fileReader,
                         unsigned int sampleRate,
                         unsigned int blockSize);

    ~AudioInstrumentMixer() override;

    /// Indirect Singleton.  Use carefully.
    /**
     * Instance is actually created in JackDriver.  But you can get to
     * it quickly through here.
     *
     * Please note that this CAN return nullptr.  It does not create
     * the instance.  That is done in JackDriver.  ALWAYS check what
     * you get from this function before using it.
     *
     * The issue is that AIM does not have a default ctor.  So the usual
     * "static instance in getInstance()" doesn't work.
     */
    static AudioInstrumentMixer *getInstance();

    void kick(bool wantLock = true);

    void setBussMixer(AudioBussMixer *mixer) { m_bussMixer = mixer; }

    void setPlugin(InstrumentId id, int position, const QString &identifier);
    void removePlugin(InstrumentId id, int position);
    void removeAllPlugins();

    void setPluginPortValue(InstrumentId id, int position,
                            unsigned int port, float value);
    float getPluginPortValue(InstrumentId id, int position,
                             unsigned int port);

    void setPluginBypass(InstrumentId, int position, bool bypass);

    QStringList getPluginPrograms(InstrumentId, int);
    QString getPluginProgram(InstrumentId, int);
    QString getPluginProgram(InstrumentId, int, int, int);
    unsigned long getPluginProgram(InstrumentId, int, QString);
    void setPluginProgram(InstrumentId, int, QString);

    QString configurePlugin(InstrumentId, int, QString, QString);
    void savePluginState();
    void getPluginPlayableAudio(std::vector<PlayableData*>& playable);

    void resetAllPlugins(bool discardEvents = false);
    void discardPluginEvents();
    void destroyAllPlugins();

    // Avoid these.  Use the above routines if possible.
    RunnablePluginInstance *getSynthPlugin(InstrumentId id)
            { return m_synths[id]; }
    RunnablePluginInstance *getPluginInstance(InstrumentId, int position);

    /**
     * Return the plugins intended for a particular buss.  (By coincidence,
     * this will also work for instruments, but it's not to be relied on.)
     * It's purely by historical accident that the instrument mixer happens
     * to hold buss plugins as well -- this could do with being refactored.
     */
    PluginList &getBussPlugins(unsigned int bussId) { return m_plugins[bussId]; }

    /**
     * Return the total of the plugin latencies for a given instrument
     * or buss id.
     */
    size_t getPluginLatency(unsigned int id);

    /**
     * Prebuffer.  This should be called only when the transport is
     * not running.
     */
    void fillBuffers(const RealTime &currentTime);

    /**
     * Ensure plugins etc have enough buffers.  This is also done by
     * fillBuffers and only needs to be called here if the extra work
     * involved in fillBuffers is not desirable.
     */
    void allocateBuffers();

    /**
     * Empty and discard buffer contents.
     */
    void emptyBuffers(RealTime currentTime = RealTime::zero());

    /**
     * An instrument is "empty" if it has no audio files, synths or
     * plugins assigned to it, and so cannot generate sound.  Empty
     * instruments can safely be ignored during playback.
     */
    bool isInstrumentEmpty(InstrumentId id) {
        return m_bufferMap[id].empty;
    }

    /**
     * An instrument is "dormant" if every readable sample on every
     * one of its buffers is zero.  Dormant instruments can safely be
     * skipped rather than mixed during playback, but they should not
     * be ignored (unless also empty).
     */
    bool isInstrumentDormant(InstrumentId id) {
        return m_bufferMap[id].dormant;
    }

    /**
     * We always have at least two channels (and hence buffers) by
     * this point, because even on a mono instrument we still have a
     * Pan setting which will have been applied by the time we get to
     * these buffers.
     */
    RingBuffer<sample_t, 2> *getRingBuffer(InstrumentId id, unsigned int channel) {
        if (channel < (unsigned int)m_bufferMap[id].buffers.size()) {
            return m_bufferMap[id].buffers[channel];
        } else {
            return nullptr;
        }
    }

    /// For call from MappedStudio.  Pan is in range -100.0 -> 100.0
    void setInstrumentLevels(InstrumentId id, float dB, float pan);

    /// For call regularly from anywhere in a non-RT thread
    void updateInstrumentMuteStates();

    /// called when audio processing is done
    void audioProcessingDone();

    sample_t* getAudioBuffer(InstrumentId id, unsigned int channel) const;

    unsigned int getNumSoftSynths() const {return m_numSoftSynths;};

protected:
    void threadRun() override;

    int getPriority() override { return 3; }

    void processBlocks(bool &readSomething);
    void processEmptyBlocks(InstrumentId id);
    bool processBlock(InstrumentId id, PlayableData **, size_t, bool &readSomething);
    void generateBuffers();

    AudioFileReader  *m_fileReader;
    AudioBussMixer   *m_bussMixer;
    size_t            m_blockSize;

    typedef std::map<InstrumentId, PluginList> PluginMap;
    typedef std::map<InstrumentId, RunnablePluginInstance *> SynthPluginMap;

    // Thread Safe?  Probably not.
    // The plugin maps will all be pre-sized in the ctor and so of
    // fixed size during normal run time; this will allow us to add
    // and edit plugins without locking.
    // ??? But we can end up with a race condition if some thread happens
    //     to get a pointer from here and then it is destroyed by another
    //     thread before the first thread tries to use it.  What guarantee
    //     do we have that this will never happen?
    //
    //     One possible guarantee is that the code is disciplined enough
    //     to stop using a plugin before it is destroyed.  That seems
    //     likely to be true.  And that might be why this works ok without
    //     any locking.
    // ??? I've seen a sigsegv (probably a race condition) due to garbage
    //     pointer values in this data structure at startup.  It only happened
    //     once and I've not seen it since.
    PluginMap m_plugins;
    SynthPluginMap m_synths;

    // maintain the same number of these as the maximum number of
    // channels on any audio instrument for each instrument
    typedef std::vector<sample_t *> ProcessBufferType;
    std::map<InstrumentId, ProcessBufferType> m_processBuffers;

    struct BufferRec
    {
        BufferRec() : empty(true), dormant(true), zeroFrames(0),
                      filledTo(RealTime::zero()), channels(2),
                      buffers(), gainLeft(0.0), gainRight(0.0), volume(0.0),
                      muted(false) { }
        ~BufferRec();

        bool empty;
        bool dormant;
        size_t zeroFrames;

        RealTime filledTo;
        size_t channels;
        std::vector<RingBuffer<sample_t, 2> *> buffers;

        float gainLeft;
        float gainRight;
        float volume;
        bool muted;
    };

    typedef std::map<InstrumentId, BufferRec> BufferMap;
    BufferMap m_bufferMap;
 private:
    unsigned int m_numSoftSynths;
};


}

#endif
