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

/// LV2 utils
/**
 * LV2Utils encapsulates the lv2 world (LilvWorld?) and supports communication
 * between LV2 plugins and LV2 UIs (different threads).
 */
class LV2Utils
{
public:
    /// Singleton
    static LV2Utils *getInstance();

    LV2Utils(LV2Utils &other) = delete;
    void operator=(const LV2Utils &) = delete;

    // Key type for GUI and worker maps.
    struct PluginPosition
    {
        InstrumentId instrument;
        int position;

        bool operator<(const PluginPosition &p) const
        {
            if (instrument < p.instrument) return true;
            if (instrument > p.instrument) return false;
            if (position < p.position) return true;
            return false;
        }
    };


    // Plugin Management

    // ??? These functions feel more like an LV2PluginManager class.
    //     Consider breaking them out into a new class or namespace.
    // yes these are for the factory class - I would like to keep all the
    // lv2 utility things here

    enum LV2PortType {LV2CONTROL, LV2AUDIO, LV2MIDI};
    enum LV2PortProtocol {LV2FLOAT, LV2ATOM};

    struct LV2PortData
    {
        QString name;
        LV2PortType portType;
        LV2PortProtocol portProtocol;
        bool isInput;
        float min;
        float max;
        float def;
        int displayHint;
    };

    struct LV2PluginData
    {
        QString name;
        QString label; // abbreviated name
        QString pluginClass;
        QString author;
        bool isInstrument;
        std::vector<LV2PortData> ports;
    };

    const std::map<QString /* URI */, LV2PluginData> &getAllPluginData();
    LV2PluginData getPluginData(const QString& uri) const;

    const LilvPlugin *getPluginByUri(const QString& uri) const;

    // Port
    int getPortIndexFromSymbol(const QString& portSymbol,
                               const LilvPlugin* plugin);

    // State
    LilvState* getStateByUri(const QString& uri);
    LilvState* getStateFromInstance(const LilvPlugin* plugin,
                                    LilvInstance* instance,
                                    LilvGetPortValueFunc getPortValueFunc,
                                    LV2PluginInstance* lv2Instance,
                                    const LV2_Feature*const* features);
    QString getStateStringFromInstance(const LilvPlugin* plugin,
                                       const QString& uri,
                                       LilvInstance* instance,
                                       LilvGetPortValueFunc getPortValueFunc,
                                       LV2PluginInstance* lv2Instance,
                                       const LV2_Feature*const* features);
    void setInstanceStateFromString(const QString& stateString,
                                    LilvInstance* instance,
                                    LilvSetPortValueFunc setPortValueFunc,
                                    LV2PluginInstance* lv2Instance,
                                    const LV2_Feature*const* features);
    LilvState* getStateFromFile(const LilvNode* uriNode,
                                const QString& filename);
    void saveStateToFile(const LilvState* state, const QString& filename);

    /// lilv_new_uri() wrapper.
    LilvNode* makeURINode(const QString& uri) const;
    /// lilv_new_string() wrapper.
    LilvNode* makeStringNode(const QString& string) const;


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


    /// Adds plugin instance to m_pluginGuis.
    void registerPlugin(InstrumentId instrument,
                        int position,
                        LV2PluginInstance* pluginInstance);
    void unRegisterPlugin(InstrumentId instrument,
                          int position,
                          LV2PluginInstance* pluginInstance);

    /// Adds plugin GUI to m_pluginGuis.
    void registerGUI(InstrumentId instrument,
                     int position,
                     AudioPluginLV2GUI* gui);
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

    QString getPortName(const QString& uri, int portIndex) const;


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

    // Used by both the UI thread and the JACK process thread.
    //
    // This is primarily used to guard m_pluginInstanceData.
    //
    // This mutex is used for the synchronization of the lv2
    // threads. Some calls are made in the audio thread others in the
    // gui thread. Data used by the calls are protected with this
    // mutex.
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QRecursiveMutex m_mutex;
#else
    QMutex m_mutex;
#endif

    /// Additional data we keep for each plugin organized by URI.
    /**
     * ??? Pull this out into an LV2PluginDatabase namespace.  This leaves the
     *     mutex with what it protects.
     *
     * Thread-Safe?  No.  GetAllPluginData() is problematic.
     * This should be written at startup (LV2Utils ctor?  once_flag/call_once?
     * static construction?  lots of options...) and then never
     * changed.  We should be able to get away with lock free if we are
     * careful here.  Since this is likely a UI-only thing, we are probably
     * safe anyway as only the UI thread will see this.
     *
     * Users:
     *   initPluginData() - Called by ctor
     *   getAllPluginData() - Returns a const reference to m_pluginData.
     *     - Called by LV2PluginFactory.
     *     - Called by the AudioPluginManager::Enumerator thread.
     *       ??? This one is odd.  It may be that the UI waits for this anyway.
     *           So there really isn't an issue.  Look closely at this.
     *     - Called by the UI thread.
     *   getPluginData() - Returns a copy of data in m_pluginData.
     *     - Safe to return a copy so long as initPluginData() has completed.
     *     - Might be able to return a const reference for speed.
     *   getPortName() - Returns a copy of the name in m_pluginData.
     *     - Safe to return a copy so long as initPluginData() has completed.
     *
     */
    std::map<QString /* URI */, LV2PluginData> m_pluginData;
    /// Assemble plugin data for each plugin and add to m_pluginData.
    void initPluginData();


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
