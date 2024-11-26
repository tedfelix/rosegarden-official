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

#ifndef RG_LV2PLUGININSTANCE_H
#define RG_LV2PLUGININSTANCE_H

#include "sound/LV2Utils.h"
#include "sound/LV2PluginDatabase.h"
#include "sound/LV2PluginParameter.h"
#include "base/AudioPluginInstance.h"

#include "base/Instrument.h"

#include "RunnablePluginInstance.h"

#include <lv2/options/options.h>

#include <alsa/seq_event.h>
#include <alsa/seq_midi_event.h>  // for snd_midi_event_t

#include <QString>
#include <QMutex>

#include <vector>
#include <map>
#include <list>


namespace Rosegarden
{


class AudioInstrumentMixer;
class PluginAudioSource;

/// LV2 Plugin Instance
/**
 * ??? This is pretty big.  Consider pulling out broad sets of functionality
 *     into classes and using containment to bring them back in.
 *     E.g. "LV2Ports ports;".
 *
 * LV2 is a variable block size API, but for one reason or another it's more
 * convenient to use a fixed block size in this wrapper.
 */
class LV2PluginInstance : public RunnablePluginInstance
{
public:
    ~LV2PluginInstance() override;

    bool isOK() const override { return m_instance != nullptr; }

    //InstrumentId getInstrument() const { return m_instrument; }
    QString getIdentifier() const override { return m_identifier; }
    //int getPosition() const { return m_position; }

    void run(const RealTime &rt) override;

    void setPortValue(unsigned int portNumber, float value) override;
    /// Used privately in .cpp so has to be public.
    void setPortValue(const char *port_symbol,
                      const void *value,
                      uint32_t size,
                      uint32_t type);

    void setPortByteArray(unsigned int port,
                          unsigned int protocol,
                          const QByteArray& ba) const;
    float getPortValue(unsigned int portNumber) override;
    /// Used privately in .cpp so has to be public.
    void* getPortValue(const char *port_symbol,
                       uint32_t *size,
                       uint32_t *type);

    QString configure(const QString& key, const QString& value) override;
    void savePluginState() override;
    void getPluginPlayableAudio(std::vector<PlayableData*>& playable) override;
    void removeAudioSource(int portIndex) override;

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

    void runWork(uint32_t size,
                 const void* data,
                 LV2_Worker_Respond_Function resp);

    /// Get m_controlPortsIn.
    void getControlInValues(LV2Utils::PortValues &controlValues) const;
    /// Get m_controlPortsOut.
    void getControlOutValues(LV2Utils::PortValues &controlValues) const;

    const LV2_Descriptor* getLV2Descriptor() const;

    LV2_Handle getHandle() const;

    int getSampleRate() const;

    virtual void audioProcessingDone() override;

    void getConnections(PluginPort::ConnectionList& clist) const;
    void setConnections(const PluginPort::ConnectionList& clist);

    bool hasParameters() const;
    void getParameters(AudioPluginInstance::PluginParameters& params) const;
    void updatePluginParameter
        (const QString& paramId,
         const AudioPluginInstance::PluginParameter& param);
    void sendPluginParameter
        (const LV2PluginParameter& lparam);

    void getPresets(AudioPluginInstance::PluginPresetList& presets) const;
    void setPreset(const QString& uri);
    void loadPreset(const QString& file);
    void savePreset(const QString& file);


    void setGUI(AudioPluginLV2GUI *gui)  { m_gui = gui; }

    /// Get plugin port updates from the queue and send to the UI.
    void triggerPortUpdates();

private:
    // To be constructed only by LV2PluginFactory
    friend class LV2PluginFactory;

    // Constructor that creates the buffers internally
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

    void sendMidiData(const QByteArray& rawMidi,
                      size_t frameOffset) const;

    void setupFeatures();

    /// GUI -> plugin control values.
    /**
     * Thread Safe.  These are created by init() and the map is not touched
     * after that.  Only the elements are touched.  Since reading/writing
     * floats is thread safe and the map never changes, this is thread safe.
     *
     * Pointers to the elements are passed to the plugin in connectPorts().
     */
    LV2Utils::PortValues m_controlPortsIn;

    /// plugin -> GUI control values.
    /**
     * Thread Safe.  These are created by init() and the map is not touched
     * after that.  Only the elements are touched.  Since reading/writing
     * floats is thread safe and the map never changes, this is thread safe.
     *
     * Pointers to the elements are passed to the plugin in connectPorts().
     */
    LV2Utils::PortValues m_controlPortsOut;

    struct AtomPort
    {
        unsigned int index;
        LV2_Atom_Sequence* atomSeq;
        bool isMidi;
        bool isPatch;
    };

    /**
     * init() creates these and then the vector is never changed.
     * connect() hooks these up to the plugin so it can read from them.
     * run() just clears these.
     *
     * The GUI and the sequencer write to atomSeq using
     * lv2_atom_sequence_append_event(). This is not thread safe.
     */
    std::vector<AtomPort> m_atomInputPorts;
    mutable QMutex m_atomInputMutex;
    /// Port updates come through here on their way to m_portValueQueue.
    /**
     * Thread Safe.  This is only touched by the audio thread.  Plugin
     * writes directly through the pointers it gets in connect().  Then
     * run() reads from it and transfers contents to m_portValueQueue.
     */
    std::vector<AtomPort> m_atomOutputPorts;

    InstrumentId m_instrument;
    // Position in the effects stack for effect plugins, 999 for synths.
    int m_position;

    LilvInstance* m_instance;
    QString m_uri;
    const LilvPlugin *m_plugin;
    LV2PluginDatabase::LV2PluginData m_pluginData;

    /**
     * We only keep the GUI pointer to support port updates from the plugin
     * to the GUI.  See triggerPortUpdates().
     */
    AudioPluginLV2GUI *m_gui{nullptr};

    std::vector<int> m_audioPortsIn;
    std::vector<int> m_audioPortsOut;
    size_t m_channelCount;

    struct MidiEvent
    {
        RealTime time;
        QByteArray data;
    };

    std::list<MidiEvent> m_eventBuffer;
    QMutex m_eventBufferMutex;

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
    LV2_Feature m_boundedBlockLengthFeature;
    std::vector<LV2_Options_Option> m_options;
    LV2_Worker_Schedule m_workerSchedule;
    LV2_Worker_Interface* m_workerInterface;
    LV2Utils::PluginPosition m_workerHandle;
    std::vector<LV2_Feature*> m_features;

    bool m_distributeChannels;
    LV2_URID m_atomTransferUrid;
    bool m_pluginHasRun;
    AudioInstrumentMixer* m_amixer;
    PluginPort::ConnectionList m_connections;
    LV2PluginParameter::Parameters m_params;
    std::string m_profilerName;

    /// Shift the next event 1 clock into the future.
    /**
     * This was a fix for missing notes at the beginning of a loop from
     * 2/6/2024.  [427db5e0]  Yoshimi and Dexed were the plugins that
     * had this problem.  They appear to sort the events such that the
     * "all notes off" is always last even if it arrives first.
     *
     * ??? This one is interesting and might need some more analysis.
     *     it appeared to be protected by a mutex, but some analysis
     *     and experimentation led to removing the mutex.  Seems to
     *     loop ok without the mutex.
     */
    bool m_eventsDiscarded;

    AudioPluginInstance::PluginPresetList m_presets;
    std::map<int, PluginAudioSource*> m_audioSources;

    // *** Port Value Queue

    /// A port index/value pair.
    struct PortValueItem
    {
        PortValueItem()  { }
        ~PortValueItem();
        // Avoid double-delete.  If we need to pass these around,
        // consider making valueAtom a shared_ptr.
        PortValueItem(const PortValueItem &) = delete;
        PortValueItem &operator=(const PortValueItem &) = delete;

        int portIndex{0};

        const LV2_Atom* valueAtom{nullptr};
    };
    typedef std::queue<PortValueItem *> PortValueQueue;

    /// Port index/value pairs sent from the plugin to the GUI.
    /**
     * updatePortValue() adds items to this.
     * triggerPortUpdates() takes items off of this.
     */
    PortValueQueue m_portValueQueue;
    QMutex m_portValueQueueMutex;

    /// Add a plugin port update to the queue for the UI.
    /**
     * Called by the plugin (from the audio thread) to update the UI.
     */
    void updatePortValue(int index, const LV2_Atom *value);

    LilvState* m_pluginState;

};


}

#endif // RG_LV2PLUGININSTANCE_H
