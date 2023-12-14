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
#include <list>
#include <set>
#include <map>
#include <QString>

#include <lilv/lilv.h>
#include <lv2/atom/atom.h>
#include <lv2/options/options.h>

#include <alsa/seq_event.h>
#include <alsa/seq_midi_event.h>
#include "sound/LV2Utils.h"
#include "sound/PluginPortConnection.h"

#include "base/Instrument.h"
#include "RingBuffer.h"

#include "RunnablePluginInstance.h"

namespace Rosegarden
{

class AudioInstrumentMixer;

// LV2 plugin instance.  LV2 is a variable block size API, but
// for one reason and another it's more convenient to use a fixed
// block size in this wrapper.
//
class LV2PluginInstance : public RunnablePluginInstance
{
public:
    ~LV2PluginInstance() override;

    bool isOK() const override { return m_instance != nullptr; }

    InstrumentId getInstrument() const { return m_instrument; }
    QString getIdentifier() const override { return m_identifier; }
    int getPosition() const { return m_position; }

    void run(const RealTime &rt) override;

    void setPortValue(unsigned int portNumber, float value) override;
    void setPortValue(const char *port_symbol,
                      const void *value,
                      uint32_t size,
                      uint32_t type);

    void setPortByteArray(unsigned int port,
                          unsigned int protocol,
                          const QByteArray& ba);
    float getPortValue(unsigned int portNumber) override;
    void* getPortValue(const char *port_symbol,
                       uint32_t *size,
                       uint32_t *type);

    QString configure(const QString& key, const QString& value) override;
    void savePluginState() override;

    void sendEvent(const RealTime& eventTime,
                   const void* event) override;

    size_t getBufferSize() override { return m_blockSize; }
    size_t getAudioInputCount() override { return m_audioPortsIn.size(); }
    size_t getAudioOutputCount() override { return m_audioPortsOut.size(); }
    sample_t **getAudioInputBuffers() override { return m_inputBuffers; }
    sample_t **getAudioOutputBuffers() override { return m_outputBuffers; }

    bool isBypassed() const override { return m_bypassed; }
    void setBypassed(bool bypassed) override { m_bypassed = bypassed; }

    size_t getLatency() override;

    void silence() override;
    void discardEvents() override;
    void setIdealChannelCount(size_t channels) override; // may re-instantiate

    int numInstances() const;

    void runWork(uint32_t size,
                 const void* data,
                 LV2_Worker_Respond_Function resp);

    void getControlInValues(std::map<int, float>& controlValues);
    void getControlOutValues(std::map<int, float>& controlValues);

    const LV2_Descriptor* getLV2Descriptor() const;

    LV2_Handle getHandle() const;

    int getSampleRate() const;

    virtual void audioProcessingDone() override;

    void getConnections(PluginPortConnection::ConnectionList& clist) const;
    void setConnections(const PluginPortConnection::ConnectionList& clist);

protected:
    // To be constructed only by LV2PluginFactory
    friend class LV2PluginFactory;

    // Constructor that creates the buffers internally
    //
    LV2PluginInstance(PluginFactory *factory,
                      InstrumentId instrument,
                      const QString& identifier,
                      int position,
                      unsigned long sampleRate,
                      size_t blockSize,
                      int idealChannelCount,
                      const QString& uri,
                      AudioInstrumentMixer* amixer);

    void init(int idealChannelCount = 0);
    void instantiate(unsigned long sampleRate);
    void cleanup();
    void activate();
    void deactivate();

    // Connection of data (and behind the scenes control) ports
    //
    void connectPorts();

 private:

    void sendMidiData(const QByteArray& rawMidi,
                      size_t frameOffset);

    std::map<int, float> m_controlPortsIn;
    std::map<int, float> m_controlPortsOut;

    struct AtomPort
    {
        unsigned int index;
        LV2_Atom_Sequence* atomSeq;
        bool isMidi;
    };

    std::list<AtomPort> m_atomInputPorts;
    std::list<AtomPort> m_atomOutputPorts;

    InstrumentId m_instrument;
    int m_position;
    LilvInstance* m_instance;
    QString m_uri;
    const LilvPlugin *m_plugin;
    LV2Utils::LV2PluginData m_pluginData;

    std::vector<int> m_audioPortsIn;
    std::vector<int> m_audioPortsOut;
    size_t m_channelCount;

    struct MidiEvent
    {
        RealTime time;
        QByteArray data;
    };

    std::list<MidiEvent> m_eventBuffer;
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
    LV2_Feature m_workerFeature;
    LV2_Feature m_optionsFeature;
    std::vector<LV2_Options_Option> m_options;
    LV2_Worker_Schedule m_workerSchedule;
    LV2_Worker_Interface* m_workerInterface;
    LV2Utils::PluginPosition m_workerHandle;
    std::vector<LV2_Feature*> m_features;

    bool m_distributeChannels;
    LV2_URID m_atomTransferUrid;
    bool m_pluginHasRun;
    AudioInstrumentMixer* m_amixer;
    PluginPortConnection::ConnectionList m_connections;
};

}

#endif // RG_LV2PLUGININSTANCE_H
