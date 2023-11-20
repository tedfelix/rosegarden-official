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

#define RG_MODULE_STRING "[AudioPluginLV2GUIWindow]"
#define RG_NO_DEBUG_PRINT 1

#include "AudioPluginLV2GUIWindow.h"

#include "misc/Debug.h"
#include "AudioPluginLV2GUI.h"

#include <QTimer>

#include <lilv/lilv.h>
#include <lv2/ui/ui.h>

#include "sound/LV2Utils.h"

namespace
{
    void writeFn(LV2UI_Controller controller,
                 uint32_t port_index,
                 uint32_t buffer_size,
                 uint32_t port_protocol,
                 const void *buffer)
    {
        Rosegarden::AudioPluginLV2GUIWindow* ap =
            static_cast<Rosegarden::AudioPluginLV2GUIWindow*>(controller);
        ap->portChange(port_index, buffer_size, port_protocol, buffer);
    }

    int LV2Resize (LV2UI_Feature_Handle handle, int width, int height )
    {
        RG_DEBUG << "resize" << width << height;
        QWidget *widget = static_cast<QWidget *>(handle);
        if (widget) {
            widget->resize(width, height);
            return 0;
        } else {
            return 1;
        }
    }
}

namespace Rosegarden
{

AudioPluginLV2GUIWindow::AudioPluginLV2GUIWindow
(AudioPluginLV2GUI* lv2Gui,
 const QString& title,
 const LilvUI* ui,
 const LV2UI_Descriptor* uidesc,
 const QString& id,
 AudioPluginLV2GUI::UIType uiType) :
    m_lv2Gui(lv2Gui),
    m_lv2II(nullptr)
{
    RG_DEBUG << "create window" << id << uiType;
    setWindowTitle(title);
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout,
            this, &AudioPluginLV2GUIWindow::timeUp);
    m_timer->start(50);

    const char *ui_bundle_uri =
        lilv_node_as_uri(lilv_ui_get_bundle_uri(ui));
    char *hostname;
    char* ubp = lilv_file_uri_parse(ui_bundle_uri, &hostname);
    LV2UI_Controller controller = this;
    LV2UI_Widget widget = nullptr;

    m_idleFeature = {LV2_UI__idleInterface, nullptr};
    m_parentFeature = {LV2_UI__parent, (void*)winId()};

    m_resizeData.handle = this;
    m_resizeData.ui_resize = LV2Resize;
    m_resizeFeature.URI = LV2_UI__resize;
    m_resizeFeature.data = &m_resizeData;

    LV2Utils* lv2utils = LV2Utils::getInstance();
    m_uridMapFeature = {LV2_URID__map, &(lv2utils->m_map)};
    m_uridUnmapFeature = {LV2_URID__unmap, &(lv2utils->m_unmap)};
    m_features.push_back(&m_uridMapFeature);
    m_features.push_back(&m_uridUnmapFeature);
    m_features.push_back(&m_idleFeature);
    m_features.push_back(&m_parentFeature);
    m_features.push_back(&m_resizeFeature);
    m_features.push_back(nullptr);

    const void* ii = uidesc->extension_data(LV2_UI__idleInterface);
    m_lv2II = (LV2UI_Idle_Interface*)ii;

    m_handle =
        uidesc->instantiate(uidesc,
                            id.toStdString().c_str(),
                            ubp,
                            writeFn,
                            controller,
                            &widget,
                            m_features.data());

    RG_DEBUG << "handle:" << m_handle << "widget:" << widget;
}

AudioPluginLV2GUIWindow::~AudioPluginLV2GUIWindow()
{
    RG_DEBUG << "~AudioPluginLV2GUIWindow";
    m_lv2II = nullptr;
}

void
AudioPluginLV2GUIWindow::portChange(uint32_t portIndex,
                                       uint32_t bufferSize,
                                       uint32_t portProtocol,
                                       const void *buffer)
{
    RG_DEBUG << "portChange" << portIndex <<
        bufferSize << portProtocol;
    m_lv2Gui->portChange(portIndex, bufferSize, portProtocol, buffer);
}

void AudioPluginLV2GUIWindow::showGui()
{
    show();
}

LV2UI_Handle AudioPluginLV2GUIWindow::getHandle() const
{
    return m_handle;
}

void AudioPluginLV2GUIWindow::timeUp()
{
    if (m_lv2II) m_lv2II->idle(m_handle);

    // check control outs
    m_lv2Gui->checkControlOutValues();
}

}