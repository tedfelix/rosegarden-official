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

namespace Rosegarden
{

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

    struct LV2PortData
    {
        QString name;
        LV2PortType portType;
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

    const std::map<QString, LV2PluginData>& getAllPluginData();
    const LilvPlugin* getPluginByUri(const QString& uri) const;
    LV2PluginData getPluginData(const QString& uri) const;

 private:
    /// Singleton.  See getInstance().
    LV2Utils();
    ~LV2Utils();

    QMutex m_mutex;
    LilvWorld* m_world;
    std::map<QString, LV2PluginData> m_pluginData;
};

}

#endif // RG_LV2UTILS_H
