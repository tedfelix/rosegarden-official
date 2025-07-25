/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_ADDTEMPOCHANGECOMMAND_H
#define RG_ADDTEMPOCHANGECOMMAND_H

#include "document/Command.h"  // for NamedCommand
#include "base/Composition.h"  // for tempoT

#include <QCoreApplication>
#include <QString>


namespace Rosegarden
{


class AddTempoChangeCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::AddTempoChangeCommand)

public:
    AddTempoChangeCommand(Composition *composition,
                          timeT time,
                          tempoT tempo,
                          tempoT target = -1):
        NamedCommand(getGlobalName()),
        m_composition(composition),
        m_time(time),
        m_tempo(tempo),
        m_target(target)
    { }

    static QString getGlobalName()  { return tr("Add Te&mpo Change..."); }

    void execute() override;
    void unexecute() override;

private:
    Composition *m_composition;
    timeT m_time;
    tempoT m_tempo;
    tempoT m_target;

    tempoT m_oldTempo{0};
    tempoT m_oldTarget{-1};
    int m_tempoChangeIndex{0};
};


}

#endif
