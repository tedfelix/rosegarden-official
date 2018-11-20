
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

#ifndef RG_ADDTRIGGERSEGMENTCOMMAND_H
#define RG_ADDTRIGGERSEGMENTCOMMAND_H

#include "base/TriggerSegment.h"
#include "document/Command.h"
#include "base/Event.h"

#include <QCoreApplication>


namespace Rosegarden
{

class Segment;
class RosegardenDocument;
class Composition;


class AddTriggerSegmentCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::AddTriggerSegmentCommand)

public:
    AddTriggerSegmentCommand(RosegardenDocument *doc,
                             timeT duration, // start time always 0
                             int basePitch = -1,
                             int baseVelocity = -1);
    virtual ~AddTriggerSegmentCommand();

    TriggerSegmentId getId() const; // after invocation

    void execute() override;
    void unexecute() override;

private:
    Composition *m_composition;
    timeT m_duration;
    int m_basePitch;
    int m_baseVelocity;
    TriggerSegmentId m_id;
    Segment *m_segment;
    bool m_detached;
};



}

#endif
