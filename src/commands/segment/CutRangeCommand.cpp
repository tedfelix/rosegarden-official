/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "CutRangeCommand.h"

#include "base/Clipboard.h"
#include "base/Composition.h"
#include "commands/edit/CopyCommand.h"
#include "DeleteRangeCommand.h"


namespace Rosegarden
{

CutRangeCommand::CutRangeCommand(Composition *composition,
                                 timeT t0, timeT t1,
                                 Clipboard *clipboard) :
        MacroCommand(tr("Cut Range"))
{
    addCommand(new CopyCommand(composition, t0, t1, clipboard));
    addCommand(new DeleteRangeCommand(composition, t0, t1));
}

}
