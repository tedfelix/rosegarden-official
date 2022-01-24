
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_ADJUSTMENUCHANGETIEPOSITIONCOMMAND_H
#define RG_ADJUSTMENUCHANGETIEPOSITIONCOMMAND_H

#include "document/BasicCommand.h"

#include <QCoreApplication>
#include <QString>


namespace Rosegarden
{


class EventSelection;
class CommandRegistry;


class ChangeTiePositionCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::ChangeTiePositionCommand)

public:
    ChangeTiePositionCommand(bool above, EventSelection &selection) :
        BasicCommand(getGlobalName(above), selection, true),
        m_selection(&selection),
        m_above(above)
    { }

    static bool getArgument(QString actionName, CommandArgumentQuerier &);
    static void registerCommand(CommandRegistry *r);

protected:
    void modifySegment() override;

private:
    static QString getGlobalName(bool above)
    {
        return above ? tr("Tie &Above") : tr("Tie &Below");
    }

    // only used on 1st execute (cf bruteForceRedo)
    EventSelection *m_selection;

    bool m_above;
};


}

#endif
