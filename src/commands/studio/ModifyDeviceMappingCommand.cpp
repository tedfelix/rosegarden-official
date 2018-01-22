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

#define RG_MODULE_STRING "[ModifyDeviceMappingCommand]"

#include "ModifyDeviceMappingCommand.h"

#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/Device.h"
#include "base/Instrument.h"
#include "base/MidiProgram.h"
#include "base/Studio.h"
#include "base/Track.h"
#include "document/RosegardenDocument.h"
#include <QString>


namespace Rosegarden
{

ModifyDeviceMappingCommand::ModifyDeviceMappingCommand(
    RosegardenDocument *doc,
    DeviceId fromDevice,
    DeviceId toDevice):
        NamedCommand(getGlobalName()),
        m_composition(&doc->getComposition()),
        m_studio(&doc->getStudio()),
        m_fromDevice(fromDevice),
        m_toDevice(toDevice)
{}

void
ModifyDeviceMappingCommand::execute()
{
    Composition::trackcontainer &tracks =
        m_composition->getTracks();
    Composition::trackcontainer::iterator it = tracks.begin();
    Instrument *instr = 0;
    int index = 0;

    for (; it != tracks.end(); ++it) {
        instr = m_studio->getInstrumentById(it->second->getInstrument());
        if (!instr || !instr->getDevice())
            continue;

        if (instr->getDevice()->getId() == m_fromDevice) {
            // if source and target are MIDI
            if (m_studio->getDevice(m_fromDevice)->getType() ==
                    Device::Midi &&
                    m_studio->getDevice(m_toDevice)->getType() ==
                    Device::Midi) {
                // Try to match channels on the target device
                //
                MidiByte channel = instr->getNaturalChannel();

                InstrumentList destList = m_studio->
                                          getDevice(m_toDevice)->getPresentationInstruments();

                InstrumentList::iterator dIt = destList.begin();

                for (; dIt != destList.end(); ++dIt) {
                    if ((*dIt)->getNaturalChannel() == channel) {
                        break;
                    }
                }

                // Failure to match anything and there's no Instruments
                // at all in the destination.  Skip to next source Instrument.
                //
                if (dIt == destList.end() || destList.size() == 0)
                    continue;


                RG_DEBUG << " Track " << it->first
                << ", setting Instrument to "
                << (*dIt)->getId() << endl;

                // store "to" and "from" values
                //
                m_mapping.push_back(
                    std::pair < TrackId,
                    InstrumentId >
                    (it->first,
                     instr->getId()));

                it->second->setInstrument((*dIt)->getId());
            } else // audio is involved in the mapping - use indexes
            {
                // assign by index numbers
                InstrumentList destList = m_studio->
                                          getDevice(m_toDevice)->getPresentationInstruments();

                // skip if we can't match
                //
                if (index > (int)(destList.size() - 1))
                    continue;

                m_mapping.push_back(
                    std::pair < TrackId,
                    InstrumentId >
                    (it->first,
                     instr->getId()));

                it->second->setInstrument(destList[index]->getId());
            }

            index++;
        }
    }

}

void
ModifyDeviceMappingCommand::unexecute()
{
    std::vector<std::pair<TrackId, InstrumentId> >
    ::iterator it = m_mapping.begin();
    Track *track = 0;

    for (; it != m_mapping.end(); ++it) {
        track = m_composition->getTrackById(it->first);
        track->setInstrument(it->second);
    }
}

}
