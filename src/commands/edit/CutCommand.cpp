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

#define RG_MODULE_STRING "[CutCommand]"

#include "CutCommand.h"

#include "base/Clipboard.h"
#include "base/Selection.h"
#include "commands/segment/SegmentEraseCommand.h"
#include "CopyCommand.h"
#include "EraseCommand.h"
#include "misc/Debug.h"

#include <QString>


namespace Rosegarden
{

CutCommand::CutCommand(EventSelection &selection,
                       Clipboard *clipboard) :
        MacroCommand(getGlobalName())
{
    addCommand(new CopyCommand(selection, clipboard));
    addCommand(new EraseCommand(&selection));
}

CutCommand::CutCommand(
        EventSelection *selection1,
        EventSelection *selection2,
        Clipboard *clipboard) :
    MacroCommand(getGlobalName())
{
    // Set any empties to nullptr to avoid doing extra work.
    if (selection1  &&  selection1->getSegmentEvents().empty())
        selection1 = nullptr;
    if (selection2  &&  selection2->getSegmentEvents().empty())
        selection2 = nullptr;

    // Nothing to do?  Bail.
    if (!selection1  &&  !selection2)
        return;

    addCommand(new CopyCommand(selection1, selection2, clipboard));
    if (selection1)
        addCommand(new EraseCommand(selection1));
    if (selection2)
        addCommand(new EraseCommand(selection2));
}

CutCommand::CutCommand(SegmentSelection &selection,
                       Clipboard *clipboard) :
        MacroCommand(getGlobalName())
{
    addCommand(new CopyCommand(selection, clipboard));

    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {
        addCommand(new SegmentEraseCommand(*i));
    }
}

}
