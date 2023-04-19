
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2023 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_ERASECOMMAND_H
#define RG_ERASECOMMAND_H

#include "document/BasicCommand.h"
#include "base/TimeT.h"

#include <QCoreApplication>


namespace Rosegarden
{


class EventSelection;


/// Erase a selection from within a segment
class EraseCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::EraseCommand)

public:
    /// Allow for multiple selections.  E.g. matrix and CC ruler.
    EraseCommand(EventSelection *selection1,
                 EventSelection *selection2 = nullptr);
    ~EraseCommand() override;

    /// Erase the events in segment that are in selection.
    /**
     * Return whether any deletions that affect later in the segment
     * were done, meaning key or clef deletions.
     */
    static bool eraseInSegment(EventSelection *selection);
    
    timeT getRelayoutEndTime() override;

protected:
    void modifySegment() override;

private:
    /**
     * Only used on first execute (cf BasicCommand::bruteForceRedo).
     *
     * QSharedPointer would be nice.
     */
    EventSelection *m_selection1;
    EventSelection *m_selection2;

    timeT m_relayoutEndTime;
};


}

#endif
