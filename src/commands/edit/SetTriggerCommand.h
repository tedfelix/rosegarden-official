/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SETTRIGGERCOMMAND_H
#define RG_SETTRIGGERCOMMAND_H

#include "base/TriggerSegment.h"
#include "document/BasicCommand.h"

#include <QString>

#include <string>


namespace Rosegarden
{


class EventSelection;


class SetTriggerCommand : public BasicCommand
{
public:
    SetTriggerCommand(EventSelection &selection,
                      TriggerSegmentId triggerSegmentId,
                      bool notesOnly,
                      bool retune,
                      const std::string& timeAdjust,
                      Mark mark,
                      QString name) :
        BasicCommand(name,
                     selection,
                     true),  // bruteForceRedo
        m_selection(&selection),
        m_triggerSegmentId(triggerSegmentId),
        m_notesOnly(notesOnly),
        m_retune(retune),
        m_timeAdjust(timeAdjust),
        m_mark(mark)
    { }

protected:
    void modifySegment() override;

private:
    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    TriggerSegmentId m_triggerSegmentId;
    bool m_notesOnly;
    bool m_retune;
    std::string m_timeAdjust;
    Mark m_mark;
};


}

#endif
