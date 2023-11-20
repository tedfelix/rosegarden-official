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

#define RG_MODULE_STRING "[AudioPluginLV2GUI]"
#define RG_NO_DEBUG_PRINT 1

#include "AudioPluginLV2GUI.h"
#include "AudioPluginLV2GUIWindow.h"

#include <QTimer>

#include <dlfcn.h>

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/AudioPluginInstance.h"
#include "sound/LV2Utils.h"

// the kx.studio extension
#include "gui/studio/lv2_external_ui.h"

namespace Rosegarden
{

AudioPluginLV2GUI::AudioPluginLV2GUI(AudioPluginInstance *instance,
                                     RosegardenMainWindow *mainWindow,
                                     InstrumentId instrument,
                                     int position) :

    m_pluginInstance(instance),
    m_mainWindow(mainWindow),
    m_instrument(instrument),
    m_position(position),
    m_window(nullptr),
    m_firstUpdate(true)
{
    m_id = strtoqstr(m_pluginInstance->getIdentifier());
    LV2Utils* lv2utils = LV2Utils::getInstance();
    m_atomTransferUrid = lv2utils->uridMap(LV2_ATOM__eventTransfer);
    const LilvPlugin* plugin = lv2utils->getPluginByUri(m_id);

    if (! plugin) {
        RG_DEBUG << "failed to get plugin " << m_id;
        return;
    }
    LilvNode* name = lilv_plugin_get_name(plugin);
    QString sname = lilv_node_as_string(name);
    RG_DEBUG << "got plugin " << sname;

    //ui types:
    // http://lv2plug.in/ns/extensions/ui#GtkUI (eg. calf)
    // http://lv2plug.in/ns/extensions/ui#X11UI (eg. guitarix)
    // kx studio external widget extension
    UIType uiType = NONE;
    LilvNode* x11UI = lv2utils->makeURINode(LV2_UI__X11UI);
    LilvNode* gtkUI = lv2utils->makeURINode(LV2_UI__GtkUI);
    LilvNode* kxUI = lv2utils->makeURINode(LV2_EXTERNAL_UI__Widget);

    m_uis = lilv_plugin_get_uis(plugin);
    const LilvUI* selectedUI = nullptr;
    LILV_FOREACH(uis, it, m_uis) {
        const LilvUI* ui = lilv_uis_get(m_uis, it);
        if (lilv_ui_is_a(ui, x11UI)) {
            selectedUI = ui;
            uiType = X11;
            break;
        }
        if (lilv_ui_is_a(ui, gtkUI)) {
            selectedUI = ui;
            uiType = GTK;
            break;
        }
        if (lilv_ui_is_a(ui, kxUI)) {
            selectedUI = ui;
            uiType = KX;
            break;
        }
    }
    lilv_node_free(x11UI);
    lilv_node_free(gtkUI);
    lilv_node_free(kxUI);
    if (! selectedUI)
        {
            RG_DEBUG << "no usable ui found";
            return;
        }
    const LilvNode *uiUri = lilv_ui_get_uri(selectedUI);
    QString uiUris = lilv_node_as_string(uiUri);
    RG_DEBUG << "ui uri:" << uiUris << uiType;
    const LilvNode* uib = lilv_ui_get_binary_uri(selectedUI);
    QString binary_uri = lilv_node_as_uri(uib);
    QString bpath =
        lilv_file_uri_parse(binary_uri.toStdString().c_str(), nullptr);
    RG_DEBUG << "ui binary:" << bpath;

    m_uilib = dlopen(bpath.toStdString().c_str(), RTLD_LOCAL | RTLD_LAZY);
    void* vpdf = dlsym(m_uilib, "lv2ui_descriptor");
    LV2UI_DescriptorFunction pdf = (LV2UI_DescriptorFunction)vpdf;
    int ui_index = 0;
    m_uidesc = nullptr;
    while(true) {
        const LV2UI_Descriptor* uidesci = (*pdf)(ui_index);
        if (uidesci == nullptr) break;
        if (QString(uidesci->URI) == uiUris)
            {
                m_uidesc = uidesci;
                break;
            }
        ui_index++;
    }
    RG_DEBUG << "descriptor: " << m_uidesc;
    QString title = sname;

    m_window = new AudioPluginLV2GUIWindow(this,
                                           title,
                                           selectedUI,
                                           m_uidesc,
                                           m_id,
                                           uiType);

    lv2utils->registerGUI(instrument, position, this);
}

AudioPluginLV2GUI::~AudioPluginLV2GUI()
{
    RG_DEBUG << "~AudioPluginLV2GUI";
    if (m_window) {
        LV2UI_Handle handle = m_window->getHandle();
        if (m_uidesc) {
            m_uidesc->cleanup(handle);
        }
        delete m_window;
        m_window = nullptr;
    }
    LV2Utils* lv2utils = LV2Utils::getInstance();
    lv2utils->unRegisterGUI(m_instrument, m_position);
    lilv_uis_free(m_uis);
}

QString
AudioPluginLV2GUI::getId() const
{
    return m_id;
}

bool
AudioPluginLV2GUI::hasGUI() const
{
    return (m_uidesc != nullptr);
}

void
AudioPluginLV2GUI::showGui() const
{
    if (m_window) m_window->showGui();
}

void
AudioPluginLV2GUI::portChange(uint32_t portIndex,
                              uint32_t bufferSize,
                              uint32_t portProtocol,
                              const void *buffer)
{
    RG_DEBUG << portIndex << bufferSize << portProtocol;
    if (portProtocol == 0) {
        // normal port with float
        const float* value = static_cast<const float*>(buffer);
        m_mainWindow->slotChangePluginPort(m_instrument,
                                           m_position,
                                           portIndex,
                                           *value);
    } else {
        // complex data
        LV2Utils* lv2utils = LV2Utils::getInstance();
        RG_DEBUG << "complex data" << portProtocol <<
            "urid:" << lv2utils->uridUnmap(portProtocol);
        QByteArray ba(static_cast<const char*>(buffer), bufferSize);

        lv2utils->setPortValue(m_instrument, m_position,
                               portIndex, portProtocol, ba);
    }
}

void
AudioPluginLV2GUI::updatePortValue(int port, float value)
{
    if (! m_window) return;
    LV2UI_Handle handle = m_window->getHandle();
    m_uidesc->port_event(handle, port, sizeof(float), 0, &value);
}

void AudioPluginLV2GUI::updatePortValue(int port, const LV2_Atom* atom)
{
    RG_DEBUG << "updatePortValue" << port;
    if (! m_window) return;
    LV2UI_Handle handle = m_window->getHandle();
    int size = atom->size + 8;
    m_uidesc->port_event(handle, port, size, m_atomTransferUrid, atom);
}

void AudioPluginLV2GUI::checkControlOutValues()
{
    LV2Utils* lv2utils = LV2Utils::getInstance();
    std::map<int, float> newControlOutValues;
    lv2utils->getControlOutValues(m_instrument,
                                  m_position,
                                  newControlOutValues);
    for(auto pair : newControlOutValues) {
        int portIndex = pair.first;
        float value = pair.second;
        if (m_firstUpdate || m_controlOutValues[portIndex] != value) {
            m_controlOutValues[portIndex] = value;
            updatePortValue(portIndex, value);
        }
    }
    m_firstUpdate = false;

}

}