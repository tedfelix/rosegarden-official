
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_CHANGECOMPOSITIONLENGTHCOMMAND_H_
#define _RG_CHANGECOMPOSITIONLENGTHCOMMAND_H_

#include "document/Command.h"
#include <QString>
#include "base/Event.h"
#include <QCoreApplication>


class Change;


namespace Rosegarden
{

class Composition;


class ChangeCompositionLengthCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::ChangeCompositionLengthCommand)

public:
    ChangeCompositionLengthCommand(Composition *composition,
                                   timeT startTime,
                                   timeT endTime);
    virtual ~ChangeCompositionLengthCommand();

    static QString getGlobalName()
        { return tr("Change &Composition Start and End..."); }

    virtual void execute();
    virtual void unexecute();

protected:
    Composition *m_composition;
    timeT        m_startTime;
    timeT        m_endTime;
    timeT        m_oldStartTime;
    timeT        m_oldEndTime;

};


}

#endif
