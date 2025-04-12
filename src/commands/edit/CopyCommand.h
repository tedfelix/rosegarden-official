
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_COPYCOMMAND_H
#define RG_COPYCOMMAND_H

#include "document/Command.h"
#include <QString>
#include "base/Event.h"
#include "base/Selection.h"
#include <QCoreApplication>




namespace Rosegarden
{


class EventSelection;
class Composition;
class Clipboard;


/// Copy a selection

class CopyCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::CopyCommand)

public:
    /// Make a CopyCommand that copies events from within a Segment
    /**
     * ??? selection should be a const &.  It is copied.  Ownership is
     *     not transferred.
     */
    CopyCommand(EventSelection *selection,
                Clipboard *clipboard);

    CopyCommand(EventSelection *selection1,
                EventSelection *selection2,
                Clipboard *clipboard);

    /// Make a CopyCommand that copies whole Segments
    CopyCommand(SegmentSelection &selection,
                Clipboard *clipboard);

    /// Make a CopyCommand that copies a range of a Composition
    CopyCommand(Composition *composition,
                timeT beginTime,
                timeT endTime,
                Clipboard *clipboard);

    ~CopyCommand() override;

    static QString getGlobalName() { return tr("&Copy"); }

    void execute() override;
    void unexecute() override;

protected:
    Clipboard *m_sourceClipboard;
    Clipboard *m_targetClipboard;
    Clipboard *m_savedClipboard;
};



}

#endif
