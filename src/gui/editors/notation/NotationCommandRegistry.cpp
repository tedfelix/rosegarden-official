/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "NotationCommandRegistry.h"

#include "commands/notation/MakeChordCommand.h"
#include "commands/notation/BeamCommand.h"
#include "commands/notation/AddFingeringMarkCommand.h"
#include "commands/notation/AddSlashesCommand.h"
#include "commands/notation/AddIndicationCommand.h"
#include "commands/notation/AddMarkCommand.h"
#include "commands/notation/AddTextMarkCommand.h"
#include "commands/notation/AutoBeamCommand.h"
#include "commands/notation/BreakCommand.h"
#include "commands/notation/ChangeSlurPositionCommand.h"
#include "commands/notation/ChangeStemsCommand.h"
#include "commands/notation/ChangeStyleCommand.h"
#include "commands/notation/ChangeTiePositionCommand.h"
#include "commands/notation/CollapseRestsCommand.h"
#include "commands/notation/DeCounterpointCommand.h"
#include "commands/notation/FixNotationQuantizeCommand.h"
#include "commands/notation/IncrementDisplacementsCommand.h"
#include "commands/notation/MakeAccidentalsCautionaryCommand.h"
#include "commands/notation/MakeNotesViableCommand.h"
#include "commands/notation/RemoveFingeringMarksCommand.h"
#include "commands/notation/RemoveMarksCommand.h"
#include "commands/notation/RemoveNotationQuantizeCommand.h"
#include "commands/notation/ResetDisplacementsCommand.h"
#include "commands/notation/RespellCommand.h"
#include "commands/notation/RestoreSlursCommand.h"
#include "commands/notation/RestoreStemsCommand.h"
#include "commands/notation/RestoreTiesCommand.h"
#include "commands/notation/SetVisibilityCommand.h"
#include "commands/notation/TieNotesCommand.h"
#include "commands/notation/UnTupletCommand.h"
#include "commands/notation/UntieNotesCommand.h"


namespace Rosegarden
{

NotationCommandRegistry::NotationCommandRegistry(ActionFileClient *client) :
    ActionCommandRegistry(client)
{
    AddFingeringMarkCommand::registerCommand(this);
    AddIndicationCommand::registerCommand(this);
    AddMarkCommand::registerCommand(this);
    AddSlashesCommand::registerCommand(this);
    AddTextMarkCommand::registerCommand(this);
    AutoBeamCommand::registerCommand(this);
    BeamCommand::registerCommand(this);
    BreakCommand::registerCommand(this);
    ChangeSlurPositionCommand::registerCommand(this);
    ChangeStemsCommand::registerCommand(this);
    ChangeStyleCommand::registerCommand(this);
    ChangeTiePositionCommand::registerCommand(this);
    CollapseRestsCommand::registerCommand(this);
    DeCounterpointCommand::registerCommand(this);
    FixNotationQuantizeCommand::registerCommand(this);
    IncrementDisplacementsCommand::registerCommand(this);
    MakeAccidentalsCautionaryCommand::registerCommand(this);
    MakeChordCommand::registerCommand(this);
    MakeNotesViableCommand::registerCommand(this);
    RemoveFingeringMarksCommand::registerCommand(this);
    RemoveMarksCommand::registerCommand(this);
    RemoveNotationQuantizeCommand::registerCommand(this);
    ResetDisplacementsCommand::registerCommand(this);
    RespellCommand::registerCommand(this);
    RestoreSlursCommand::registerCommand(this);
    RestoreStemsCommand::registerCommand(this);
    RestoreTiesCommand::registerCommand(this);
    SetVisibilityCommand::registerCommand(this);
    TieNotesCommand::registerCommand(this);
    UnTupletCommand::registerCommand(this);
    UntieNotesCommand::registerCommand(this);
}

NotationCommandRegistry::~NotationCommandRegistry()
{
}


}

