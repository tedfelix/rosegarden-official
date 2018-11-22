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

#ifndef RG_CUTTOTRIGGERSEGMENTCOMMAND_H
#define RG_CUTTOTRIGGERSEGMENTCOMMAND_H

#include "commands/segment/PasteToTriggerSegmentCommand.h"
#include "document/BasicSelectionCommand.h"
#include "gui/editors/notation/NoteStyle.h"

#include <string>

class QString;

namespace Rosegarden
{
  class Composition;
  class Event;
  class EventSelection;

  typedef QString NoteStyleName;
  
/* Make an ornament from a selection */
class CutToTriggerSegmentCommand : public BasicSelectionCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::CutToTriggerSegmentCommand)

    public:
    CutToTriggerSegmentCommand(EventSelection * selection,
                               Composition   &comp,
                               QString       name,
                               int           basePitch,
                               int           baseVelocity,
                               NoteStyleName noteStyle,
                               bool          retune,
                               std::string   timeAdjust,
                               Mark          mark);
    void execute() override;
    void unexecute() override;

protected:
    void modifySegment() override;

    PasteToTriggerSegmentWorker m_paster;
    EventSelection *            m_selection;
    timeT                       m_time;
    timeT                       m_duration;
    NoteStyleName               m_noteStyle;
    bool                        m_retune;
    std::string                 m_timeAdjust;
    Mark                        m_mark;
};
}

#endif /* ifndef RG_CUTTOTRIGGERSEGMENTCOMMAND_H */
