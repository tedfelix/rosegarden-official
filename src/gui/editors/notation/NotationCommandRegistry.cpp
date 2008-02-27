/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "NotationCommandRegistry.h"

#include "gui/general/EditView.h"

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
#include "commands/notation/RestoreTiesCommand.h"

#include "NoteFontFactory.h"
#include "NoteFont.h"
#include "NoteCharacter.h"
#include "NoteStyleFactory.h"

#include <qiconset.h>


namespace Rosegarden
{

NotationCommandRegistry::NotationCommandRegistry(EditView *v) :
    EditViewCommandRegistry(v)
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
    RestoreTiesCommand::registerCommand(this);
}

NotationCommandRegistry::~NotationCommandRegistry()
{
}

bool
NotationCommandRegistry::findIcon(QString iconName, QIconSet &icon)
{
    NoteFont *font = 0;
    try {
        font = NoteFontFactory::getFont
            (NoteFontFactory::getDefaultFontName(), 6);
    } catch (Exception) {
        font = NoteFontFactory::getFont
            (NoteFontFactory::getDefaultFontName(),
             NoteFontFactory::getDefaultSize(NoteFontFactory::getDefaultFontName()));
    }

    if (!font) return false;

    NoteCharacter character;
    bool found = font->getCharacter
        (NoteStyleFactory::getStyle(NoteStyleFactory::DefaultStyle)->
         getSomeCharName(iconName),
         character);

    if (found) {
        QPixmap *pixmap = character.getPixmap();
        if (pixmap) icon = QIconSet(*pixmap);
        else found = false;
    }

    return found;
}

}

