
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

#ifndef _RG_SEGMENTSPLITBYRECORDINGSRCCOMMAND_H_
#define _RG_SEGMENTSPLITBYRECORDINGSRCCOMMAND_H_

#include "document/Command.h"
#include <QString>
#include <klocale.h>


namespace Rosegarden
{

class Segment;
class Composition;


class SegmentSplitByRecordingSrcCommand : public NamedCommand
{
public:
    SegmentSplitByRecordingSrcCommand(Segment *segment,
                                      int channel, int device);
    virtual ~SegmentSplitByRecordingSrcCommand();
    
    static QString getGlobalName()
        { return i18n("Split by &Recording Source..."); }

    virtual void execute();
    virtual void unexecute();

private:
    Composition *m_composition;
    Segment *m_segment;
    Segment *m_newSegmentA;
    Segment *m_newSegmentB;
    int m_channel;
    int m_device;
    bool m_executed;
};


}

#endif
