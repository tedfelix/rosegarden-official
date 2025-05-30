
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

#ifndef RG_CHANGESLURPOSITIONCOMMAND_H
#define RG_CHANGESLURPOSITIONCOMMAND_H

#include "document/BasicCommand.h"

#include <QCoreApplication>
#include <QString>


namespace Rosegarden
{


class EventSelection;
class CommandRegistry;


class ChangeSlurPositionCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::ChangeSlurPositionCommand)

public:
    ChangeSlurPositionCommand(bool above, EventSelection &selection) :
        BasicCommand(getGlobalName(above), selection, true),
        m_selection(&selection),
        m_above(above)
    { }

    static QString getGlobalName(bool above)
    {
        return above ? tr("Slur &Above") : tr("Slur &Below");
    }

    static bool getArgument(const QString& actionName, CommandArgumentQuerier &);
    static void registerCommand(CommandRegistry *r);

protected:
    void modifySegment() override;

private:
    // only used on 1st execute (cf bruteForceRedo)
    EventSelection *m_selection;

    bool m_above;
};


}

#endif
