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

#define RG_MODULE_STRING "[PluginAudioSource]"
#define RG_NO_DEBUG_PRINT 1

#include "PluginAudioSource.h"

#include "misc/Debug.h"
#include "base/Composition.h"
#include "document/RosegardenDocument.h"
#include "sound/RunnablePluginInstance.h"

namespace Rosegarden
{

    PluginAudioSource::PluginAudioSource(RunnablePluginInstance* plugin,
                                         InstrumentId instrument,
                                         int portIndex,
                                         size_t blockSize) :
        m_plugin(plugin),
        m_instrument(instrument),
        m_portIndex(portIndex),
        m_blockSize(blockSize)
{
    RG_DEBUG << "PluginAudioSource ctor";
    Composition& comp = RosegardenDocument::currentDocument->getComposition();
    timeT duration = comp.getDuration();
    m_duration = comp.getElapsedRealTime(duration);
}

PluginAudioSource::~PluginAudioSource()
{
    RG_DEBUG << "PluginAudioSource dtor";
    // clean up any remaining audio data
    while (! m_dataQueue.empty()) {
        sample_t* block = m_dataQueue.front();
        m_dataQueue.pop();
        delete[] block;
    }
}

size_t PluginAudioSource::getSampleFramesAvailable()
{
    RG_DEBUG << "getSampleFramesAvailable";
    return ! m_dataQueue.empty();
}

bool PluginAudioSource::isFullyBuffered() const
{
    //RG_DEBUG << "isFullyBuffered";
    return true;
}

bool PluginAudioSource::isBuffered() const
{
    //RG_DEBUG << "isBuffered";
    return false;
}

RealTime PluginAudioSource::getStartTime() const
{
    //RG_DEBUG << "getStartTime";
    return RealTime(0, 0);
}

RealTime PluginAudioSource::getEndTime() const
{
    //RG_DEBUG << "getEndTime";
    return getStartTime() + getDuration();
}

RealTime PluginAudioSource::getDuration() const
{
    //RG_DEBUG << "getDuration";
    return m_duration;
}

size_t PluginAudioSource::addSamples
(std::vector<sample_t *> &target,
 size_t channels,
 size_t nframes,
 size_t offset)
{
    RG_DEBUG << "addSamples" << target.size() << channels << nframes <<
        offset << m_dataQueue.size();
    // for PluginAudioSource the offset should always be 0
    Q_ASSERT(offset == 0);
    // for PluginAudioSource the nframes should always be the block size
    Q_ASSERT(nframes == m_blockSize);
    if (m_dataQueue.empty()) return 0;
    sample_t* block = m_dataQueue.front();
    for (unsigned int ch = 0; ch < channels; ++ch) {
        double ms = 0;
        for (unsigned int it=0; it < m_blockSize; it++) {
            ms += block[it] * block[it];
            target[ch][it] += block[it];
        }
        RG_DEBUG << "ms: " << ms;
    }
    m_dataQueue.pop();
    delete[] block;

    return nframes;
}

void PluginAudioSource::clearBuffers()
{
    RG_DEBUG << "clearBuffers";
}

    bool PluginAudioSource::fillBuffers(const RealTime & /* currentTime */)
{
    //RG_DEBUG << "fillBuffers" << currentTime;
    return true;
}

bool PluginAudioSource::updateBuffers()
{
    RG_DEBUG << "updateBuffers";
    return false;
}

InstrumentId PluginAudioSource::getInstrument() const
{
    //RG_DEBUG << "getInstrument";
    return m_instrument;
}

AudioFile* PluginAudioSource::getAudioFile() const
{
    RG_DEBUG << "getAudioFile";
    return nullptr;
}

void PluginAudioSource::cancel()
{
    RG_DEBUG << "cancel";
}

int PluginAudioSource::getRuntimeSegmentId() const
{
    RG_DEBUG << "getRuntimeSegmentId";
    return -1;
}

bool PluginAudioSource::isSmallFile() const
{
    //RG_DEBUG << "isSmallFile";
    return true;
}

unsigned int PluginAudioSource::getTargetChannels() const
{
    RG_DEBUG << "getTargetChannels";
    return 1;
}

void PluginAudioSource::setAudioData(sample_t* data)
{
    RG_DEBUG << "setAudioData";
    // copy the data and queue it
    sample_t* block = new sample_t[m_blockSize];
    memcpy(block, data, m_blockSize * sizeof(sample_t));
    m_dataQueue.push(block);
}

void PluginAudioSource::pluginFinished()
{
    RG_DEBUG << "pluginFinished";
    // the plugin is gone
    m_plugin = nullptr;
}

void PluginAudioSource::deactivate()
{
    // tell the plugin we are inactive
    if (m_plugin) m_plugin->removeAudioSource(m_portIndex);
}

}
