/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2023 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_AUDIOPLUGINLV2GUI_H
#define RG_AUDIOPLUGINLV2GUI_H

#include <lilv/lilv.h>
#include <lv2/ui/ui.h>
#include <lv2/atom/atom.h>

#include <map>
#include "gui/application/RosegardenMainWindow.h"

namespace Rosegarden
{

class AudioPluginInstance;
class AudioPluginLV2GUIWindow;
class LV2PluginInstance;
class AudioPluginLV2GUIManager;

// cppcheck-suppress noCopyConstructor
class AudioPluginLV2GUI
{
 public:
    AudioPluginLV2GUI(AudioPluginInstance *instance,
                      RosegardenMainWindow *mainWindow,
                      InstrumentId instrument,
                      int position,
                      AudioPluginLV2GUIManager* manager);
    ~AudioPluginLV2GUI();

    enum UIType {X11, GTK, KX, NONE};

    // copy constructor not used
    AudioPluginLV2GUI(const AudioPluginInstance&) = delete;

    QString getId() const;
    bool hasGUI() const;

    void showGui() const;

    void portChange(uint32_t portIndex,
                    uint32_t bufferSize,
                    uint32_t portProtocol,
                    const void *buffer);

    void updatePortValue(int port, float value);
    void updatePortValue(int port, const LV2_Atom* atom);

    void checkControlOutValues();

    const LV2PluginInstance* getPluginInstance() const;

    void closeUI();

 private:
    AudioPluginLV2GUIManager* m_manager;
    AudioPluginInstance* m_pluginInstance;
    RosegardenMainWindow* m_mainWindow;
    InstrumentId m_instrument;
    int m_position;
    QString m_id;
    LilvUIs* m_uis;
    void* m_uilib;
    const LV2UI_Descriptor* m_uidesc;
    AudioPluginLV2GUIWindow* m_window;
    LV2_URID m_atomTransferUrid;
    std::map<int, float> m_controlOutValues;
    bool m_firstUpdate;
};


}

#endif
