
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

#ifndef RG_CUTCOMMAND_H
#define RG_CUTCOMMAND_H

#include <QString>
#include <QCoreApplication>
#include "document/Command.h"
#include "base/Selection.h"


namespace Rosegarden
{


class EventSelection;
class Clipboard;


/// Cut a selection

class CutCommand : public MacroCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::CutCommand)

public:
    /// Make a CutCommand that cuts events from within a Segment
    CutCommand(EventSelection &selection,
               Clipboard *clipboard);

    /// Make a CutCommand that cuts whole Segments
    CutCommand(SegmentSelection &selection,
               Clipboard *clipboard);

    static QString getGlobalName() { return tr("Cu&t"); }
};



}

#endif
