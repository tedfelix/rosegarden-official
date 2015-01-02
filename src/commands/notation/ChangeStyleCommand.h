
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

#ifndef RG_CHANGESTYLECOMMAND_H
#define RG_CHANGESTYLECOMMAND_H

#include "document/BasicSelectionCommand.h"
#include "gui/editors/notation/NoteStyle.h"
#include <QString>
#include <QCoreApplication>


class Change;


namespace Rosegarden
{

class EventSelection;
class CommandRegistry;


class ChangeStyleCommand : public BasicSelectionCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::ChangeStyleCommand)

public:
    ChangeStyleCommand(NoteStyleName style,
                       EventSelection &selection) :
        BasicSelectionCommand(getGlobalName(style), selection, true),
        m_selection(&selection), m_style(style) { }

    static QString getGlobalName() {
        return tr("Change &Note Style");
    }

    static QString getGlobalName(NoteStyleName style);

    static NoteStyleName getArgument(QString actionName, CommandArgumentQuerier &);
    static void registerCommand(CommandRegistry *r);

protected:
    virtual void modifySegment();

private:
    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    NoteStyleName m_style;
};



}

#endif
