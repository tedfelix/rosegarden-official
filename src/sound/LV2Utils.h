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

#include <QMutex>

#include <map>
#include <lilv/lilv.h>

#include "base/Instrument.h"

namespace Rosegarden
{

class LV2PluginInstance;
class AudioPluginLV2GUI;

/// LV2 utils
/**
 * LV2Utils encapsulate the lv2 world and supports communcation
 * between LV2 plugins and LV2 uis (different threads),
 */

class LV2Utils
{
 public:
    /// Singleton
    static LV2Utils *getInstance();

    LV2Utils(LV2Utils &other) = delete;
    void operator=(const LV2Utils &) = delete;

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

    const std::map<QString, LV2PluginData>& getAllPluginData() const;
    const LilvPlugin* getPluginByUri(const QString& uri) const;
    LV2PluginData getPluginData(const QString& uri) const;
    const LilvUIs* getPluginUIs(const QString& uri) const;
    LilvNode* makeURINode(const QString& uri) const;

    void registerPlugin(InstrumentId instrument,
                        int position,
                        LV2PluginInstance* pluginInstance);
    void registerGUI(InstrumentId instrument,
                     int position,
                     AudioPluginLV2GUI* gui);

    void unRegisterPlugin(InstrumentId instrument,
                          int position);
    void unRegisterGUI(InstrumentId instrument,
                       int position);

 private:
    /// Singleton.  See getInstance().
    LV2Utils();
    ~LV2Utils();

    QMutex m_mutex;
    LilvWorld* m_world;
    const LilvPlugins* m_plugins;
    std::map<QString, LV2PluginData> m_pluginData;

    struct LV2UPlugin
    {
        LV2PluginInstance* pluginInstance;
        AudioPluginLV2GUI* gui;
        LV2UPlugin() {pluginInstance = nullptr; gui = nullptr;}
    };
    typedef std::map<int, LV2UPlugin> IntPluginMap;
    typedef std::map<int, IntPluginMap> PluginGuiMap;

    PluginGuiMap m_pluginGuis;
};

}

#endif // RG_LV2UTILS_H
