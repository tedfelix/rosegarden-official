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


#include "MoveTracksCommand.h"

#include "base/Composition.h"
#include "base/Track.h"
#include <QString>


namespace Rosegarden
{

MoveTracksCommand::MoveTracksCommand(Composition *composition,
                                     TrackId srcTrack,
                                     TrackId destTrack):
        NamedCommand(getGlobalName()),
        m_composition(composition),
        m_srcTrack(srcTrack),
        m_destTrack(destTrack)
{}

MoveTracksCommand::~MoveTracksCommand()
{}

void
MoveTracksCommand::execute()
{
    Track *srcTrack = m_composition->getTrackById(m_srcTrack);
    Track *destTrack = m_composition->getTrackById(m_destTrack);

    // Swap positions
    const int srcPosition = srcTrack->getPosition();
    srcTrack->setPosition(destTrack->getPosition());
    destTrack->setPosition(srcPosition);

    m_composition->updateRefreshStatuses();
    m_composition->notifyTrackChanged(srcTrack);
    m_composition->notifyTrackChanged(destTrack);
}

void
MoveTracksCommand::unexecute()
{
    // ??? Given that this is the same as execute(), why not just call
    //     execute()?

    Track *srcTrack = m_composition->getTrackById(m_srcTrack);
    Track *destTrack = m_composition->getTrackById(m_destTrack);

    // Swap positions
    const int srcPosition = srcTrack->getPosition();
    srcTrack->setPosition(destTrack->getPosition());
    destTrack->setPosition(srcPosition);

    m_composition->updateRefreshStatuses();
    m_composition->notifyTrackChanged(srcTrack);
    m_composition->notifyTrackChanged(destTrack);
}

}
