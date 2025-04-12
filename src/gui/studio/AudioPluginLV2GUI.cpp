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

#define RG_MODULE_STRING "[AudioPluginLV2GUI]"
#define RG_NO_DEBUG_PRINT 1

#include "AudioPluginLV2GUI.h"

#include "AudioPluginLV2GUIWindow.h"
#include "AudioPluginLV2GUIManager.h"
#include "sound/LV2PluginInstance.h"
#include "sound/LV2URIDMapper.h"

#include "sound/AudioInstrumentMixer.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/AudioPluginInstance.h"
#include "gui/application/RosegardenMainWindow.h"
#include "document/RosegardenDocument.h"

// the kx.studio extension
#include "gui/studio/lv2_external_ui.h"

#include <dlfcn.h>


namespace Rosegarden
{


AudioPluginLV2GUI::AudioPluginLV2GUI(AudioPluginInstance *instance,
                                     RosegardenMainWindow *mainWindow,
                                     InstrumentId instrument,
                                     int position,
                                     AudioPluginLV2GUIManager* manager) :
    m_mainWindow(mainWindow),
    m_manager(manager),
    m_pluginInstance(instance),
    m_instrument(instrument),
    m_position(position),
    m_uis(nullptr),
    m_uilib(nullptr),
    m_uidesc(nullptr),
    m_window(nullptr),
    m_firstUpdate(true),
    m_selectedUI(nullptr),
    m_uiType(NONE)
{
    m_id = strtoqstr(m_pluginInstance->getIdentifier());
    LV2Utils* lv2utils = LV2Utils::getInstance();
    m_atomTransferUrid = LV2URIDMapper::uridMap(LV2_ATOM__eventTransfer);
    const LilvPlugin* plugin = lv2utils->getPluginByUri(m_id);

    if (! plugin) {
        RG_DEBUG << "failed to get plugin " << m_id;
        return;
    }
    LilvNode* name = lilv_plugin_get_name(plugin);
    QString sname = lilv_node_as_string(name);
    RG_DEBUG << "got plugin " << sname;
    RosegardenDocument *doc = RosegardenDocument::currentDocument;
    Studio &studio = doc->getStudio();

    // Get the appropriate instrument based on the ID.
    if (m_instrument < AudioInstrumentBase) {
        // submaster
        m_title = tr("Sub %1").arg(m_instrument) + " " + sname;
    } else {
        Instrument *inst = studio.getInstrumentById(m_instrument);
        m_title = strtoqstr(inst->getPresentationName()) + " " + sname;
    }

    //ui types:
    // http://lv2plug.in/ns/extensions/ui#GtkUI (eg. calf)
    // http://lv2plug.in/ns/extensions/ui#X11UI (eg. guitarix)
    // kx studio external widget extension
    LilvNode* x11UI = lv2utils->makeURINode(LV2_UI__X11UI);
    LilvNode* gtkUI = lv2utils->makeURINode(LV2_UI__GtkUI);
    LilvNode* kxUI = lv2utils->makeURINode(LV2_EXTERNAL_UI__Widget);

    m_uis = lilv_plugin_get_uis(plugin);
    LILV_FOREACH(uis, it, m_uis) {
        const LilvUI* ui = lilv_uis_get(m_uis, it);
        if (lilv_ui_is_a(ui, x11UI)) {
            m_selectedUI = ui;
            m_uiType = X11;
            break;
        }
#ifdef HAVE_GTK2
        if (lilv_ui_is_a(ui, gtkUI)) {
            m_selectedUI = ui;
            m_uiType = GTK;
            break;
        }
#endif
        if (lilv_ui_is_a(ui, kxUI)) {
            m_selectedUI = ui;
            m_uiType = KX;
            break;
        }
    }
    lilv_node_free(x11UI);
    lilv_node_free(gtkUI);
    lilv_node_free(kxUI);
    if (! m_selectedUI)
        {
            RG_DEBUG << "no usable ui found";
            return;
        }
    const LilvNode *uiUri = lilv_ui_get_uri(m_selectedUI);
    QString uiUris = lilv_node_as_string(uiUri);
    RG_DEBUG << "ui uri:" << uiUris << m_uiType;
    const LilvNode* uib = lilv_ui_get_binary_uri(m_selectedUI);
    QString binary_uri = lilv_node_as_uri(uib);
    QString bpath =
        lilv_file_uri_parse(binary_uri.toStdString().c_str(), nullptr);
    RG_DEBUG << "ui binary:" << bpath;

    // Find the UI descriptor for uiUris.

    m_uilib = dlopen(bpath.toStdString().c_str(), RTLD_LOCAL | RTLD_LAZY);
    if (m_uilib) {
        void* vpdf = dlsym(m_uilib, "lv2ui_descriptor");
        if (vpdf) {
            LV2UI_DescriptorFunction pdf = (LV2UI_DescriptorFunction)vpdf;
            int ui_index = 0;
            while(true) {
                const LV2UI_Descriptor* uidesci = (*pdf)(ui_index);
                if (uidesci == nullptr) break;
                if (QString(uidesci->URI) == uiUris)
                    {
                        m_uidesc = uidesci;
                        break;
                    }
                ui_index++;
                RG_DEBUG << "descriptor: " << m_uidesc;
            }
        }
    }
}

AudioPluginLV2GUI::~AudioPluginLV2GUI()
{
    RG_DEBUG << "~AudioPluginLV2GUI";

    // Clear the LV2PluginInstance gui pointer.
    AudioInstrumentMixer *aim = AudioInstrumentMixer::getInstance();
    if (aim) {
        RunnablePluginInstance *rpi =
                aim->getPluginInstance(m_instrument, m_position);
        if (rpi) {
            LV2PluginInstance *lpi = dynamic_cast<LV2PluginInstance *>(rpi);
            if (lpi)
                lpi->setGUI(nullptr);
        }
    }

    if (m_window) {
        LV2UI_Handle handle = m_window->getHandle();
        if (m_uidesc) {
            m_uidesc->cleanup(handle);
        }
        RG_DEBUG << "~AudioPluginLV2GUI delete window";
        delete m_window;
        m_window = nullptr;
    }
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
    RG_DEBUG << "hasGui" << m_uidesc;
    return (m_uidesc != nullptr);
}

void
AudioPluginLV2GUI::showGui()
{
    if (! m_uidesc) return;
    LV2Utils* lv2utils = LV2Utils::getInstance();
    if (! m_window) {
        // create the window now
        m_window = new AudioPluginLV2GUIWindow(this,
                                               m_title,
                                               m_selectedUI,
                                               m_uidesc,
                                               m_id,
                                               m_uiType);
    }
    m_window->showGui();
    // set the control in values correctly
    // this must be done after showGui to avoid problems with the gtk embedding
    LV2Utils::PortValues controlInValues;
    lv2utils->getControlInValues(m_instrument,
                                 m_position,
                                 controlInValues);
    // For each control in value, update the UI.
    for (const auto &pair : controlInValues) {
        int portIndex = pair.first;
        float value = pair.second;
        updatePortValue(portIndex, value);
    }
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
            "urid:" << LV2URIDMapper::uridUnmap(portProtocol);
        QByteArray ba(static_cast<const char*>(buffer), bufferSize);

        lv2utils->setPortValue(m_instrument, m_position,
                               portIndex, portProtocol, ba);
    }
}

void
AudioPluginLV2GUI::updatePortValue(int port, float value)
{
    //RG_DEBUG << "updatePortValue" << port;
    if (! m_window) return;
    LV2UI_Handle handle = m_window->getHandle();
    if (m_uidesc && m_uidesc->port_event) {
        m_uidesc->port_event(handle, port, sizeof(float), 0, &value);
    }
}

void AudioPluginLV2GUI::updatePortValue(int port, const LV2_Atom* atom)
{
    //RG_DEBUG << "updatePortValue" << port;
    if (! m_window) return;
    LV2UI_Handle handle = m_window->getHandle();
    int size = atom->size + 8;
    if (m_uidesc && m_uidesc->port_event)
        m_uidesc->port_event(handle, port, size, m_atomTransferUrid, atom);
}

void AudioPluginLV2GUI::updateControlOutValues()
{
    LV2Utils* lv2utils = LV2Utils::getInstance();
    LV2Utils::PortValues newControlOutValues;
    lv2utils->getControlOutValues(m_instrument,
                                  m_position,
                                  newControlOutValues);
    // For each control out value, update any that have changed.
    for (const auto &pair : newControlOutValues) {
        int portIndex = pair.first;
        float value = pair.second;
        if (m_firstUpdate || m_controlOutValues[portIndex] != value) {
            m_controlOutValues[portIndex] = value;
            updatePortValue(portIndex, value);
        }
    }
    m_firstUpdate = false;

    // and trigger port updates

    LV2PluginInstance *pi = getPluginInstance();
    if (pi)
        pi->triggerPortUpdates();
}

LV2PluginInstance *AudioPluginLV2GUI::getPluginInstance() const
{
    AudioInstrumentMixer *aim = AudioInstrumentMixer::getInstance();
    if (!aim)
        return nullptr;

    RunnablePluginInstance *rpi =
            aim->getPluginInstance(m_instrument, m_position);
    if (!rpi)
        return nullptr;

    LV2PluginInstance *lpi = dynamic_cast<LV2PluginInstance *>(rpi);

    return lpi;
}

void AudioPluginLV2GUI::closeUI()
{
    RG_DEBUG << "closeUI";
    // this will cause this object to be deleted
    m_manager->stopGUI(m_instrument,
                       m_position);
}


}
