
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_RENAMETRACKCOMMAND_H_
#define _RG_RENAMETRACKCOMMAND_H_

#include "base/Track.h"
#include <string>
#include "document/Command.h"
#include <QString>
#include <QCoreApplication>




namespace Rosegarden
{

class Composition;


class RenameTrackCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::RenameTrackCommand)

public:
    RenameTrackCommand(Composition *composition,
                       TrackId track, 
                       std::string name);
    virtual ~RenameTrackCommand();

    static QString getGlobalName() { return tr("Rename Track"); }

    virtual void execute();
    virtual void unexecute();

protected:
    Composition *m_composition;
    TrackId      m_trackId;
    std::string              m_oldName;
    std::string              m_newName;
};


}

#endif
