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

#define RG_MODULE_STRING "[RenameTrackCommand]"

#include "RenameTrackCommand.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Composition.h"
#include "base/Track.h"
#include <QString>


namespace Rosegarden
{

RenameTrackCommand::RenameTrackCommand(Composition *composition,
                                       TrackId trackId,
                                       QString longName,
                                       QString shortName) :
        NamedCommand(getGlobalName()),
        m_composition(composition),
        m_trackId(trackId),
        m_newLongName(longName),
        m_newShortName(shortName)
{
    if (!m_composition)
        return;

    Track *track = composition->getTrackById(m_trackId);
    if (!track) {
        RG_DEBUG << "RenameTrackCommand: Cannot find track with ID " << m_trackId;
        return;
    }

    // Save the old name for unexecute (undo)
    m_oldLongName = QString::fromStdString(track->getLabel());
    m_oldShortName = QString::fromStdString(track->getShortLabel());
}

RenameTrackCommand::~RenameTrackCommand()
{}

void
RenameTrackCommand::execute()
{
    if (!m_composition)
        return;

    Track *track = m_composition->getTrackById(m_trackId);

    if (!track)
        return;

    track->setLabel(qstrtostr(m_newLongName));
    track->setShortLabel(qstrtostr(m_newShortName));
    m_composition->notifyTrackChanged(track);
}

void
RenameTrackCommand::unexecute()
{
    if (!m_composition)
        return;

    Track *track = m_composition->getTrackById(m_trackId);

    if (!track)
        return;

    track->setLabel(m_oldLongName.toStdString());
    track->setShortLabel(m_oldShortName.toStdString());
    m_composition->notifyTrackChanged(track);
}

}
