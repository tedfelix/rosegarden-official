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
#include "sound/PluginPortConnection.h"

#include <lilv/lilv.h>
#include <lv2/urid/urid.h>
#include <lv2/atom/atom.h>
#include <lv2/worker/worker.h>

#include <QMutex>
#include <QString>

#include <map>

namespace Rosegarden
{


class LV2PluginInstance;
class AudioPluginLV2GUI;
class LV2Gtk;


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


    // URI/URID mapping.

    // ??? Split this out into its own LV2URIMapper class.

    /// Gets the URID for a URI.
    /**
     * If the URI hasn't been seen yet, a new URID is assigned.
     *
     * @see m_uridMap
     */
    LV2_URID uridMap(const char *uri);
    /// Member function pointer to uridMap().
    LV2_URID_Map m_map;

    /// Gets the URI for a URID.
    /**
     * @see m_uridUnmap
     */
    const char *uridUnmap(LV2_URID urid);
    /// Member function pointer to uridUnmap().
    LV2_URID_Unmap m_unmap;


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


    // LV2 Worker Feature

    // ??? Move these to LV2Worker as statics?

    // interface for a worker class
    // ??? Only used by LV2Worker.  Move there.
    typedef LV2_Worker_Status (*ScheduleWork)(LV2_Worker_Schedule_Handle handle,
                                              uint32_t size,
                                              const void* data);

    //typedef LV2_Worker_Status (*RespondWork)(LV2_Worker_Respond_Handle handle,
    //                                         uint32_t size,
    //                                         const void *data);

    struct WorkerJob
    {
        uint32_t size;
        const void *data;
    };

    /// LV2Worker derives from this.
    class Worker
    {
    public:
        virtual ~Worker() {};
        virtual ScheduleWork getScheduler() = 0;
        virtual WorkerJob *getResponse(const LV2Utils::PluginPosition &pp) = 0;
    };

    void registerWorker(Worker *worker);
    Worker *getWorker() const;
    void unRegisterWorker();

    void runWork(const PluginPosition &pp,
                 uint32_t size,
                 const void *data,
                 LV2_Worker_Respond_Function resp);


    // Plugin Management

    // ??? These functions feel more like an LV2PluginManager class.
    //     Consider breaking them out into a new class or namespace.

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
    LilvState* getDefaultStateByUri(const QString& uri);
    QString getStateFromInstance(const LilvPlugin* plugin,
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

    /// lilv_new_uri() wrapper.
    LilvNode* makeURINode(const QString& uri) const;
    /// lilv_new_string() wrapper.
    LilvNode* makeStringNode(const QString& string) const;


    /// Lock m_mutex.
    /**
     * ??? This is used to lock m_workerResponses, and what else?
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
    void updatePortValue(InstrumentId instrument,
                         int position,
                         int index,
                         const LV2_Atom* atom);

//    int numInstances(InstrumentId instrument,
//                     int position) const;

    void getControlInValues(InstrumentId instrument,
                            int position,
                            std::map<int, float> &controlValues);

    void getControlOutValues(InstrumentId instrument,
                             int position,
                             std::map<int, float> &controlValues);

    LV2PluginInstance* getPluginInstance(InstrumentId instrument,
                                         int position) const;

    /// Singleton.
    // ??? Move to LV2Gtk::getInstance().
    LV2Gtk *getLV2Gtk() const;

    void getConnections
        (InstrumentId instrument,
         int position,
         PluginPortConnection::ConnectionList& clist) const;
    // ??? Might be unused.  Caller appears to be unused.
    void setConnections
        (InstrumentId instrument,
         int position,
         const PluginPortConnection::ConnectionList& clist) const;

    QString getPortName(const QString& uri, int portIndex) const;

 private:

    /// Singleton.  See getInstance().
    LV2Utils();
    ~LV2Utils();


    // URID <-> URI maps

    // URI (string) to URID (uint)
    std::map<std::string /* URI */, LV2_URID> m_uridMap;
    // URID (uint) to URI (string)
    std::map<LV2_URID, std::string /* URI */> m_uridUnmap;
    // Next URID.
    LV2_URID m_nextId;


    // ??? What data is this synchronizing?  It appears to be used
    //     for a number of different things but not everything.  Why?
    // ??? What threads are involved?  Plugin instance threads?
    //     Plugin GUI threads?  Plugin worker threads?
    //     RG GUI thread?  RG sequencer thread?
    QMutex m_mutex;


    /// Instance pointer used by the lilv_*() functions.
    LilvWorld *m_world;

    /// Result of lilv_world_get_all_plugins().
    /**
     * Used by getPluginByUri().
     */
    const LilvPlugins *m_plugins;

    /// Additional data we keep for each plugin organized by URI.
    std::map<QString /* URI */, LV2PluginData> m_pluginData;
    /// Assemble plugin data for each plugin and add to m_pluginData.
    void initPluginData();

    // Plugin instance data organized by instrument/position.
    // ??? rename: PluginInstanceData
    struct LV2UPlugin
    {
        LV2PluginInstance *pluginInstance{nullptr};
        AudioPluginLV2GUI *gui{nullptr};
    };
    // ??? rename: PluginInstanceMap
    typedef std::map<PluginPosition, LV2UPlugin> PluginGuiMap;
    PluginGuiMap m_pluginGuis;

    /// The LV2Worker instance.
    /**
     * Would a Singleton be simpler?
     */
    Worker *m_worker;

    // ??? Move to LV2Gtk.
    LV2Gtk *m_lv2gtk;
};


}

#endif // RG_LV2UTILS_H
