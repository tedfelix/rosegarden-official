
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_COPYCOMMAND_H_
#define _RG_COPYCOMMAND_H_

#include <kcommand.h>
#include <qstring.h>
#include "base/Event.h"
#include <klocale.h>




namespace Rosegarden
{

class SegmentSelection;
class EventSelection;
class Composition;
class Clipboard;


/// Copy a selection

class CopyCommand : public KNamedCommand
{
public:
    /// Make a CopyCommand that copies events from within a Segment
    CopyCommand(EventSelection &selection,
                Clipboard *clipboard);

    /// Make a CopyCommand that copies whole Segments
    CopyCommand(SegmentSelection &selection,
                Clipboard *clipboard);

    /// Make a CopyCommand that copies a range of a Composition
    CopyCommand(Composition *composition,
                timeT beginTime,
                timeT endTime,
                Clipboard *clipboard);

    virtual ~CopyCommand();

    static QString getGlobalName() { return i18n("&Copy"); }

    virtual void execute();
    virtual void unexecute();

protected:
    Clipboard *m_sourceClipboard;
    Clipboard *m_targetClipboard;
    Clipboard *m_savedClipboard;
};



}

#endif
