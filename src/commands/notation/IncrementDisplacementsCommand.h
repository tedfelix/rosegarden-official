
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2023 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_INCREMENTDISPLACEMENTSCOMMAND_H
#define RG_INCREMENTDISPLACEMENTSCOMMAND_H

#include "document/BasicCommand.h"

#include <QCoreApplication>
#include <QPoint>
#include <QString>


namespace Rosegarden
{


class EventSelection;
class CommandRegistry;


class IncrementDisplacementsCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::IncrementDisplacementsCommand)

public:
    IncrementDisplacementsCommand(QPoint relative,
                                  EventSelection &selection) :
        BasicCommand(getGlobalName(), selection, true),
        m_selection(&selection),
        m_dx(relative.x()),
        m_dy(relative.y())
    { }

    IncrementDisplacementsCommand(EventSelection &selection,
                                  long dx, long dy) :
        BasicCommand(getGlobalName(), selection, true),
        m_selection(&selection),
        m_dx(dx),
        m_dy(dy)
    { }

    static void registerCommand(CommandRegistry *r);
    static QPoint getArgument(const QString& actionName, CommandArgumentQuerier &);

protected:
    void modifySegment() override;

private:
    static QString getGlobalName() { return tr("Fine Reposition"); }

    // only used on 1st execute (cf bruteForceRedo)
    EventSelection *m_selection;
    long m_dx;
    long m_dy;
};


}

#endif
