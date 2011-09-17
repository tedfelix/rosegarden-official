/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2010 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "SegmentLinkTransposeCommand.h"

#include "SegmentTransposeCommand.h"
#include "base/SegmentLinker.h"

namespace Rosegarden
{
    
SegmentLinkTransposeCommand::SegmentLinkTransposeCommand(
        std::vector<Segment *> linkedSegs, bool changeKey, int steps,
        int semitones, bool transposeSegmentBack) :
    MacroCommand(getGlobalName()),
    m_linkedSegs(linkedSegs),
    m_linkTransposeParams(changeKey,steps,semitones,transposeSegmentBack)
{
    //make this command not update link siblings by default
    setUpdateLinks(false);
    //add the individual transpose commands here
    std::vector<Segment *>::iterator itr;
    for (itr = m_linkedSegs.begin(); itr != m_linkedSegs.end(); ++itr) {
        addCommand(new SegmentTransposeCommand(**itr, changeKey, steps,
                                              semitones, transposeSegmentBack));
        m_oldLinkTransposeParams.push_back((*itr)->getLinkTransposeParams());
    }
}

SegmentLinkTransposeCommand::~SegmentLinkTransposeCommand()
{
    // nothing
}

void
SegmentLinkTransposeCommand::execute()
{
    std::vector<Segment *>::iterator itr;
    for (itr = m_linkedSegs.begin(); itr != m_linkedSegs.end(); ++itr) {
        Segment *linkedSeg = *itr;
        linkedSeg->setLinkTransposeParams(m_linkTransposeParams);
    }
    MacroCommand::execute();
}

void
SegmentLinkTransposeCommand::unexecute()
{
    std::vector<Segment *>::iterator itr;
    for (itr = m_linkedSegs.begin(); itr != m_linkedSegs.end(); ++itr) {
        Segment *linkedSeg = *itr;
        Segment::LinkTransposeParams oldParams = 
            m_oldLinkTransposeParams[std::distance(m_linkedSegs.begin(),itr)];
        linkedSeg->setLinkTransposeParams(oldParams);
    }
    MacroCommand::unexecute();
}

SegmentLinkResetTransposeCommand::SegmentLinkResetTransposeCommand(
                                    std::vector<Segment *> &linkedSegs) :
    MacroCommand(getGlobalName())
{
    //make this command not update link siblings by default
    setUpdateLinks(false);
    std::vector<Segment *>::iterator itr;
    for (itr = linkedSegs.begin(); itr != linkedSegs.end(); ++itr) {
        addCommand(new SingleSegmentLinkResetTransposeCommand(**itr));
    }
}

void
SingleSegmentLinkResetTransposeCommand::modifySegment()
{
    m_linkedSeg->getLinker()->refreshSegment(m_linkedSeg);
}

void
SingleSegmentLinkResetTransposeCommand::execute()
{
    m_linkedSeg->setLinkTransposeParams(Segment::LinkTransposeParams());
    BasicCommand::execute();
}

void
SingleSegmentLinkResetTransposeCommand::unexecute()
{
    m_linkedSeg->setLinkTransposeParams(m_linkTransposeParams);
    BasicCommand::unexecute();
}

}
