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

#ifndef RG_AUDIOPLUGINLV2GUIWINDOW_H
#define RG_AUDIOPLUGINLV2GUIWINDOW_H

#include "AudioPluginLV2GUI.h"
#include "LV2Gtk.h"

#include <lilv/lilv.h>
#include <lv2/ui/ui.h>
#include <lv2/data-access/data-access.h>
#include <lv2/options/options.h>

// the kx.studio extension
#include "lv2_external_ui.h"

#include <string>
#include <vector>


namespace Rosegarden
{


/// The widget that holds the LV2 GUI.
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
    void slotTimeUp();

private:
    void closeEvent(QCloseEvent* event) override;

    AudioPluginLV2GUI* m_lv2Gui;
    AudioPluginLV2GUI::UIType m_uiType;

    /// 50msec idle timer.  See timeUp() for details.
    QTimer* m_timer;

    // Features
    // ??? These are for the instantiate() call.  Do we really need to keep
    //     them around at instance scope?  Or can they be discarded after
    //     instantiate() is called (ctor local scope)?

    // good question. Quick answer - I don'know. There is no
    // requirement in the lv2 documentation that these parameters
    // should only be used in instantiate. I am sure most plugins will
    // do just that but it is possible that a plugin may keep a
    // pointer to a feature. Having these as data members is on the
    // safe side.
    LV2_Extension_Data_Feature m_dataAccess;
    LV2_Feature m_dataFeature;
    LV2_Feature m_uridMapFeature;
    LV2_Feature m_uridUnmapFeature;
    LV2_Feature m_idleFeature;
    LV2_Feature m_parentFeature;
    LV2UI_Resize m_resizeData;
    LV2_Feature m_resizeFeature;
    LV2_Feature m_instanceFeature;
    std::vector<LV2_Options_Option> m_options;
    LV2_Feature m_optionsFeature;
    LV2_External_UI_Host m_extUiHost;
    LV2_Feature m_extHostFeature;
    std::vector<LV2_Feature*> m_features;

    LV2UI_Idle_Interface* m_lv2II;
    LV2UI_Handle m_handle;

    LV2Gtk::LV2GtkWidget m_gwidget;
    QWidget* m_containerWidget;
    QWindow* m_parentWindow;
    LV2UI_Widget m_widget;

    QString m_title;
    std::string m_titleStdString;

    /// Shutdown flag for the timer.  See timeUp().
    bool m_shutdownRequested;
};


}


#endif
