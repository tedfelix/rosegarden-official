/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[StudioControl]"

#include "StudioControl.h"

#include "sound/Midi.h"
#include "misc/Debug.h"
#include "base/MidiProgram.h"
#include "base/Profiler.h"
#include "base/RealTime.h"
#include "gui/seqmanager/ChannelManager.h"
#include "sequencer/RosegardenSequencer.h"
#include "sound/MappedCommon.h"
#include "sound/MappedEventInserter.h"
#include "sound/MappedEventList.h"
#include "sound/MappedEvent.h"
#include "sound/MappedInstrument.h"
#include "sound/MappedStudio.h"

#include <QByteArray>
#include <QDataStream>
#include <QString>


namespace Rosegarden
{

ChannelManager StudioControl::m_channelManager(nullptr);

MappedObjectId
StudioControl::createStudioObject(MappedObject::MappedObjectType type)
{
    return RosegardenSequencer::getInstance()->createMappedObject(type);
}

bool
StudioControl::destroyStudioObject(MappedObjectId id)
{
    return RosegardenSequencer::getInstance()->destroyMappedObject(id);
}

MappedObjectPropertyList
StudioControl::getStudioObjectProperty(MappedObjectId id,
                                       const MappedObjectProperty &property)
{
    return RosegardenSequencer::getInstance()->getPropertyList(id, property);
}

bool
StudioControl::setStudioObjectProperty(MappedObjectId id,
                                       const MappedObjectProperty &property,
                                       MappedObjectValue value)
{
    RosegardenSequencer::getInstance()->setMappedProperty(id, property, value);
    return true;
}

bool
StudioControl::setStudioObjectProperties(const MappedObjectIdList &ids,
        const MappedObjectPropertyList &properties,
        const MappedObjectValueList &values)
{
    RosegardenSequencer::getInstance()->setMappedProperties
        (ids, properties, values);
    return true;
}

bool
StudioControl::setStudioObjectProperty(MappedObjectId id,
                                       const MappedObjectProperty &property,
                                       const QString &value)
{
    RosegardenSequencer::getInstance()->setMappedProperty(id, property, value);
    return true;
}

QString
StudioControl::setStudioObjectPropertyList(MappedObjectId id,
        const MappedObjectProperty &property,
        const MappedObjectPropertyList &values)
{
    QString error = RosegardenSequencer::getInstance()->setMappedPropertyList(id, property, values);
    return error;
}

#if 0
MappedObjectId
StudioControl::getStudioObjectByType(MappedObject::MappedObjectType type)
{
    return RosegardenSequencer::getInstance()->getMappedObjectId(type);
}
#endif

void
StudioControl::setStudioPluginPort(MappedObjectId pluginId,
                                   unsigned long portId,
                                   MappedObjectValue value)
{
    RosegardenSequencer::getInstance()->setMappedPort(pluginId, portId, value);
}

MappedObjectValue
StudioControl::getStudioPluginPort(MappedObjectId pluginId,
                                   unsigned long portId)
{
    return RosegardenSequencer::getInstance()->getMappedPort(pluginId, portId);
}

#if 0
MappedObjectPropertyList
StudioControl::getPluginInformation()
{
    return RosegardenSequencer::getInstance()->getPluginInformation();
}
#endif

QString
StudioControl::getPluginProgram(MappedObjectId id, int bank, int program)
{
    return RosegardenSequencer::getInstance()->getPluginProgram(id, bank, program);
}

unsigned long
StudioControl::getPluginProgram(MappedObjectId id, QString name)
{
    return RosegardenSequencer::getInstance()->getPluginProgram(id, name);
}

void
StudioControl::connectStudioObjects(MappedObjectId id1,
                                    MappedObjectId id2)
{
    RosegardenSequencer::getInstance()->connectMappedObjects(id1, id2);
}

void
StudioControl::disconnectStudioObjects(MappedObjectId id1,
                                       MappedObjectId id2)
{
    RosegardenSequencer::getInstance()->disconnectMappedObjects(id1, id2);
}

void
StudioControl::disconnectStudioObject(MappedObjectId id)
{
    RosegardenSequencer::getInstance()->disconnectMappedObject(id);
}

void
StudioControl::sendMappedEvent(const MappedEvent &mE)
{
    RosegardenSequencer::getInstance()->processMappedEvent(mE);
}

void
StudioControl::sendMappedEventList(const MappedEventList &mC)
{
    if (mC.size() == 0)
        return ;

    MappedEventList::const_iterator it = mC.begin();

    for (; it != mC.end(); ++it) {
        RosegardenSequencer::getInstance()->processMappedEvent(*it);
    }
}

void
StudioControl::sendMappedInstrument(const MappedInstrument &mI)
{
    RosegardenSequencer::getInstance()->setMappedInstrument(mI.getType(),
                                                            mI.getId());
}

void
StudioControl::sendQuarterNoteLength(const RealTime &length)
{
    RosegardenSequencer::getInstance()->setQuarterNoteLength(length);
}

// @author Tom Breton (Tehom)
void
StudioControl::fillWithImmediateNote(
        MappedEventList &mappedEventList, Instrument *instrument,
        int pitch, int velocity, RealTime duration, bool oneshot)
{
    if (!instrument)
        return;

#ifdef DEBUG_PREVIEW_NOTES
    RG_DEBUG << "fillWithNote() on" << (instrument->isPercussion() ? "percussion" : "non-percussion") << instrument->getName() << instrument->getId();
#endif

    if ((pitch < 0) || (pitch > 127))
        return;

    if (velocity < 0)
        velocity = 100;

    MappedEvent::MappedEventType type =
            oneshot ? MappedEvent::MidiNoteOneShot : MappedEvent::MidiNote;

    // Make the event.
    MappedEvent mappedEvent(
            instrument->getId(),
            type,
            pitch,
            velocity,
            RealTime::zeroTime,  // absTime
            duration,
            RealTime::zeroTime);  // audioStartMarker

    // Since we're not going thru MappedBufMetaIterator::acceptEvent()
    // which checks tracks for muting, we needn't set a track.

    // Set up channel manager.
    m_channelManager.setInstrument(instrument);
    m_channelManager.setEternalInterval();
    // ??? It's odd that we would say "false" for changedInstrument given
    //     that we did indeed change the Instrument.  Why are we doing this?
    m_channelManager.allocateChannelInterval(false);

    MappedEventInserter inserter(mappedEventList);

    // Insert the event.
    // Setting firstOutput to true indicates that we want a channel
    // setup.
    m_channelManager.insertEvent(
            NoTrack,  // trackId
            instrument->getStaticControllers(),
            RealTime::zeroTime,  // refTime
            mappedEvent,
            true,  // firstOutput
            inserter);
}

void
StudioControl::
playPreviewNote(Instrument *instrument, int pitch,
                int velocity, RealTime duration, bool oneshot)
{
    MappedEventList mC;
    fillWithImmediateNote(mC, instrument, pitch, velocity, duration, oneshot);
    sendMappedEventList(mC);
}

void
StudioControl::
sendChannelSetup(Instrument *instrument, int channel)
{
    MappedEventList mappedEventList;
    MappedEventInserter inserter(mappedEventList);

    // Insert BS, PC, CCs, and pitch bend.
    ChannelManager::insertChannelSetup(
            -1,  // trackId
            instrument,
            channel,
            RealTime::zeroTime,  // insertTime
            instrument->getStaticControllers(),
            inserter);

    // Send it out.
    sendMappedEventList(mappedEventList);
}

// Send a single controller to output.  This is used for fixed-channel
// instruments.
void
StudioControl::
sendController(const Instrument *instrument, int channel,
               MidiByte controller, MidiByte value)
{
    MappedEventList mC;
    MappedEventInserter inserter(mC);

    // Passing -1 for trackId which is unused here.
    ChannelManager::insertController(
            -1,  // trackId
            instrument,
            channel,
            RealTime::zeroTime,  // insertTime
            controller,
            value,
            inserter);

    sendMappedEventList(mC);
}

}
