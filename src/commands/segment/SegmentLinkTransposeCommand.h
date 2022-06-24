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

#ifndef RG_SEGMENTLINKTRANSPOSECOMMAND_H
#define RG_SEGMENTLINKTRANSPOSECOMMAND_H

#include "document/BasicCommand.h"
#include "document/Command.h"
#include "base/Segment.h"

#include <QCoreApplication>

#include <vector>


namespace Rosegarden
{


/// Hidden feature of the Segment Parameters box (SegmentParameterBox).
class SegmentLinkTransposeCommand : public MacroCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SegmentLinkTransposeCommand);

public:
    /// Set transpose on segments.
    explicit SegmentLinkTransposeCommand
      (const std::vector<Segment *>& linkedSegs,
       bool changeKey, int steps, int semitones, bool transposeSegmentBack);
    ~SegmentLinkTransposeCommand() override;

    void execute() override;
    void unexecute() override;

private:
    std::vector<Segment *> m_linkedSegs;

    ///new parameters
    Segment::LinkTransposeParams m_linkTransposeParams;

    ///old parameters
    std::vector<Segment::LinkTransposeParams> m_oldLinkTransposeParams;
};

// ********************************************************************

class SegmentLinkResetTransposeCommand : public MacroCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SegmentLinkResetTransposeCommand);

public:
    explicit SegmentLinkResetTransposeCommand
      (std::vector<Segment *> &linkedSegs);

};

// ********************************************************************

class SingleSegmentLinkResetTransposeCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SingleSegmentLinkResetTransposeCommand);

public:
    explicit SingleSegmentLinkResetTransposeCommand(Segment &linkedSeg) :
        BasicCommand(tr("Reset Transpose on Linked Segment"),
                     linkedSeg,
                     linkedSeg.getStartTime(),
                     linkedSeg.getEndMarkerTime(),
                     true),  // bruteForceRedo
        m_linkedSeg(&linkedSeg),
        m_linkTransposeParams(m_linkedSeg->getLinkTransposeParams())
    { }

    void execute() override;
    void unexecute() override;

protected:
    void modifySegment() override;

private:
    Segment *m_linkedSeg;
    Segment::LinkTransposeParams m_linkTransposeParams;
};


}

#endif
