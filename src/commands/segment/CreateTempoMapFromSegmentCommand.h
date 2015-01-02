
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

#ifndef RG_CREATETEMPOMAPFROMSEGMENTCOMMAND_H
#define RG_CREATETEMPOMAPFROMSEGMENTCOMMAND_H

#include <map>
#include "document/Command.h"
#include "base/Event.h"
#include "base/Composition.h" // for tempoT

#include <QCoreApplication>


namespace Rosegarden
{


/**
 * CreateTempoMapFromSegment applies timings found in a reference
 * segment to the composition as a whole via the tempo map.
 */

class CreateTempoMapFromSegmentCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::CreateTempoMapFromSegmentCommand)

public:
    CreateTempoMapFromSegmentCommand(Segment *grooveSegment);
    virtual ~CreateTempoMapFromSegmentCommand();

    virtual void execute();
    virtual void unexecute();

private:
    void initialise(Segment *s);
    
    Composition *m_composition;

    typedef std::map<timeT, tempoT> TempoMap;
    TempoMap m_oldTempi;
    TempoMap m_newTempi;
};
    


}

#endif
