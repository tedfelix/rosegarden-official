
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SEGMENTRESCALECOMMAND_H
#define RG_SEGMENTRESCALECOMMAND_H

#include "document/Command.h"
#include <QString>
#include "base/Event.h"
#include <QCoreApplication>




namespace Rosegarden
{

class Segment;


class SegmentRescaleCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SegmentRescaleCommand)

public:
    SegmentRescaleCommand(Segment *segment,
                          int multiplier,
                          int divisor);
    SegmentRescaleCommand(Segment *segment,
                          int multiplier,
                          int divisor,
                          timeT newStartTime);
    ~SegmentRescaleCommand() override;

    void execute() override;
    void unexecute() override;

    static QString getGlobalName() { return tr("Stretch or S&quash..."); }

private:
    Segment *m_segment;
    Segment *m_newSegment;
    bool m_startTimeGiven;
    timeT m_startTime;
    int m_multiplier;
    int m_divisor;
    bool m_detached;

    timeT rescale(timeT) const;
};



}

#endif
