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

#ifndef RG_LINKEDSEGMENTSCOMMAND_H
#define RG_LINKEDSEGMENTSCOMMAND_H

#include "base/Segment.h"
#include "document/Command.h"

#include <set>

class QString;

namespace Rosegarden
{

struct CompareForLinkedGroupSameTime
{
    // We equate only segments from the same link group starting at
    // the same time.
    bool operator()(const Segment * a, const Segment *b)
    {
        if(a->getLinker() < b->getLinker()) { return true; }
        if(a->getLinker() > b->getLinker()) { return false; }
        return a->getStartTime() < b->getStartTime();
    }        
};

typedef std::multiset<Segment *,CompareForLinkedGroupSameTime>
    LinkedGroups;

class LinkedSegmentsCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::LinkedSegmentsCommand)
public:
    typedef std::vector<Segment *> SegmentVec;
    LinkedSegmentsCommand(const QString &name,
                          SegmentVec originalSegments,
			  Composition *composition);
    ~LinkedSegmentsCommand() override;

protected:
    void execute()override = 0;
    void unexecute()override = 0;
    void executeAttachDetach();
    void unexecuteAttachDetach();
    // Copy auxilliary properties of source segment to target: track,
    // repeatingness, delay.  Does not include any segment-time properties.
    void copyAuxProperties(Segment *source, Segment *target);
    
    // The original segments, in a linked group that all start at the
    // same time or a singleton.
    SegmentVec m_originalSegments;

    // The resulting segments, again a linked group or singleton.
    SegmentVec m_newSegments;
    Composition *m_composition;

 private:
    bool m_detached;
};
 
}

#endif /* ifndef RG_LINKEDSEGMENTSCOMMAND_H */
