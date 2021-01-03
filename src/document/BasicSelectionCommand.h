
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

#ifndef RG_BASICSELECTIONCOMMAND_H
#define RG_BASICSELECTIONCOMMAND_H

#include "BasicCommand.h"


class QString;


namespace Rosegarden
{

class Segment;
class EventSelection;


/**
 * Subclass of BasicCommand that manages the brute-force undo and redo
 * extends based on a given selection.
 */

class BasicSelectionCommand : public BasicCommand
{
public:
    ~BasicSelectionCommand() override;

protected:
    /// selection from segment
    BasicSelectionCommand(const QString &name,
                          EventSelection &selection,
                          bool bruteForceRedoRequired = false);

    /// entire segment
    BasicSelectionCommand(const QString &name,
                          Segment &segment,
                          bool bruteForceRedoRequired = false);
};


}

#endif
