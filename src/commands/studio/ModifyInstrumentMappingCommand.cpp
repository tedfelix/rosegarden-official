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


#include "ModifyInstrumentMappingCommand.h"

#include "base/Composition.h"
#include "base/MidiProgram.h"
#include "base/Studio.h"
#include "base/Track.h"
#include "document/RosegardenDocument.h"
#include <QString>


namespace Rosegarden
{

ModifyInstrumentMappingCommand::ModifyInstrumentMappingCommand(
    RosegardenDocument *doc,
    InstrumentId fromInstrument,
    InstrumentId toInstrument):
        NamedCommand(getGlobalName()),
        m_composition(&doc->getComposition()),
        m_studio(&doc->getStudio()),
        m_fromInstrument(fromInstrument),
        m_toInstrument(toInstrument)
{}

void
ModifyInstrumentMappingCommand::execute()
{
    Composition::trackcontainer &tracks =
        m_composition->getTracks();
    Composition::trackcontainer::iterator it = tracks.begin();

    for (; it != tracks.end(); ++it) {
        if (it->second->getInstrument() == m_fromInstrument) {
            m_mapping.push_back(it->first);
            it->second->setInstrument(m_toInstrument);
        }
    }

}

void
ModifyInstrumentMappingCommand::unexecute()
{
    std::vector<TrackId>::iterator it = m_mapping.begin();
    Track *track = 0;

    for (; it != m_mapping.end(); ++it) {
        track = m_composition->getTrackById(*it);
        track->setInstrument(m_fromInstrument);
    }
}

}
