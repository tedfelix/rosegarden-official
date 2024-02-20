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
#include "sound/LV2PluginInstance.h"
#include "sound/LV2Utils.h"
#include "LV2Gtk.h"

#include <QTimer>
#include <QWindow>
#include <QCloseEvent>

#include <lilv/lilv.h>
#include <lv2/ui/ui.h>
#include <lv2/instance-access/instance-access.h>
#include <lv2/parameters/parameters.h>


namespace
{
    /// For LV2UI_Descriptor::instantiate().
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

    /// For m_resizeFeature.
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

    /// For LV2_External_UI_Host.
    void ui_closed(LV2UI_Controller controller)
    {
        Rosegarden::AudioPluginLV2GUIWindow* ap =
            static_cast<Rosegarden::AudioPluginLV2GUIWindow*>(controller);
        ap->uiClosed();
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
    m_uiType(uiType),
    m_lv2II(nullptr),
    m_containerWidget(nullptr),
    m_parentWindow(nullptr),
    m_widget(nullptr),
    m_title(title),
    m_shutdownRequested(false)
{
    RG_DEBUG << "create window" << id << m_uiType << m_title;
    setWindowTitle(m_title);

    // Create the idle timer for this window.
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout,
            this, &AudioPluginLV2GUIWindow::slotTimeUp);
    m_timer->start(100);

    const char *ui_bundle_uri =
        lilv_node_as_uri(lilv_ui_get_bundle_uri(ui));
    // Parsed Bundle URI
    char* ubp = lilv_file_uri_parse(ui_bundle_uri, nullptr);

    // Assemble the features.

    m_idleFeature = {LV2_UI__idleInterface, nullptr};
    m_parentFeature = {LV2_UI__parent, (void*)winId()};

    const LV2PluginInstance* pluginInstance = m_lv2Gui->getPluginInstance();
    if (! pluginInstance) {
        RG_DEBUG << "no instance";
        return;
    }
    LV2_Handle handle = pluginInstance->getHandle();
    m_instanceFeature = {LV2_INSTANCE_ACCESS_URI, handle};

    const LV2_Descriptor* lv2d = pluginInstance->getLV2Descriptor();
    m_dataAccess.data_access = lv2d->extension_data;
    m_dataFeature = {LV2_DATA_ACCESS_URI, (void*)&m_dataAccess};

    m_resizeData.handle = this;
    m_resizeData.ui_resize = LV2Resize;
    m_resizeFeature.URI = LV2_UI__resize;
    m_resizeFeature.data = &m_resizeData;

    LV2Utils* lv2utils = LV2Utils::getInstance();
    LV2_URID sampleRateUrid = lv2utils->uridMap(LV2_PARAMETERS__sampleRate);
    LV2_URID af_urid = lv2utils->uridMap(LV2_ATOM__Float);
    float sampleRate = pluginInstance->getSampleRate();
    std::string titleStdString = m_title.toStdString();
    LV2_URID titleUrid = lv2utils->uridMap(LV2_UI__windowTitle);
    LV2_URID as_urid = lv2utils->uridMap(LV2_ATOM__String);
    LV2_Options_Option opt;
    opt.context = LV2_OPTIONS_INSTANCE;
    opt.subject = 0;
    opt.key = sampleRateUrid;
    opt.size = 4;
    opt.type = af_urid;
    opt.value = &sampleRate;
    m_options.push_back(opt);
    opt.key = titleUrid;
    opt.size = titleStdString.size();
    opt.type = as_urid;
    opt.value = titleStdString.c_str();
    m_options.push_back(opt);
    opt.subject = 0;
    opt.key = 0;
    opt.size = 0;
    opt.type = 0;
    opt.value = 0;
    m_options.push_back(opt);
    m_optionsFeature = {LV2_OPTIONS__options, m_options.data()};

    m_extUiHost.ui_closed = &ui_closed;
    m_extUiHost.plugin_human_id = titleStdString.c_str();
    m_extHostFeature = {LV2_EXTERNAL_UI__Host, &m_extUiHost};

    m_uridMapFeature = {LV2_URID__map, &(lv2utils->m_map)};
    m_uridUnmapFeature = {LV2_URID__unmap, &(lv2utils->m_unmap)};

    // note the instance and data access features are
    // deprecated. However some plugins require them
    m_features.push_back(&m_uridMapFeature);
    m_features.push_back(&m_uridUnmapFeature);
    m_features.push_back(&m_idleFeature);
    m_features.push_back(&m_parentFeature);
    m_features.push_back(&m_resizeFeature);
    m_features.push_back(&m_instanceFeature);
    m_features.push_back(&m_dataFeature);
    m_features.push_back(&m_optionsFeature);
    m_features.push_back(&m_extHostFeature);
    m_features.push_back(nullptr);

    // Get the Idle Interface if any.
    const void* ii = nullptr;
    if (uidesc->extension_data)
        ii = uidesc->extension_data(LV2_UI__idleInterface);
    m_lv2II = (LV2UI_Idle_Interface*)ii;

    // Instantiate the UI.
    m_handle =
        uidesc->instantiate(uidesc,  // descriptor
                            id.toStdString().c_str(),  // plugin_uri
                            ubp,  // bundle_path
                            writeFn,  // write_function
                            this,  // controller
                            &m_widget,  // widget
                            m_features.data());  // features

    RG_DEBUG << "handle:" << m_handle << "widget:" << m_widget;

    if (m_uiType == AudioPluginLV2GUI::GTK) {
        LV2Gtk* lv2gtk = LV2Gtk::getInstance();
        m_gwidget = lv2gtk->getWidget(m_widget, this);
        int width;
        int height;
        lv2gtk->getSize(m_gwidget, width, height);
        resize(width, height);

        const WId wid = (WId)(lv2gtk->getWinId(m_gwidget));
        RG_DEBUG << "create gtk window from" << hex << wid;
        m_parentWindow = QWindow::fromWinId(wid);
        m_parentWindow->setFlags(Qt::FramelessWindowHint);
        m_containerWidget = QWidget::createWindowContainer(m_parentWindow);
        m_containerWidget->setMinimumSize(QSize(width, height));
        m_containerWidget->setParent(this);
        RG_DEBUG << "got gtk window" << m_parentWindow <<
            m_parentWindow->parent();
    }
}

AudioPluginLV2GUIWindow::~AudioPluginLV2GUIWindow()
{
    RG_DEBUG << "~AudioPluginLV2GUIWindow";
    m_timer->stop();
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
    if (m_uiType == AudioPluginLV2GUI::KX) {
        LV2_External_UI_Widget* euw = (LV2_External_UI_Widget*)m_widget;
        euw->show(euw);
    } else {
        show();
    }
}

LV2UI_Handle AudioPluginLV2GUIWindow::getHandle() const
{
    return m_handle;
}

void AudioPluginLV2GUIWindow::uiClosed()
{
    RG_DEBUG << "ui closed";

    // Signal slotTimeUp() that we need a close.
    // Can't do much here as this may be called from a different thread.
    m_shutdownRequested = true;
}

void AudioPluginLV2GUIWindow::setSize(int width, int height, bool isRequest)
{
    RG_DEBUG << "setSize" << width << height << isRequest;

    // Disallow shrinking of both dimensions.
    if (this->width() >= width && this->height() >= height) return;

    resize(width, height);
}

void AudioPluginLV2GUIWindow::slotTimeUp()
{
    // Handle shutdown.
    if (m_shutdownRequested) {
        RG_DEBUG << "slotTimeUp shutdown requested";
        m_timer->stop();
        // this will cuase this object to be deleted
        m_lv2Gui->closeUI();
        return;
    }

    // Call idle handler.
    if (m_lv2II) m_lv2II->idle(m_handle);

    // Update control outs.
    m_lv2Gui->updateControlOutValues();

    // For kx, call run().
    if (m_uiType == AudioPluginLV2GUI::KX) {
        LV2_External_UI_Widget* euw = (LV2_External_UI_Widget*)m_widget;
        euw->run(euw);
    }
}

void AudioPluginLV2GUIWindow::closeEvent(QCloseEvent* event)
{
    RG_DEBUG << "closeEvent";

    event->ignore();

    m_timer->stop();

    // tell the ui to tidy up
    if (m_parentWindow) m_parentWindow->setParent(nullptr);

    LV2Gtk* lv2gtk = LV2Gtk::getInstance();
    lv2gtk->deleteWidget(m_gwidget);

    // this will cause this object to be deleted
    m_lv2Gui->closeUI();
}


}
