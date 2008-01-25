/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
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


#include "MoveTracksCommand.h"

#include "base/Composition.h"
#include "base/Track.h"
#include <qstring.h>


namespace Rosegarden
{

MoveTracksCommand::MoveTracksCommand(Composition *composition,
                                     TrackId srcTrack,
                                     TrackId destTrack):
        KNamedCommand(getGlobalName()),
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

    int srcPosition = srcTrack->getPosition();

    srcTrack->setPosition(destTrack->getPosition());
    destTrack->setPosition(srcPosition);

    m_composition->updateRefreshStatuses();
}

void
MoveTracksCommand::unexecute()
{
    Track *srcTrack = m_composition->getTrackById(m_srcTrack);
    Track *destTrack = m_composition->getTrackById(m_destTrack);

    int srcPosition = srcTrack->getPosition();

    srcTrack->setPosition(destTrack->getPosition());
    destTrack->setPosition(srcPosition);

    m_composition->updateRefreshStatuses();
}

}
