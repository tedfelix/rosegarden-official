
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

#ifndef RG_REMOVETEMPOCHANGECOMMAND_H
#define RG_REMOVETEMPOCHANGECOMMAND_H

#include "document/Command.h"
#include <QString>
#include "base/Event.h"
#include "base/Composition.h" // for tempoT
#include <QCoreApplication>


class Remove;


namespace Rosegarden
{

class Composition;


class RemoveTempoChangeCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::RemoveTempoChangeCommand)

public:
    RemoveTempoChangeCommand(Composition *composition,
                             int index):
        NamedCommand(getGlobalName()),
        m_composition(composition),
        m_tempoChangeIndex(index),
        m_oldTime(0),
        m_oldTempo(0){}

    virtual ~RemoveTempoChangeCommand() {}

    static QString getGlobalName() { return tr("Remove &Tempo Change..."); }

    virtual void execute();
    virtual void unexecute();

private:
    Composition *m_composition;
    int                      m_tempoChangeIndex;
    timeT        m_oldTime;
    tempoT       m_oldTempo;
};




}

#endif
