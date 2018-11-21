
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

#ifndef RG_PASTEEVENTSCOMMAND_H
#define RG_PASTEEVENTSCOMMAND_H

#include "document/BasicCommand.h"
#include "base/Selection.h"
#include <map>
#include <QString>
#include "base/Event.h"
#include <QCoreApplication>




namespace Rosegarden
{

class Segment;
class Clipboard;


/// Paste from a single-segment clipboard to a segment

class PasteEventsCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::PasteEventsCommand)

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
     * in a MacroCommand sequence.  pasteEndTime must supply the
     * latest time in the destination segment that may be modified
     * by the paste.
     */
    PasteEventsCommand(Segment &segment,
                       Clipboard *clipboard,
                       timeT pasteTime,
                       timeT pasteEndTime,
                       PasteType pasteType);

    ~PasteEventsCommand() override;

    EventSelection getPastedEvents();

    static QString getGlobalName() { return tr("&Paste"); }

    /// Determine whether this paste will succeed (without executing it yet)
    bool isPossible();

    timeT getRelayoutEndTime() override;

protected:
    void modifySegment() override;
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
