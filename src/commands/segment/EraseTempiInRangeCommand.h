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
 * Erase tempos in a given range
 */
class EraseTempiInRangeCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::EraseTempiInRangeCommand)

public:
    EraseTempiInRangeCommand(Composition *composition,
                            timeT rangeBegin,
                            timeT rangeEnd);
    virtual ~EraseTempiInRangeCommand();

    virtual void execute();
    virtual void unexecute();

private:
    Composition *m_composition;
    TempoSelection m_temposPre;
};



}

#endif
