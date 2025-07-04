/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_LV2PLUGINDATABASE_H
#define RG_LV2PLUGINDATABASE_H

#include <QString>

#include <map>


namespace Rosegarden
{


/// Stores plugin data for easier access.
namespace LV2PluginDatabase
{
    enum LV2PortType  { LV2CONTROL, LV2AUDIO, LV2MIDI };
    enum LV2PortProtocol  { LV2FLOAT, LV2ATOM };

    struct LV2PortData
    {
        QString name;
        LV2PortType portType;
        LV2PortProtocol portProtocol;
        bool isPatch;
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
        bool isInstrument{false};
        std::vector<LV2PortData> ports;
    };

    typedef std::map<QString /*uri*/, Rosegarden::LV2PluginDatabase::LV2PluginData>
            PluginDatabase;

    /// Additional data we keep for each plugin organized by URI.
    const PluginDatabase &getAllPluginData();

    LV2PluginData getPluginData(const QString& uri);

    /// Easy access to port names.
    QString getPortName(const QString& uri, int portIndex);

}


}


#endif
