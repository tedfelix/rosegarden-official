
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

#ifndef _RG_SEGMENTCHANGETRANSPOSECOMMAND_H_
#define _RG_SEGMENTCHANGETRANSPOSECOMMAND_H_

#include "document/Command.h"
#include <QString>
#include <vector>
#include "base/Event.h"




namespace Rosegarden
{

class Segment;


class SegmentChangeTransposeCommand : public NamedCommand
{
public:
    /// Set transpose on segments.
    SegmentChangeTransposeCommand(int, Segment *segment);
    virtual ~SegmentChangeTransposeCommand();

    virtual void execute();
    virtual void unexecute();

    static QString getGlobalName(int);

private:
    Segment *m_segment;
    int m_oldUnit;
    int m_unit;
};



}

#endif
