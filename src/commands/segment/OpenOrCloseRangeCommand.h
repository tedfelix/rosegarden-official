
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

#ifndef RG_OPENORCLOSERANGECOMMAND_H
#define RG_OPENORCLOSERANGECOMMAND_H

#include "base/Selection.h"
#include "document/Command.h"
#include <vector>
#include "base/Event.h"

#include <QCoreApplication>


namespace Rosegarden
{

class Segment;
class Composition;

/**
 * Pull all segments, time sigs, tempos etc starting after the end of
 * a given range back by the duration of that range, so as to fill in
 * the (presumably empty) range itself.
 *
 * This does not actually split any segments etc, it just moves them.
 */
class OpenOrCloseRangeCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::OpenOrCloseRangeCommand)

public:
    OpenOrCloseRangeCommand(Composition *composition,
                            timeT rangeBegin,
                            timeT rangeEnd,
                            bool open);
    virtual ~OpenOrCloseRangeCommand();

    virtual void execute();
    virtual void unexecute();

private:
    Composition *m_composition;
    timeT m_beginTime;
    timeT m_endTime;

    bool m_prepared;
    bool m_hasExecuted;
    bool m_opening;

    std::vector<Segment *> m_moving;

    TimeSignatureSelection m_timesigsPre;
    TimeSignatureSelection m_timesigsPost;

    TempoSelection m_temposPre;
    TempoSelection m_temposPost;

    // I own the markers that aren't currently in the composition.
    MarkerSelection m_markersPre;
    MarkerSelection m_markersPost;

    timeT m_loopBegin;
    timeT m_loopEnd;
};



}

#endif
