
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

#ifndef RG_SEGMENTRECORDCOMMAND_H
#define RG_SEGMENTRECORDCOMMAND_H

#include "document/Command.h"

#include <QCoreApplication>


namespace Rosegarden
{

class Segment;
class Composition;


/**
 * SegmentRecordCommand pretends to insert a Segment that is actually
 * already in the Composition (the currently-being-recorded one).  It's
 * used at the end of recording, to ensure that GUI updates happen
 * correctly, and it provides the ability to undo recording.  (The
 * unexecute does remove the segment, it doesn't just pretend to.)
 */
class SegmentRecordCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SegmentRecordCommand)

public:
    SegmentRecordCommand(Segment *segment);
    virtual ~SegmentRecordCommand();

    virtual void execute();
    virtual void unexecute();

private:
    Composition *m_composition;
    Segment *m_segment;
    bool m_detached;
};



}

#endif
