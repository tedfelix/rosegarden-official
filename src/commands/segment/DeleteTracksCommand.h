
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

#ifndef RG_DELETETRACKSCOMMAND_H
#define RG_DELETETRACKSCOMMAND_H

#include "document/Command.h"
#include <QString>
#include <vector>
#include <QCoreApplication>
#include "base/Track.h"


namespace Rosegarden
{

class Track;
class Segment;
class Composition;


class DeleteTracksCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::DeleteTracksCommand)

public:
    DeleteTracksCommand(Composition *composition,
                        std::vector<TrackId> tracks);
    virtual ~DeleteTracksCommand();

    static QString getGlobalName() { return tr("Delete Tracks..."); }

    void execute() override;
    void unexecute() override;

protected:
    Composition           *m_composition;
    std::vector<TrackId>   m_tracks;

    std::vector<Track*>    m_oldTracks;
    std::vector<Segment*>  m_oldSegments;
    bool                               m_detached;
};


}

#endif
