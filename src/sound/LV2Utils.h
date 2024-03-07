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

#ifndef RG_LV2UTILS_H
#define RG_LV2UTILS_H

#include "base/Instrument.h"
#include "base/AudioPluginInstance.h"

#include <lilv/lilv.h>
#include <lv2/urid/urid.h>
#include <lv2/atom/atom.h>
#include <lv2/worker/worker.h>

#include <QMutex>
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
#include <QRecursiveMutex>
#endif
#include <QString>

#include <map>
#include <queue>

namespace Rosegarden
{

class LV2PluginInstance;
class AudioPluginLV2GUI;
class LV2Worker;

class LV2Utils
{
public:
    /// Singleton
    static LV2Utils *getInstance();


    // *** lilv wrappers ***

    // Most of these wrappers are used only by LV2PluginInstance.

    // lilv_plugins_get_by_uri() wrapper
    const LilvPlugin *getPluginByUri(const QString& uri) const;

    // lilv_port_get_index() wrapper
    int getPortIndexFromSymbol(const QString& portSymbol,
                               const LilvPlugin* plugin);

    // lilv_state_new_from_world() wrapper
    LilvState* getStateByUri(const QString& uri);

    // lilv_state_new_from_instance() wrapper
    LilvState* getStateFromInstance(const LilvPlugin* plugin,
                                    LilvInstance* instance,
                                    LilvGetPortValueFunc getPortValueFunc,
                                    LV2PluginInstance* lv2Instance,
                                    const LV2_Feature*const* features);

    // lilv_state_new_from_instance() wrapper
    QString getStateStringFromInstance(const LilvPlugin* plugin,
                                       const QString& uri,
                                       LilvInstance* instance,
                                       LilvGetPortValueFunc getPortValueFunc,
                                       LV2PluginInstance* lv2Instance,
                                       const LV2_Feature*const* features);

    // lilv_state_new_from_string() wrapper
    void setInstanceStateFromString(const QString& stateString,
                                    LilvInstance* instance,
                                    LilvSetPortValueFunc setPortValueFunc,
                                    LV2PluginInstance* lv2Instance,
                                    const LV2_Feature*const* features);

    // lilv_state_new_from_file() wrapper
    LilvState* getStateFromFile(const LilvNode* uriNode,
                                const QString& filename);

    // lilv_state_save() wrapper
    void saveStateToFile(const LilvState* state, const QString& filename);

    /// lilv_new_uri() wrapper
    LilvNode* makeURINode(const QString& uri) const;

    /// lilv_new_string() wrapper
    LilvNode* makeStringNode(const QString& string) const;


    // *** LV2 Plugin Instance Database ***

    /// Unique ID for a specific plugin instance.
    /**
     * Combines the InstrumentId and the position of the plugin within
     * that instrument.
     *
     * Used as a key type for plugin instance data and LV2Worker maps.
     *
     * See LV2PluginInstance::m_instrument and m_position.
     *
     * ??? Rename: PluginID, then see if it is needed elsewhere.  Promote
     *             to Instrument.
     */
    struct PluginPosition
    {
        InstrumentId instrument;
        // Position in the effects stack for effect plugins,
        // 999 (Instrument::SYNTH_PLUGIN_POSITION) for synths.
        int position;

        bool operator<(const PluginPosition &p) const
        {
            if (instrument < p.instrument) return true;
            if (instrument > p.instrument) return false;
            if (position < p.position) return true;
            return false;
        }
    };

    /// Lock m_mutex.
    /**
     * Called from both the UI thread and the JACK process thread.
     *
     * ??? This is used to lock m_workerResponses, and what else?
     *
     * This is the central mutex lock for all the lv2 thread communication
     * Used in run() in LV2PluginInstance - see also the LOCKED macro.
     * I find it difficult to know when to use the mutex and when not - so
     * there may be some mistakes here
     */
    void lock();
    /// Unlock m_mutex.
    void unlock();


    /// Adds a plugin instance to m_pluginInstanceData.
    void registerPlugin(InstrumentId instrument,
                        int position,
                        LV2PluginInstance* pluginInstance);
    /// Removes a plugin instance from m_pluginInstanceData.
    void unRegisterPlugin(InstrumentId instrument,
                          int position,
                          LV2PluginInstance* pluginInstance);

    /// Adds a plugin GUI to m_pluginInstanceData.
    void registerGUI(InstrumentId instrument,
                     int position,
                     AudioPluginLV2GUI* gui);
    /// Removes a plugin GUI from m_pluginInstanceData.
    void unRegisterGUI(InstrumentId instrument,
                       int position);

    // set the value for the plugin
    void setPortValue(InstrumentId instrument,
                      int position,
                      int index,
                      unsigned int protocol,
                      const QByteArray& data);

    // called by the plugin to update the ui
    // IMPORTANT: Must call lock() before calling this!
    // ??? Move the logic from the caller into here so that lock()
    //     need not be public for this.  Then we can use LOCKED which
    //     is safer.
    void updatePortValue(InstrumentId instrument,
                         int position,
                         int index,
                         const LV2_Atom* atom);

    void triggerPortUpdates(InstrumentId instrument,
                            int position);

    typedef std::map<int /*portIndex*/, float /*value*/> PortValues;

    void getControlInValues(InstrumentId instrument,
                            int position,
                            PortValues &controlValues);

    void getControlOutValues(InstrumentId instrument,
                             int position,
                             PortValues &controlValues);

    /// Calls the plugin instance's runWork().
    void runWork(const PluginPosition &pp,
                 uint32_t size,
                 const void *data,
                 LV2_Worker_Respond_Function resp);

    LV2PluginInstance* getPluginInstance(InstrumentId instrument,
                                         int position) const;

    void getConnections
        (InstrumentId instrument,
         int position,
         PluginPort::ConnectionList& clist) const;
    // used in AudioPluginLV2GUIManager
    void setConnections
        (InstrumentId instrument,
         int position,
         const PluginPort::ConnectionList& clist) const;

    // Presets

    void setupPluginPresets(const QString& uri,
                            AudioPluginInstance::PluginPresetList& presets);
    void getPresets(InstrumentId instrument,
                    int position,
                    AudioPluginInstance::PluginPresetList& presets) const;
    void setPreset(InstrumentId instrument,
                   int position,
                   const QString& uri);
    void loadPreset(InstrumentId instrument,
                    int position,
                    const QString& file);
    void savePreset(InstrumentId instrument,
                    int position,
                    const QString& file);

private:

    /// Singleton.  See getInstance().
    LV2Utils();
    ~LV2Utils();
    LV2Utils(LV2Utils &other) = delete;
    void operator=(const LV2Utils &) = delete;

    // Used by both the UI thread and the audio (JACK process) thread.
    //
    // This is primarily used to guard m_pluginInstanceData.
    //
    // This mutex is used for the synchronization of the lv2
    // threads. Some calls are made in the audio thread others in the
    // gui thread. Data used by the calls are protected with this
    // mutex.
    //
    // ??? Because all the plugins share this mutex, there will be
    //     unnecessary contention.  We could
    //     move toward finer grain.  A mutex for each plugin
    //     instance.  Move toward LV2PluginInstance having an instance of
    //     PluginInstanceData.  This should reduce mutex contention with only
    //     a small memory cost.
    //
    //     There is only one audio thread and only one worker thread, so we
    //     should probably analyze the potential contention to see how much
    //     gain there would actually be before moving forward on this.  E.g.
    //     plugins won't be contending with each other on the audio thread.
    //     However, finer grain locking means the UI thread and the audio
    //     thread will be less likely to contend with each other.
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QRecursiveMutex m_mutex;
#else
    QMutex m_mutex;
#endif


    // Plugin instance data organized by instrument/position.

    struct AtomQueueItem
    {
        int portIndex;
        const LV2_Atom* atomBuffer;
        AtomQueueItem();
        ~AtomQueueItem();
    };
    typedef std::queue<AtomQueueItem*> AtomQueue;

    struct PluginInstanceData
    {
        // ??? This pointer is also kept in AudioInstrumentMixer::m_synths
        //     or m_plugins as appropriate.  They are indexed by InstrumentId.
        LV2PluginInstance *pluginInstance{nullptr};
        AudioPluginLV2GUI *gui{nullptr};
        AtomQueue atomQueue;
    };
    typedef std::map<PluginPosition, PluginInstanceData> PluginInstanceDataMap;
    /**
     * Users:
     *   registerPlugin() - writes
     *   registerGUI() - writes
     *   unRegisterPlugin() - writes
     *   unRegisterGUI() - writes
     *   setPortValue() - writes (calls lv2_atom_sequence_append_event())
     *   updatePortValue() - writes, JACK audio thread!!!
     *   triggerPortUpdate() - writes
     *   runWork() - reads? (sends work off to the worker)
     *   getControlInValues() - reads
     *   getControlOutValues() - reads
     *   getPluginInstance() - Returns a non-const pointer into this.
     *                         Callers are responsible for locking.
     *   getPresets() - Returns a non-const reference into this.  Callers
     *                  are responsible for locking.
     *   setPreset() - Reads, then calls lilv_state_restore().
     *   loadPreset()
     *   savePreset()
     *
     * Thread-Safe?  Maybe.  Locks are inconsistent around this.  The JACK audio
     * process thread definitely touches this (updatePortValue()) along with
     * the UI thread.  Anything the JACK audio thread reads or writes has to
     * be locked by both the JACK audio thread and the UI thread.  Everything
     * else is likely used by the UI thread only and need not be locked.
     *
     * I think the way to go here is to split off the portion of the data that
     * is hit by the JACK audio thread.  Then we can lock it and keep it
     * in sync appropriately while all the rest is UI thread stuff and can be
     * lock free.
     */
    PluginInstanceDataMap m_pluginInstanceData;

};


}

#endif // RG_LV2UTILS_H
