
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

#ifndef _RG_ADDTRACKSCOMMAND_H_
#define _RG_ADDTRACKSCOMMAND_H_

#include "base/MidiProgram.h"
#include "base/Composition.h"
#include <kcommand.h>
#include <qstring.h>
#include <vector>
#include <map>
#include <klocale.h>



namespace Rosegarden
{

class Track;
class Composition;


class AddTracksCommand : public KNamedCommand
{
public:
    AddTracksCommand(Composition *composition,
                     unsigned int nbTracks,
                     InstrumentId id,
                     int position); // -1 -> at end
    virtual ~AddTracksCommand();

    static QString getGlobalName() { return i18n("Add Tracks..."); }

    virtual void execute();
    virtual void unexecute();

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
