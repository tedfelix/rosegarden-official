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

#ifndef RG_ADDFINGERINGMARKCOMMAND_H
#define RG_ADDFINGERINGMARKCOMMAND_H

#include "document/BasicSelectionCommand.h"
#include <string>
#include <vector>
#include <QString>

#include <QCoreApplication>


namespace Rosegarden
{

class EventSelection;
class CommandRegistry;

class AddFingeringMarkCommand : public BasicSelectionCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::AddFingeringMarkCommand)

public:
    AddFingeringMarkCommand(std::string fingering,
                            EventSelection &selection) :
        BasicSelectionCommand(getGlobalName(fingering), selection, true),
        m_selection(&selection), m_fingering(fingering) { }

    static QString getGlobalName(std::string fingering = "");
    static QString getActionName(std::string fingering = "");

    static std::string getArgument(QString actionName, CommandArgumentQuerier &);

    static std::vector<std::string> getStandardFingerings();

    static void registerCommand(CommandRegistry *r);

protected:
    virtual void modifySegment();

private:
    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    std::string m_fingering;
};



}

#endif
