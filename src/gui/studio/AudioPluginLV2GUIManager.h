/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_AUDIOPLUGINLV2GUIMANAGER_H
#define RG_AUDIOPLUGINLV2GUIMANAGER_H

#include "base/AudioPluginInstance.h"

#include <QObject>

#include <map>


namespace Rosegarden
{


class RosegardenMainWindow;
class Studio;
class AudioPluginLV2GUI;
class LV2Worker;


/**
 * AudioPluginGUIManager::m_lv2Manager is the one instance of this class.
 */
// cppcheck-suppress noConstructor
class AudioPluginLV2GUIManager : public QObject
{
    Q_OBJECT

public:
    explicit AudioPluginLV2GUIManager(RosegardenMainWindow *mainWindow);
    ~AudioPluginLV2GUIManager();

    void setStudio(Studio *studio);

    bool hasGUI(InstrumentId instrument, int position);
    void showGUI(InstrumentId instrument, int position);
    void stopGUI(InstrumentId instrument, int position);
    void stopAllGUIs();

    void updatePort(InstrumentId instrument, int position, int port);

    bool hasParameters(InstrumentId instrument, int position) const;
    void getParameters(InstrumentId instrument,
                       int position,
                       AudioPluginInstance::PluginParameters& params);
    void updatePluginParameter
        (InstrumentId instrument,
         int position,
         const QString& paramId,
         const AudioPluginInstance::PluginParameter& param);

    void getPresets(InstrumentId instrument,
                    int position,
                    AudioPluginInstance::PluginPresetList& presets);
    void setPreset(InstrumentId instrument, int position, const QString& uri);
    void loadPreset(InstrumentId instrument, int position, const QString& file);
    void savePreset(InstrumentId instrument, int position, const QString& file);

    bool canEditConnections(InstrumentId instrument, int position) const;
    void getConnections
        (InstrumentId instrument,
         int position,
         PluginPort::ConnectionList& clist) const;
    void setConnections
        (InstrumentId instrument,
         int position,
         const PluginPort::ConnectionList& clist);

public slots:
    void slotStopGUIDelayed();

private:
    AudioPluginLV2GUI* getInstance(InstrumentId instrument, int position);

    void updateControls(InstrumentId instrument, int position);

    RosegardenMainWindow *m_mainWindow;
    Studio *m_studio;

    typedef std::map<int /* position */, AudioPluginLV2GUI *> IntGUIMap;
    typedef std::map<InstrumentId, IntGUIMap> GUIMap;
    GUIMap m_guis;

    // Values for delayed stopGUI.
    // Set by stopGUI() and used by slotStopGUIDelayed().
    InstrumentId m_instrument;
    int m_position;
};


}

#endif
