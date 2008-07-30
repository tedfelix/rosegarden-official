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


#include "NotationSelectionPaster.h"

#include <klocale.h>
#include "base/Event.h"
#include "base/Selection.h"
#include "base/ViewElement.h"
#include "commands/edit/PasteEventsCommand.h"
#include "gui/general/EditTool.h"
#include "gui/general/LinedStaff.h"
#include "document/RosegardenGUIDoc.h"
#include "NotationTool.h"
#include "NotationView.h"
#include "NotationElement.h"


namespace Rosegarden
{

NotationSelectionPaster::NotationSelectionPaster(EventSelection& es,
        NotationView* view)
        : NotationTool("NotationPaster", view),
        m_selection(es)
{
    m_nParentView->setCanvasCursor(Qt::crossCursor);
}

NotationSelectionPaster::~NotationSelectionPaster()
{}

void NotationSelectionPaster::handleLeftButtonPress(timeT,
        int,
        int staffNo,
        QMouseEvent* e,
        ViewElement*)
{
    if (staffNo < 0)
        return ;
    Event *clef = 0, *key = 0;

    LinedStaff *staff = m_nParentView->getLinedStaff(staffNo);

    NotationElementList::iterator closestElement =
        staff->getClosestElementToCanvasCoords(e->x(), (int)e->y(),
                                               clef, key, false, -1);

    if (closestElement == staff->getViewElementList()->end())
        return ;

    timeT time = (*closestElement)->getViewAbsoluteTime();

    Segment& segment = staff->getSegment();
    PasteEventsCommand *command = new PasteEventsCommand
                                  (segment, m_parentView->getDocument()->getClipboard(), time,
                                   PasteEventsCommand::Restricted);

    if (!command->isPossible()) {
        m_parentView->slotStatusHelpMsg(i18n("Couldn't paste at this point"));
    } else {
        m_parentView->addCommandToHistory(command);
        m_parentView->slotStatusHelpMsg(i18n("Ready."));
    }
}

}
