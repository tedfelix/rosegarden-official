
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

#ifndef RG_ADDMARKCOMMAND_H
#define RG_ADDMARKCOMMAND_H

#include "document/BasicSelectionCommand.h"
#include <QString>

#include <QCoreApplication>


namespace Rosegarden
{

class EventSelection;
class CommandRegistry;

class AddMarkCommand : public BasicSelectionCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::AddMarkCommand)

public:
    AddMarkCommand(Mark mark,
                   EventSelection &selection) :
        BasicSelectionCommand(getGlobalName(mark), selection, true),
        m_selection(&selection), m_mark(mark) { }

    static QString getGlobalName(Mark mark);
    static QString getActionName(Mark mark);
    static QString getShortcut(Mark mark);
    static QString getIconName(Mark mark);

    static Mark getArgument(QString actionName, CommandArgumentQuerier &);
    static void registerCommand(CommandRegistry *r);

protected:
    virtual void modifySegment();

private:
    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    Mark m_mark;
};



}

#endif
