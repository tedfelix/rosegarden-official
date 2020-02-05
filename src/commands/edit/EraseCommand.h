
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

#ifndef RG_ERASECOMMAND_H
#define RG_ERASECOMMAND_H

#include "document/BasicSelectionCommand.h"
#include "base/Event.h"

#include <QCoreApplication>
#include <QString>


namespace Rosegarden
{


class EventSelection;


/// Erase a selection from within a segment
class EraseCommand : public BasicSelectionCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::EraseCommand)

public:
    EraseCommand(EventSelection &selection);
    ~EraseCommand() override;

    static QString getGlobalName() { return tr("&Erase"); }

    // Return whether any deletions that affect later in the segment
    // were done, meaning key or clef deletions.
    static bool eraseInSegment(EventSelection *selection);
    
    timeT getRelayoutEndTime() override;

protected:
    void modifySegment() override;

private:
    /**
     * Only used on first execute (cf BasicSelectionCommand::bruteForceRedo).
     *
     * QSharedPointer would be nice.
     */
    EventSelection *m_selection;

    timeT m_relayoutEndTime;
};


}

#endif
