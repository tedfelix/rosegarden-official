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

#ifndef RG_SEGMENTINSERTCOMMAND_H
#define RG_SEGMENTINSERTCOMMAND_H

#include "base/Track.h"
#include "document/Command.h"
#include "base/Event.h"

#include <QCoreApplication>


namespace Rosegarden
{

class Studio;
class Segment;
class RosegardenDocument;
class Composition;


class SegmentInsertCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SegmentInsertCommand)

public:
    SegmentInsertCommand(RosegardenDocument *doc,
                         TrackId track,
                         timeT startTime,
                         timeT endTime);
    SegmentInsertCommand(Composition *composition,
                         Segment *segment,
                         TrackId track);
    virtual ~SegmentInsertCommand();

    Segment *getSegment() const; // after invocation

    virtual void execute();
    virtual void unexecute();
    
private:
    Composition *m_composition;
    Studio      *m_studio;
    Segment     *m_segment;
    int                      m_track;
    timeT        m_startTime;
    timeT        m_endTime;
    bool m_detached;
};



}

#endif
