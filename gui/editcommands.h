/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef EDIT_COMMANDS_H
#define EDIT_COMMANDS_H


#include "basiccommand.h"

namespace Rosegarden {
    class Clipboard;
}


/// Cut a selection from within a segment

class CutCommand : public CompoundCommand
{
public:
    CutCommand(Rosegarden::EventSelection &selection,
	       Rosegarden::Clipboard *clipboard);

    static QString name() { return "Cu&t"; }
};


/// Copy a selection from within a segment

class CopyCommand : public Command // no refresh needed
{
public:
    CopyCommand(Rosegarden::EventSelection &selection,
		Rosegarden::Clipboard *clipboard);
    virtual ~CopyCommand();

    static QString name() { return "&Copy"; }

    virtual void execute();
    virtual void unexecute();

protected:
    Rosegarden::Clipboard *m_sourceClipboard;
    Rosegarden::Clipboard *m_targetClipboard;
};


/// Paste from a single-segment clipboard to a segment

class PasteCommand : public BasicCommand
{
public:
    enum PasteType {
	Restricted,		// paste into existing gap
	Simple,			// erase existing events to make room
	OpenAndPaste,		// bump up existing events to make room
	NoteOverlay,		// overlay and tie notation-style
	MatrixOverlay		// overlay raw matrix-style
    };

    PasteCommand(Rosegarden::Segment &segment,
		 Rosegarden::Clipboard *clipboard,
		 Rosegarden::timeT pasteTime,
		 PasteType pasteType = getDefaultPasteType());

    static QString name() { return "&Paste"; }

    /// Determine whether this paste will succeed (without executing it yet)
    bool isPossible();

    virtual Rosegarden::timeT getRelayoutEndTime();
    
    static void setDefaultPasteType(PasteType type) { m_defaultPaste = type; }
    static PasteType getDefaultPasteType() { return m_defaultPaste; }

protected:
    virtual void modifySegment();
    Rosegarden::timeT getEffectiveEndTime(Rosegarden::Segment &,
					  Rosegarden::Clipboard *,
					  Rosegarden::timeT);
    Rosegarden::timeT m_relayoutEndTime;
    Rosegarden::Clipboard *m_clipboard;
    PasteType m_pasteType;

    static PasteType m_defaultPaste;
};


/// Erase a selection from within a segment

class EraseCommand : public BasicSelectionCommand
{
public:
    EraseCommand(Rosegarden::EventSelection &selection);

    static QString name() { return "&Erase"; }

    virtual Rosegarden::timeT getRelayoutEndTime();

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    Rosegarden::timeT m_relayoutEndTime;
};


#endif
