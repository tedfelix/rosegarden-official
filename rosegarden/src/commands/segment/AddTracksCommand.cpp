/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "AddTracksCommand.h"

#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/MidiProgram.h"
#include "base/Track.h"
#include <qstring.h>


namespace Rosegarden
{

AddTracksCommand::AddTracksCommand(Composition *composition,
                                   unsigned int nbTracks,
                                   InstrumentId id):
        KNamedCommand(getGlobalName()),
        m_composition(composition),
        m_nbNewTracks(nbTracks),
        m_instrumentId(id),
        m_detached(false)

{
}

AddTracksCommand::~AddTracksCommand()
{
    if (m_detached) {
        for (unsigned int i = 0; i < m_newTracks.size(); ++i)
            delete m_newTracks[i];

        m_newTracks.clear();
    }
}

void AddTracksCommand::execute()
{
    // Re-attach tracks
    //
    if (m_detached) {
        for (unsigned int i = 0; i < m_newTracks.size(); i++)
            m_composition->addTrack(m_newTracks[i]);

        return ;
    }

    int highPosition = 0;
    Composition::trackiterator it =
        m_composition->getTracks().begin();

    for (; it != m_composition->getTracks().end(); ++it) {
        if ((*it).second->getPosition() > highPosition)
            highPosition = (*it).second->getPosition();
    }

    for (unsigned int i = 0; i < m_nbNewTracks; ++i) {
        TrackId trackId = m_composition->getNewTrackId();
        Track *track = new Track(trackId);

        track->setPosition(highPosition + 1 + i);
        track->setInstrument(m_instrumentId);

        m_composition->addTrack(track);
    }
}

void AddTracksCommand::unexecute()
{
    unsigned int startTrack = m_composition->getNbTracks();
    unsigned int endTrack = startTrack - m_nbNewTracks;

    for (unsigned int i = startTrack; i > endTrack; --i) {
        Track *track = m_composition->getTrackByPosition(i - 1);

        if (track) {
            if (m_detached == false)
                m_newTracks.push_back(track);
            m_composition->detachTrack(track);
        } else
            RG_DEBUG << "AddTracksCommand::unexecute - "
            << "can't detach track at position " << i << endl;
    }

    m_detached = true;
}

}
