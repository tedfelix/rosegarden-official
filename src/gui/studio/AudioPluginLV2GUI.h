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
#include "lv2/ui/ui.h"

#include <vector>
#include "gui/application/RosegardenMainWindow.h"

namespace Rosegarden
{

class AudioPluginInstance;

class AudioPluginLV2GUI : public QWidget
{
    Q_OBJECT
 public:
    AudioPluginLV2GUI(AudioPluginInstance *instance,
                      LilvWorld* world,
                      RosegardenMainWindow *mainWindow,
                      InstrumentId instrument,
                      int position);
    ~AudioPluginLV2GUI();

    // copy constructor not used
    AudioPluginLV2GUI(const AudioPluginInstance&) = delete;

    QString getId() const;
    bool hasGUI() const;

    void portChange(uint32_t portIndex,
                    uint32_t bufferSize,
                    uint32_t portProtocol,
                    const void *buffer);

 public slots:
    void timeUp();

 private:
    LilvWorld* m_world;
    AudioPluginInstance* m_pluginInstance;
    RosegardenMainWindow* m_mainWindow;
    InstrumentId m_instrument;
    int m_position;
    QString m_id;
    LilvUIs* m_uis;
    void* m_uilib;
    LV2UI_Idle_Interface* m_lv2II;
    QTimer* m_timer;
    LV2UI_Handle m_handle;
    const LV2UI_Descriptor* m_uidesc;
    LV2_Feature m_uridMapFeature;
    LV2_Feature m_uridUnmapFeature;
    LV2_Feature m_idleFeature;
    LV2_Feature m_parentFeature;
    LV2_Feature m_resizeFeature;
    LV2UI_Resize m_resizeData;
    std::vector<LV2_Feature*> m_features;
};


}

#endif
