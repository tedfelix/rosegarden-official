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

#ifndef RG_AUDIOPLUGINLV2GUI_H
#define RG_AUDIOPLUGINLV2GUI_H

#include "base/Instrument.h"
#include "sound/LV2Utils.h"

#include <lilv/lilv.h>
#include <lv2/ui/ui.h>
#include <lv2/atom/atom.h>

#include <map>


namespace Rosegarden
{


class RosegardenMainWindow;
class AudioPluginInstance;
class AudioPluginLV2GUIWindow;
class LV2PluginInstance;
class AudioPluginLV2GUIManager;


// cppcheck-suppress noCopyConstructor
 class AudioPluginLV2GUI : public QObject
{
    Q_OBJECT
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
    void showGui();

    void portChange(uint32_t portIndex,
                    uint32_t bufferSize,
                    uint32_t portProtocol,
                    const void *buffer);

    void updatePortValue(int port, float value);
    void updatePortValue(int port, const LV2_Atom* atom);

    /// Calls updatePortValue() for any that have changed.
    void updateControlOutValues();

    LV2PluginInstance *getPluginInstance() const;

    void closeUI();

private:

    RosegardenMainWindow* m_mainWindow;

    AudioPluginLV2GUIManager* m_manager;
    AudioPluginInstance* m_pluginInstance;

    InstrumentId m_instrument;
    int m_position;
    QString m_id;

    // From lilv_plugin_get_uis().
    LilvUIs* m_uis;

    // From dlopen().
    void* m_uilib;

    const LV2UI_Descriptor* m_uidesc;
    AudioPluginLV2GUIWindow* m_window;
    LV2_URID m_atomTransferUrid;

    /// Cache for detecting changes in checkControlOutValues().
    LV2Utils::PortValues m_controlOutValues;

    bool m_firstUpdate;

    QString m_title;

    const LilvUI* m_selectedUI;
    UIType m_uiType;
};


}

#endif
