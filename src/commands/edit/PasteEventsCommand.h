
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

#ifndef _RG_PASTEEVENTSCOMMAND_H_
#define _RG_PASTEEVENTSCOMMAND_H_

#include "document/BasicCommand.h"
#include "base/Selection.h"
#include <map>
#include <qstring.h>
#include "base/Event.h"
#include <klocale.h>




namespace Rosegarden
{

class Segment;
class Clipboard;


/// Paste from a single-segment clipboard to a segment

class PasteEventsCommand : public BasicCommand
{
public:
    enum PasteType {
        Restricted,             // paste into existing gap
        Simple,                 // erase existing events to make room
        OpenAndPaste,           // bump up existing events to make room
        NoteOverlay,            // overlay and tie notation-style
        MatrixOverlay           // overlay raw matrix-style
    };

    typedef std::map<PasteType, QString> PasteTypeMap;
    static PasteTypeMap getPasteTypes(); // type, descrip

    /**
     * Construct a Paste command from a clipboard that already contains
     * the events to be pasted.
     */
    PasteEventsCommand(Segment &segment,
                       Clipboard *clipboard,
                       timeT pasteTime,
                       PasteType pasteType);

    /**
     * Construct a Paste command from a clipboard that will contain
     * the events to be pasted by the time the Paste command is
     * executed, but might not do so yet.  This is necessary if the
     * Paste command is to follow another clipboard-based command
     * in a KMacroCommand sequence.  pasteEndTime must supply the
     * latest time in the destination segment that may be modified
     * by the paste.
     */
    PasteEventsCommand(Segment &segment,
                       Clipboard *clipboard,
                       timeT pasteTime,
                       timeT pasteEndTime,
                       PasteType pasteType);

    virtual ~PasteEventsCommand();

    EventSelection getPastedEvents();

    static QString getGlobalName() { return i18n("&Paste"); }

    /// Determine whether this paste will succeed (without executing it yet)
    bool isPossible();

    virtual timeT getRelayoutEndTime();

protected:
    virtual void modifySegment();
    timeT getEffectiveEndTime(Segment &,
                                          Clipboard *,
                                          timeT);
    timeT m_relayoutEndTime;
    Clipboard *m_clipboard;
    PasteType m_pasteType;
    EventSelection m_pastedEvents;
};



}

#endif
