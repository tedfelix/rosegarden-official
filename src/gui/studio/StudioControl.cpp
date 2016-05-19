/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
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
#include "sound/ImmediateNote.h"

#include <QByteArray>
#include <QDataStream>
#include <QString>


namespace Rosegarden
{

ImmediateNote *
StudioControl::
getFiller(void)
{
    m_instanceMutex.lock();
    if (!m_immediateNoteFiller)
        { m_immediateNoteFiller = new ImmediateNote(); }
    m_instanceMutex.unlock();
    return m_immediateNoteFiller;
}

ImmediateNote *
StudioControl::
m_immediateNoteFiller = 0;

QMutex
StudioControl::m_instanceMutex;
    
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

MappedObjectId
StudioControl::getStudioObjectByType(MappedObject::MappedObjectType type)
{
    return RosegardenSequencer::getInstance()->getMappedObjectId(type);
}

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

MappedObjectPropertyList
StudioControl::getPluginInformation()
{
    return RosegardenSequencer::getInstance()->getPluginInformation();
}

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

void
StudioControl::
playPreviewNote(Instrument *instrument, int pitch,
                int velocity, RealTime duration, bool oneshot)
{
    MappedEventList mC;
    ImmediateNote * filler = getFiller();
    filler->fillWithNote(mC, instrument, pitch, velocity, duration, oneshot);
    sendMappedEventList(mC);
}

void
StudioControl::
sendChannelSetup(Instrument *instrument, int channel)
{
    MappedEventList mappedEventList;
    MappedEventInserter inserter(mappedEventList);

    // Insert bank selects and program change.
    // Acquire it from ChannelManager.  Passing -1 for trackId which
    // is unused here.
    ChannelManager::insertProgramForInstrument(channel, instrument,
                                               inserter,
                                               RealTime::zeroTime, -1);

    ChannelManager::SimpleCallbacks callbacks;

    // Insert controllers (and pitch bend).
    ChannelManager::insertControllers(channel, instrument,
                                      inserter, RealTime::zeroTime,
                                      RealTime::zeroTime,
                                      &callbacks, -1);

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

    // Acquire it from ChannelManager.  Passing -1 for trackId which
    // is unused here.
    ChannelManager::insertController(channel, instrument, inserter,
                                     RealTime::zeroTime, -1,
                                     controller, value);
    sendMappedEventList(mC);
}

}
