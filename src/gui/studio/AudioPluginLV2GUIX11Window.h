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

#ifndef RG_AUDIOPLUGINLV2GUIX11WINDOW_H
#define RG_AUDIOPLUGINLV2GUIX11WINDOW_H

#include "AudioPluginLV2GUIWindow.h"

#include <lilv/lilv.h>
#include <lv2/ui/ui.h>

namespace Rosegarden
{

class AudioPluginLV2GUI;

class AudioPluginLV2GUIX11Window :
    public QWidget,
    public AudioPluginLV2GUIWindow
{
    Q_OBJECT
 public:
    AudioPluginLV2GUIX11Window(AudioPluginLV2GUI* lv2Gui,
                               const QString& title,
                               const LilvUI* ui,
                               const LV2UI_Descriptor* uidesc,
                               const QString& id);
    ~AudioPluginLV2GUIX11Window();

    void portChange(uint32_t portIndex,
                    uint32_t bufferSize,
                    uint32_t portProtocol,
                    const void *buffer);

    virtual void showGui() override;
 public slots:
    void timeUp();

 private:
    AudioPluginLV2GUI* m_lv2Gui;
    QTimer* m_timer;
    LV2_Feature m_uridMapFeature;
    LV2_Feature m_uridUnmapFeature;
    LV2_Feature m_idleFeature;
    LV2_Feature m_parentFeature;
    LV2_Feature m_resizeFeature;
    LV2UI_Resize m_resizeData;
    LV2UI_Idle_Interface* m_lv2II;
    LV2UI_Handle m_handle;
    std::vector<LV2_Feature*> m_features;
};


}

#endif
