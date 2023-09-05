/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_LV2PLUGININSTANCE_H
#define RG_LV2PLUGININSTANCE_H

#include <vector>
#include <set>
#include <map>
#include <QString>

#include <lilv/lilv.h>
#include <lv2/atom/atom.h>

#include <alsa/seq_event.h>
#include <alsa/seq_midi_event.h>
#include "sound/LV2Utils.h"

#include "base/Instrument.h"
#include "RingBuffer.h"

#include "RunnablePluginInstance.h"

namespace Rosegarden
{

// LV2 plugin instance.  LV2 is a variable block size API, but
// for one reason and another it's more convenient to use a fixed
// block size in this wrapper.
//
class LV2PluginInstance : public RunnablePluginInstance
{
public:
    ~LV2PluginInstance() override;

    bool isOK() const override { return !m_instances.empty(); }

    InstrumentId getInstrument() const { return m_instrument; }
    QString getIdentifier() const override { return m_identifier; }
    int getPosition() const { return m_position; }

    void run(const RealTime &rt) override;

    void setPortValue(unsigned int portNumber, float value) override;
    // !!!!
    void setPortByteArray(unsigned int port, const QByteArray& ba);
    float getPortValue(unsigned int portNumber) override;

    void sendEvent(const RealTime& eventTime,
                   const void* event) override;

    size_t getBufferSize() override { return m_blockSize; }
    size_t getAudioInputCount() override { return m_instanceCount * m_audioPortsIn.size(); }
    size_t getAudioOutputCount() override { return m_instanceCount * m_audioPortsOut.size(); }
    sample_t **getAudioInputBuffers() override { return m_inputBuffers; }
    sample_t **getAudioOutputBuffers() override { return m_outputBuffers; }

    bool isBypassed() const override { return m_bypassed; }
    void setBypassed(bool bypassed) override { m_bypassed = bypassed; }

    size_t getLatency() override;

    void silence() override;
    void discardEvents() override;
    void setIdealChannelCount(size_t channels) override; // may re-instantiate

    int numInstances() const;
protected:
    // To be constructed only by LV2PluginFactory
    friend class LV2PluginFactory;

    // Constructor that creates the buffers internally
    //
    LV2PluginInstance(PluginFactory *factory,
                      InstrumentId instrument,
                      QString identifier,
                      int position,
                      unsigned long sampleRate,
                      size_t blockSize,
                      int idealChannelCount,
                      const QString& uri);

    void init(int idealChannelCount = 0);
    void instantiate(unsigned long sampleRate);
    void cleanup();
    void activate();
    void deactivate();

    // Connection of data (and behind the scenes control) ports
    //
    void connectPorts();

 private:

    struct InstanceData
    {
        std::vector<std::pair<unsigned long, float> > controlPortsIn;
        std::vector<std::pair<unsigned long, float> > controlPortsOut;
    };

    InstrumentId   m_instrument;
    int                        m_position;
    std::vector<LilvInstance*> m_instances;
    std::vector<InstanceData>  m_instanceData;
    size_t                     m_instanceCount;
    QString m_uri;
    const LilvPlugin *m_plugin;
    LV2Utils::LV2PluginData m_pluginData;

    std::vector<int>          m_audioPortsIn;
    std::vector<int>          m_audioPortsOut;

    int m_midiPort;
    std::map<RealTime, snd_seq_event_t> m_eventBuffer;
    std::vector<LV2_Atom_Sequence*> m_midiIn;
    snd_midi_event_t *m_midiParser;
    LV2_URID m_midiEventUrid;

    size_t                    m_blockSize;
    sample_t                **m_inputBuffers;
    sample_t                **m_outputBuffers;
    size_t                    m_sampleRate;
    float                    *m_latencyPort;
    bool                      m_run;

    bool                      m_bypassed;

    LV2_Feature m_uridMapFeature;
    LV2_Feature m_uridUnmapFeature;

    std::vector<LV2_Feature*> m_features;
};

}

#endif // RG_LV2PLUGININSTANCE_H
