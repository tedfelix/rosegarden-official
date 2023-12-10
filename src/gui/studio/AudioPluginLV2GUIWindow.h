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

#ifndef RG_AUDIOPLUGINLV2GUIWINDOW_H
#define RG_AUDIOPLUGINLV2GUIWINDOW_H

#include <lilv/lilv.h>
#include <lv2/ui/ui.h>
#include <lv2/data-access/data-access.h>
#include <lv2/options/options.h>
// the kx.studio extension
#include "lv2_external_ui.h"

#include <vector>

#include "AudioPluginLV2GUI.h"
#include "LV2Gtk.h"

namespace Rosegarden
{

class AudioPluginLV2GUIWindow :
    public QWidget,
    public LV2Gtk::SizeCallback
{
    Q_OBJECT
 public:
    AudioPluginLV2GUIWindow(AudioPluginLV2GUI* lv2Gui,
                            const QString& title,
                            const LilvUI* ui,
                            const LV2UI_Descriptor* uidesc,
                            const QString& id,
                            AudioPluginLV2GUI::UIType uiType);
    ~AudioPluginLV2GUIWindow();

    void portChange(uint32_t portIndex,
                    uint32_t bufferSize,
                    uint32_t portProtocol,
                    const void *buffer);

    void showGui();
    LV2UI_Handle getHandle() const;
    void uiClosed();

    // gtk callback
    virtual void setSize(int width, int height, bool isRequest) override;

 public slots:
    void timeUp();

 private:
    void closeEvent(QCloseEvent* event) override;

    AudioPluginLV2GUI* m_lv2Gui;
    AudioPluginLV2GUI::UIType m_uiType;
    QTimer* m_timer;
    LV2_Extension_Data_Feature m_dataAccess;
    LV2_Feature m_uridMapFeature;
    LV2_Feature m_uridUnmapFeature;
    LV2_Feature m_idleFeature;
    LV2_Feature m_parentFeature;
    LV2_Feature m_resizeFeature;
    LV2_Feature m_instanceFeature;
    LV2_Feature m_dataFeature;
    LV2_Feature m_optionsFeature;
    LV2_Feature m_extHostFeature;
    LV2UI_Resize m_resizeData;
    LV2UI_Idle_Interface* m_lv2II;
    LV2UI_Handle m_handle;
    std::vector<LV2_Feature*> m_features;
    std::vector<LV2_Options_Option> m_options;
    LV2_External_UI_Host m_extUiHost;
    LV2Gtk::LV2GtkWidget m_gwidget;
    QWidget* m_cWidget;
    QWindow* m_pWindow;
    LV2UI_Widget m_widget;
    QString m_title;
    std::string m_titles;
};

}

#endif
