/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2023 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef PLUGINAUDIOSOURCE_H
#define PLUGINAUDIOSOURCE_H

#include "PlayableData.h"

#include <queue>

namespace Rosegarden
{

class AudioFile;
class RunnablePluginInstance;

 class PluginAudioSource : public PlayableData
{
 public:
    PluginAudioSource(RunnablePluginInstance* plugin,
                      InstrumentId instrument,
                      int portIndex,
                      int channel,
                      size_t blockSize);
    ~PluginAudioSource();

    virtual size_t getSampleFramesAvailable() override;

    virtual bool isFullyBuffered() const override;

    virtual bool isBuffered() const override;

    virtual RealTime getStartTime() const override;

    virtual RealTime getEndTime() const override;

    virtual RealTime getDuration() const override;

    virtual size_t addSamples
        (std::vector<sample_t *> &target,
         size_t channels,
         size_t nframes,
         size_t offset = 0) override;

    virtual void clearBuffers() override;

    virtual bool fillBuffers(const RealTime &currentTime) override;

    virtual bool updateBuffers() override;

    virtual InstrumentId getInstrument() const override;

    virtual AudioFile* getAudioFile() const override;

    virtual void cancel() override;

    virtual int getRuntimeSegmentId() const override;

    virtual bool isSmallFile() const override;

    virtual unsigned int getTargetChannels() const override;

    void setAudioData(sample_t* data);

    void pluginFinished();

    void deactivate() override;

 private:
    RunnablePluginInstance* m_plugin;
    InstrumentId m_instrument;
    int m_portIndex;
    int m_channel;
    size_t m_blockSize;
    std::queue<sample_t*> m_dataQueue;
    RealTime m_duration;
};

}

#endif
