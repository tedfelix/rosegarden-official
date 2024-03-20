/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_LV2UTILS_H
#define RG_LV2UTILS_H

#include "base/Instrument.h"
#include "base/AudioPluginInstance.h"
#include "sound/LV2PluginParameter.h"

#include <lilv/lilv.h>
#include <lv2/atom/atom.h>
#include <lv2/worker/worker.h>

#include <QString>

#include <map>
#include <queue>


namespace Rosegarden
{


class LV2PluginInstance;
class AudioPluginLV2GUI;


class LV2Utils
{
public:
    /// Singleton
    static LV2Utils *getInstance();


    // *** lilv wrappers and helpers ***

    // Most of these wrappers are used only by LV2PluginInstance.

    // lilv_plugins_get_by_uri() wrapper
    const LilvPlugin *getPluginByUri(const QString& uri) const;

    // lilv_port_get_index() wrapper
    int getPortIndexFromSymbol(const QString& portSymbol,
                               const LilvPlugin* plugin);

    // lilv_state_new_from_world() wrapper
    LilvState* getStateByUri(const QString& uri);

    // lilv_state_new_from_instance() wrapper
    LilvState* getStateFromInstance(const LilvPlugin* plugin,
                                    LilvInstance* instance,
                                    LilvGetPortValueFunc getPortValueFunc,
                                    LV2PluginInstance* lv2Instance,
                                    const LV2_Feature*const* features);

    // lilv_state_new_from_instance() wrapper
    QString getStateStringFromInstance(const LilvPlugin* plugin,
                                       const QString& uri,
                                       LilvInstance* instance,
                                       LilvGetPortValueFunc getPortValueFunc,
                                       LV2PluginInstance* lv2Instance,
                                       const LV2_Feature*const* features);

    // lilv_state_new_from_string() wrapper
    void setInstanceStateFromString(const QString& stateString,
                                    LilvInstance* instance,
                                    LilvSetPortValueFunc setPortValueFunc,
                                    LV2PluginInstance* lv2Instance,
                                    const LV2_Feature*const* features);

    // lilv_state_new_from_file() wrapper
    LilvState* getStateFromFile(const LilvNode* uriNode,
                                const QString& filename);

    // lilv_state_save() wrapper
    void saveStateToFile(const LilvState* state, const QString& filename);

    /// lilv_new_uri() wrapper
    LilvNode* makeURINode(const QString& uri) const;

    /// lilv_new_string() wrapper
    LilvNode* makeStringNode(const QString& string) const;

    /// Helper.  Gathers uri/label data into a vector.
    void setupPluginPresets(const QString& uri,
                            AudioPluginInstance::PluginPresetList& presets);


    // *** LV2 Plugin Instance Database ***

    /// Unique ID for a specific plugin instance.
    /**
     * Combines the InstrumentId and the position of the plugin within
     * that instrument.
     *
     * Used as a key type for plugin instance data and LV2Worker maps.
     *
     * See LV2PluginInstance::m_instrument and m_position.
     *
     * ??? Rename: PluginID, then see if it is needed elsewhere.  Promote
     *             to Instrument.
     */
    struct PluginPosition
    {
        InstrumentId instrument{NoInstrument};
        // Position in the effects stack for effect plugins,
        // 999 (Instrument::SYNTH_PLUGIN_POSITION) for synths.
        int position{0};

        bool operator<(const PluginPosition &p) const
        {
            if (instrument < p.instrument) return true;
            if (instrument > p.instrument) return false;
            if (position < p.position) return true;
            return false;
        }
    };


    /// Set a plugin port value.
    /**
     * Called by the plugin UI.
     */
    void setPortValue(InstrumentId instrument,
                      int position,
                      int index,
                      unsigned int protocol,
                      const QByteArray& data);

    typedef std::map<int /*portIndex*/, float /*value*/> PortValues;

    void getControlInValues(InstrumentId instrument,
                            int position,
                            PortValues &controlValues);

    void getControlOutValues(InstrumentId instrument,
                             int position,
                             PortValues &controlValues);

    /// Calls the plugin instance's runWork().
    /**
     * Called regularly by LV2Worker to send work back to a plugin.
     * Called by LV2Worker's worker thread which is the UI thread as of this
     * writing, so locking is not needed.
     */
    void runWork(const PluginPosition &pp,
                 uint32_t size,
                 const void *data,
                 LV2_Worker_Respond_Function resp);

    void getConnections
        (InstrumentId instrument,
         int position,
         PluginPort::ConnectionList& clist) const;
    // used in AudioPluginLV2GUIManager
    void setConnections
        (InstrumentId instrument,
         int position,
         const PluginPort::ConnectionList& clist) const;

    // Parameters
    void setupPluginParameters
        (const QString& uri, LV2PluginParameter::Parameters& params);
    bool hasParameters(InstrumentId instrument,
                       int position) const;
    void getParameters(InstrumentId instrument,
                       int position,
                       AudioPluginInstance::PluginParameters& params);
    void updatePluginParameter
        (InstrumentId instrument,
         int position,
         const QString& paramId,
         const AudioPluginInstance::PluginParameter& param);

    // Presets
    void getPresets(InstrumentId instrument,
                    int position,
                    AudioPluginInstance::PluginPresetList& presets) const;
    void setPreset(InstrumentId instrument,
                   int position,
                   const QString& uri);
    void loadPreset(InstrumentId instrument,
                    int position,
                    const QString& file);
    void savePreset(InstrumentId instrument,
                    int position,
                    const QString& file);


private:

    /// Singleton.  See getInstance().
    LV2Utils();
    ~LV2Utils();
    LV2Utils(LV2Utils &other) = delete;
    void operator=(const LV2Utils &) = delete;

    void fillParametersFromProperties(LV2PluginParameter::Parameters& params,
                                      const LilvNodes* properties,
                                      bool write);

};


}

#endif // RG_LV2UTILS_H
