
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MOVETRACKSCOMMAND_H
#define RG_MOVETRACKSCOMMAND_H

#include "base/Track.h"
#include "document/Command.h"
#include <QString>
#include <QCoreApplication>




namespace Rosegarden
{

class Composition;


class MoveTracksCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::MoveTracksCommand)

public:
    MoveTracksCommand(Composition *composition,
                      TrackId srcTrack,
                      TrackId destTrack);
    virtual ~MoveTracksCommand();

    static QString getGlobalName() { return tr("Move Tracks..."); }

    virtual void execute();
    virtual void unexecute();

protected:
    Composition           *m_composition;

    TrackId                m_srcTrack;
    TrackId                m_destTrack;
};


}

#endif
