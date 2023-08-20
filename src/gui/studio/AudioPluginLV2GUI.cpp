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
#include "AudioPluginLV2GUIX11Window.h"

#include <QTimer>

#include <dlfcn.h>

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/AudioPluginInstance.h"
#include "sound/LV2Urid.h"
#include "sound/LV2Utils.h"

namespace Rosegarden
{

AudioPluginLV2GUI::AudioPluginLV2GUI(AudioPluginInstance *instance,
                                     RosegardenMainWindow *mainWindow,
                                     InstrumentId instrument,
                                     int position) :

    m_pluginInstance(instance),
    m_mainWindow(mainWindow),
    m_instrument(instrument),
    m_position(position)
{
    m_id = strtoqstr(m_pluginInstance->getIdentifier());
    LV2Utils* lv2utils = LV2Utils::getInstance();
    const LilvPlugin* plugin = lv2utils->getPluginByUri(m_id);

    if (! plugin) {
        RG_DEBUG << "failed to get plugin " << m_id;
        return;
    }
    LilvNode* name = lilv_plugin_get_name(plugin);
    QString sname = lilv_node_as_string(name);
    RG_DEBUG << "got plugin " << sname;

    //ui types:
    // http://lv2plug.in/ns/extensions/ui#GtkUI (calf)
    // http://lv2plug.in/ns/extensions/ui#X11UI (guitarix)

    LilvNode* x11UI = lv2utils->makeURINode(LV2_UI__X11UI);
    m_uis = lilv_plugin_get_uis(plugin);
    const LilvUI* selectedUI = nullptr;
    LILV_FOREACH(uis, it, m_uis) {
        const LilvUI* ui = lilv_uis_get(m_uis, it);
        if (lilv_ui_is_a(ui, x11UI)) {
            selectedUI = ui;
            break;
        }
    }
    lilv_node_free(x11UI);
    if (! selectedUI)
        {
            RG_DEBUG << "no usable ui found";
            return;
        }
    const LilvNode *uiUri = lilv_ui_get_uri(selectedUI);
    QString uiUris = lilv_node_as_string(uiUri);
    RG_DEBUG << "ui uri:" << uiUris;
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
    int numInst = lv2utils->numInstances(instrument, position);
    RG_DEBUG << "num instances: " << numInst;
    QString title = sname;

    for(int i=0; i<numInst; ++i) {
        QString chanStr = " / ";
        if (numInst > 1) {
            if (i == 0) chanStr += "Left";
            else if (i == 1) chanStr += "Right";
            else chanStr =  QString::number(i+1);
            title = sname + chanStr;
        }

        m_windows.push_back(new AudioPluginLV2GUIX11Window(this,
                                                           title,
                                                           i,
                                                           selectedUI,
                                                           m_uidesc,
                                                           m_id));
    }

    lv2utils->registerGUI(instrument, position, this);
}

AudioPluginLV2GUI::~AudioPluginLV2GUI()
{
    LV2Utils* lv2utils = LV2Utils::getInstance();
    for(auto win : m_windows) {
        delete win;
    }
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
    for(auto win : m_windows) {
        win->showGui();
    }
}

void
AudioPluginLV2GUI::portChange(int channel,
                              uint32_t portIndex,
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
                                           channel,
                                           *value);
    } else {
        // complex data
        QByteArray ba(static_cast<const char*>(buffer), bufferSize);
        /* !!! m_mainWindow->slotChangePluginPortBuf(m_instrument,
                                              m_position,
                                              portIndex,
                                              ba); */
    }
}

void
AudioPluginLV2GUI::updatePortValue(int, float)
{
    //m_uidesc->port_event(m_handle, port, sizeof(float), 0, &value);
}

}
