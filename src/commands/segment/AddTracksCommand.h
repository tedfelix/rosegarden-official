
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

#ifndef RG_ADDTRACKSCOMMAND_H
#define RG_ADDTRACKSCOMMAND_H

#include "base/MidiProgram.h"
#include "base/Composition.h"
#include "document/Command.h"
#include <QString>
#include <vector>
#include <map>
#include <QCoreApplication>



namespace Rosegarden
{

class Track;
class Composition;


class AddTracksCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::AddTracksCommand)

public:
    AddTracksCommand(Composition *composition,
                     unsigned int nbTracks,
                     InstrumentId id,
                     int position); // -1 -> at end
    virtual ~AddTracksCommand();

    static QString getGlobalName() { return tr("Add Tracks..."); }

    void execute() override;
    void unexecute() override;

protected:
    Composition           *m_composition;
    unsigned int           m_nbNewTracks;
    InstrumentId           m_instrumentId;
    int                    m_position;

    typedef std::map<TrackId, int> TrackPositionMap;

    std::vector<Track *>   m_newTracks;
    TrackPositionMap       m_oldPositions;

    bool                   m_detached;
};


}

#endif
