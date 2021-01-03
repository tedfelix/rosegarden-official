/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_STUDIOCONTROL_H
#define RG_STUDIOCONTROL_H

#include "gui/seqmanager/ChannelManager.h"
#include "base/MidiProgram.h"
#include "sound/MappedCommon.h"
#include "sound/MappedStudio.h"
#include "base/RealTime.h"

#include <QMutex>
#include <QString>


namespace Rosegarden
{


class MappedInstrument;
class MappedEvent;
class MappedEventList;
class Instrument;


/// Global interface for plugin, audio, and MIDI configuration.
/**
 * Historically, this was an important class.  It's now a trivial layer
 * that can be removed.
 *
 * StudioControl was first introduced in r3007, 9/9/2002.  It originally
 * served to wrap the DCOP interface to the sequencer process.
 * https://sourceforge.net/p/rosegarden/code/3007
 *
 * The sequencer process was moved into a thread in r9071, 9/1/2008.
 * As a result, StudioControl was reduced to simply delegating directly
 * to RosegardenSequencer.
 * https://sourceforge.net/p/rosegarden/code/9071
 *
 * Since this class does little more than delegate to RosegardenSequencer,
 * it should be removed and callers should call RosegardenSequencer
 * directly.  There are portions of this that do more than delegation and
 * they should be moved elsewhere (e.g. into RosegardenSequencer) to get
 * rid of this class.
 */
class StudioControl
{
public:

    // *** Object management

    /**
     * RosegardenDocument calls this to create AudioBuss, AudioInput,
     * AudioFader, and PluginSlot objects.  RosegardenMainWindow calls
     * this to create PluginSlot objects.  This is never called to
     * create Studio or PluginPort objects.
     */
    static MappedObjectId
        createStudioObject(MappedObject::MappedObjectType type);

    //static MappedObjectId
    //    getStudioObjectByType(MappedObject::MappedObjectType type);

    /// This is used to destroy plugin (slot?) objects.
    static bool destroyStudioObject(MappedObjectId id);


    // *** Properties

    /// Used by AudioPluginDialog to get programs for a plugin.
    static MappedObjectPropertyList getStudioObjectProperty(
            MappedObjectId id,
            const MappedObjectProperty &property);

    /// Used to set things like audio levels (MappedAudioFader::FaderLevel).
    static bool setStudioObjectProperty(MappedObjectId id,
                                        const MappedObjectProperty &property,
                                        MappedObjectValue value);

    /// Set many properties.
    /**
     * Used by RosegardenDocument::initialiseStudio() to set up the studio.
     */
    static bool setStudioObjectProperties(
            const MappedObjectIdList &ids,
            const MappedObjectPropertyList &properties,
            const MappedObjectValueList &values);

    /// Set a value to a string
    /**
     * Used to set plugin identifier (name) and program.
     */
    static bool setStudioObjectProperty(MappedObjectId id,
                                        const MappedObjectProperty &property,
                                        const QString &value);

    /// Set a value to a string list.  Return value is error if any.
    /**
     * Used for plugin configuration (MappedPluginSlot::Configuration).
     */
    static QString setStudioObjectPropertyList(MappedObjectId id,
					       const MappedObjectProperty &property,
					       const MappedObjectPropertyList &values);


    // *** Plugins

    static void setStudioPluginPort(MappedObjectId pluginId,
                                    unsigned long portId,
                                    MappedObjectValue value);

    static MappedObjectValue getStudioPluginPort(MappedObjectId pluginId,
                                                 unsigned long portId);

    /// Get all plugin information.
    //static MappedObjectPropertyList getPluginInformation();

    /// Get program name for a given bank/program.
    static QString getPluginProgram(MappedObjectId pluginId,
                                    int bank, int program);

    /// Get bank/program for a given name.
    /**
     * Return value is bank << 16 + program.
     */
    static unsigned long getPluginProgram(MappedObjectId pluginId,
                                          QString name);


    // *** Audio Connections

    /// Used to connect audio ins and outs to audio instruments.
    static void connectStudioObjects(MappedObjectId id1,
                                     MappedObjectId id2);
    static void disconnectStudioObjects(MappedObjectId id1,
                                        MappedObjectId id2);
    static void disconnectStudioObject(MappedObjectId id);


    // *** Send via MIDI

    /// Send a MIDI event immediately.
    static void sendMappedEvent(const MappedEvent &event);
    /// Send multiple MIDI events immediately.
    static void sendMappedEventList(const MappedEventList &eventList);

    /// Send mapped instrument to the sequencer.
    static void sendMappedInstrument(const MappedInstrument &mI);

    /// Delegate to RosegardenSequencer.
    static void sendQuarterNoteLength(const RealTime &length);

    /// Used by the matrix and notation editors to play notes.
    static void playPreviewNote(Instrument *instrument, int pitch,
                                int velocity, RealTime duration,
                                bool oneshot = true);

    /// Send bank selects, program changes, and controllers immediately.
    /**
     * Set up a channel for output.
     *
     * This is used for fixed channel instruments and also to set up
     * MIDI thru channels.
     */
    static void sendChannelSetup(Instrument *instrument, int channel);

    /// Send a control change immediately.
    /**
     * ??? rename: sendControlChange()
     */
    static void sendController(const Instrument *instrument, int channel,
                               MidiByte controller, MidiByte value);

 private:
    static ChannelManager m_channelManager;

    /// Used by playPreviewNote() to insert a note into a MappedEventList.
    static void fillWithImmediateNote(
            MappedEventList &mappedEventList, Instrument *instrument,
            int pitch, int velocity, RealTime duration, bool oneshot);

};

}

#endif
