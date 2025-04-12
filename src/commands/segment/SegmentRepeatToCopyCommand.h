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

#ifndef RG_SEGMENTREPEATTOCOPYCOMMAND_H
#define RG_SEGMENTREPEATTOCOPYCOMMAND_H

#include "document/Command.h"
#include <vector>

#include <QCoreApplication>


namespace Rosegarden
{

class Segment;
class Composition;


class SegmentRepeatToCopyCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SegmentRepeatToCopyCommand);

public:
    explicit SegmentRepeatToCopyCommand(Segment *segment);
    ~SegmentRepeatToCopyCommand() override;

    void execute() override;
    void unexecute() override;

private:

    Composition           *m_composition;
    Segment               *m_segment;
    std::vector<Segment*>  m_newSegments;
    bool                               m_detached;
};


}

#endif
