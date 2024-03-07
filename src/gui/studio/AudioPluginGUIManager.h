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

#ifndef RG_AUDIOPLUGINGUIMANAGER_H
#define RG_AUDIOPLUGINGUIMANAGER_H

#include "base/AudioPluginInstance.h"


namespace Rosegarden
{


class AudioPluginOSCGUIManager;
class AudioPluginLV2GUIManager;
class RosegardenMainWindow;
class Studio;


/// Polymorphic interface to the various AudioPlugin*GUIManager classes.
/**
 * RosegardenMainWindow::m_pluginGUIManager is the only instance.
 *
 * ??? If AudioPluginOSCGUIManager and AudioPluginLV2GUIManager derived
 *     from a common base class, this class would not be necessary.  It is
 *     implementing a switch on type for a number of functions.  I'm
 *     not usually a fan of polymorphism, but it probably makes sense in this
 *     case.
 *
 *     I think we would still need this class as a single instance that will
 *     pass on to either DSSI or LV2. If AudioPluginOSCGUIManager and
 *     AudioPluginLV2GUIManager derive from a bas class
 *     (AudioPluginGUIManagerBase ?) this class could hold a map <gui type,
 *     AudioPluginGUIManagerBase> but it is still basically a class for
 *     distributing to the correct architecture.
 */
// cppcheck-suppress noCopyConstructor
class AudioPluginGUIManager
{
public:

    explicit AudioPluginGUIManager(RosegardenMainWindow *mainWindow);
    virtual ~AudioPluginGUIManager();

    void setStudio(Studio *studio);

    bool hasGUI(InstrumentId instrument, int position);
    void showGUI(InstrumentId instrument, int position);
    void stopGUI(InstrumentId instrument, int position);
    void stopAllGUIs();

    bool hasParameters(InstrumentId instrument, int position) const;
    void getParameters(InstrumentId instrument,
                       int position,
                       AudioPluginInstance::PluginParameters& params);
    void updatePluginParameter
        (InstrumentId instrument,
         int position,
         const QString& paramId,
         const AudioPluginInstance::PluginParameter& param);

    bool canUsePresets(InstrumentId instrument, int position) const;
    void getPresets(InstrumentId instrument,
                          int position,
                          AudioPluginInstance::PluginPresetList& presets);
    void setPreset(InstrumentId instrument, int position, const QString& uri);
    void loadPreset(InstrumentId instrument, int position, const QString& file);
    void savePreset(InstrumentId instrument, int position, const QString& file);

    bool canEditConnections(InstrumentId instrument, int position) const;
    void getConnections
        (InstrumentId instrument,
         int position,
         PluginPort::ConnectionList& clist) const;
    // cppcheck-suppress functionStatic
    void setConnections
        (InstrumentId instrument,
         int position,
         const PluginPort::ConnectionList& clist) const;

    void updateProgram(InstrumentId instrument, int position);
    void updatePort(InstrumentId instrument, int position, int port);
    void updateConfiguration(InstrumentId instrument, int position,
                             QString key);

private:
    enum PluginGUIArchitecture {UNKNOWN, OSC, LV2};
    PluginGUIArchitecture getArchitecture
        (InstrumentId instrument, int position) const;

    Studio *m_studio;

    AudioPluginOSCGUIManager *m_oscManager;
#ifdef HAVE_LILV
    AudioPluginLV2GUIManager *m_lv2Manager;
#endif

};

}

#endif
