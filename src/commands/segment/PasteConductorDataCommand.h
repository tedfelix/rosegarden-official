
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_PASTECONDUCTORDATACOMMAND_H
#define RG_PASTECONDUCTORDATACOMMAND_H

#include "document/Command.h"
#include "base/Event.h"
#include "base/Selection.h"

#include <QCoreApplication>


namespace Rosegarden
{

class Composition;
class Clipboard;


/**
 * Paste time signature and tempo data from the given clipboard into
 * the given composition starting at the given time.
 */
class PasteConductorDataCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::PasteConductorDataCommand)

public:
    PasteConductorDataCommand(Composition *composition,
                              Clipboard *clipboard,
                              timeT t);
    ~PasteConductorDataCommand() override;

    void execute() override;
    void unexecute() override;

private:
    Composition *m_composition;
    Clipboard *m_clipboard;
    timeT m_t0;
    TimeSignatureSelection m_timesigsPre;
    TempoSelection m_temposPre;
};



}

#endif
