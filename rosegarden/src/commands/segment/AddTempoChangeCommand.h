
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_ADDTEMPOCHANGECOMMAND_H_
#define _RG_ADDTEMPOCHANGECOMMAND_H_

#include <kcommand.h>
#include <qstring.h>
#include "base/Event.h"
#include "base/Composition.h" // for tempoT
#include <klocale.h>




namespace Rosegarden
{

class AddTempoChangeCommand : public KNamedCommand
{
public:
    AddTempoChangeCommand(Composition *composition,
                          timeT time,
                          tempoT tempo,
                          tempoT target = -1):
        KNamedCommand(getGlobalName()),
        m_composition(composition),
        m_time(time),
        m_tempo(tempo),
        m_target(target),
        m_oldTempo(0),
        m_tempoChangeIndex(0) {}

    virtual ~AddTempoChangeCommand();

    static QString getGlobalName() { return i18n("Add Te&mpo Change..."); }

    virtual void execute();
    virtual void unexecute();

private:
    Composition *m_composition;
    timeT m_time;
    tempoT m_tempo;
    tempoT m_target;
    tempoT m_oldTempo;
    int m_tempoChangeIndex;
};



}

#endif
