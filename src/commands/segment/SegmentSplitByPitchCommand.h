
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

#ifndef _RG_SEGMENTSPLITBYPITCHCOMMAND_H_
#define _RG_SEGMENTSPLITBYPITCHCOMMAND_H_

#include "base/Segment.h"
#include <kcommand.h>
#include <qstring.h>
#include <klocale.h>
#include "gui/general/ClefIndex.h"




namespace Rosegarden
{

class Composition;


class SegmentSplitByPitchCommand : public KNamedCommand
{
public:
    enum ClefHandling {
        LeaveClefs,
        RecalculateClefs,
        UseTrebleAndBassClefs
    };
    
    SegmentSplitByPitchCommand(Segment *segment,
                               int splitPitch,
                               bool ranging,
                               bool duplicateNonNoteEvents,
                               ClefHandling clefHandling);
    virtual ~SegmentSplitByPitchCommand();

    static QString getGlobalName()
        { return i18n("Split by &Pitch..."); }

    virtual void execute();
    virtual void unexecute();

private:
    int getSplitPitchAt(Segment::iterator i, int lastSplitPitch);

    Composition *m_composition;
    Segment *m_segment;
    Segment *m_newSegmentA;
    Segment *m_newSegmentB;
    int m_splitPitch;
    bool m_ranging;
    bool m_dupNonNoteEvents;
    ClefHandling m_clefHandling;
    bool m_executed;
};


}

#endif
