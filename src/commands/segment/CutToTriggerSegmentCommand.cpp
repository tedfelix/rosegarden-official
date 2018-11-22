/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "CutToTriggerSegmentCommand.h"

#include "base/BaseProperties.h"
#include "base/Clipboard.h"
#include "base/Composition.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/SegmentNotationHelper.h"
#include "base/TriggerSegment.h"
#include "commands/edit/EraseCommand.h"
#include "document/BasicCommand.h"
#include "gui/editors/notation/NotationProperties.h"
#include "gui/editors/notation/NoteStyle.h"
#include "gui/editors/notation/NoteStyleFactory.h"
#include "misc/Strings.h"

#include <QString>

namespace Rosegarden
{

CutToTriggerSegmentCommand::
CutToTriggerSegmentCommand(EventSelection * selection,
			   Composition       &comp,
			   QString           name,
			   int               basePitch,
			   int               baseVelocity,
			   NoteStyleName     noteStyle,
                           bool              retune,
                           std::string       timeAdjust,
                           Mark              mark)
    : BasicSelectionCommand(tr("Make Ornament"), *selection, true),
      m_paster(&comp, selection, name, basePitch, baseVelocity),
      m_selection(selection),
      m_time(selection->getStartTime()),
      m_duration(selection->getTotalDuration()),
      m_noteStyle(noteStyle),
      m_retune(retune),
      m_timeAdjust(timeAdjust),
      m_mark(mark)
{}

void
CutToTriggerSegmentCommand::execute()
{
    // Create the trigger segment rec.  This has to be done first
    // because we can't know the trigger segment id until then
    // (really, until m_paster has executed at least once)
    m_paster.execute();

    // Now take advantage of BasicCommand facilities, so that we only
    // have to define modifySegment.
    BasicCommand::execute();
}  
void
CutToTriggerSegmentCommand::unexecute()
{
    // Do this in reverse order from execute, just to be safe.
    BasicCommand::unexecute();
    m_paster.unexecute();
}

// modifySegment just deals with the effects on the segment that
// selection was in, the trigger segment is managed separately by
// m_paster.
void
CutToTriggerSegmentCommand::modifySegment()
{
    using namespace BaseProperties;

    // This is only possible the first time, before selection's
    // contents evaporate due to the erasing.  This requires that we
    // use bruteForceRedo = true.
    EraseCommand::eraseInSegment(m_selection);

    /* Adapted from InsertTriggerNoteCommand */

    const TriggerSegmentId id = m_paster.getTriggerSegmentId();
    // Insert via a model event, so as to apply the note style.
    // This is a subset of the work done by NoteInsertionCommand

    Event *e = new Event(Note::EventType, m_time, m_duration);

    // Set the properties that every tied note has.
    // makeThisNoteViable will give these to every tied note.
    e->set<Int>(PITCH, m_paster.getBasePitch());
    e->set<Int>(VELOCITY, m_paster.getBaseVelocity());
    e->set<Bool>(TRIGGER_EXPAND, true);

    if (m_noteStyle != NoteStyleFactory::DefaultStyle) {
        e->set<String>(NotationProperties::NOTE_STYLE, qstrtostr(m_noteStyle));
    }

    Segment &s(getSegment());
    Segment::iterator i = s.insert(e);
    SegmentNotationHelper(s).makeThisNoteViable(i);
    s.normalizeRests(m_time, m_time + m_duration);

    // Now set the properties that only the trigger note has.
    e->set<Int>(TRIGGER_SEGMENT_ID, id);
    e->set<Bool>(TRIGGER_SEGMENT_RETUNE, m_retune);
    e->set<String>(TRIGGER_SEGMENT_ADJUST_TIMES, m_timeAdjust);

    if (m_mark != Marks::NoMark) {
        Marks::addMark(*e, m_mark, true);
    }


    // Update references to this new ornament
    TriggerSegmentRec *rec =
        s.getComposition()->getTriggerSegmentRec(id);

    if (rec)
        { rec->updateReferences(); }
}
    
}
