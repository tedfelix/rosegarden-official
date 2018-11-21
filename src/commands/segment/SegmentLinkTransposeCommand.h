
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

#ifndef RG_SEGMENTLINKTRANSPOSECOMMAND_H
#define RG_SEGMENTLINKTRANSPOSECOMMAND_H

#include "document/Command.h"
#include "document/BasicCommand.h"
#include "document/BasicSelectionCommand.h"
#include "base/Selection.h"

#include <QCoreApplication>

namespace Rosegarden
{

class LinkedSegment;


class SegmentLinkTransposeCommand : public MacroCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SegmentLinkTransposeCommand)

public:
    /// Set transpose on segments.
    SegmentLinkTransposeCommand(std::vector<Segment *> linkedSegs,
        bool changeKey, int steps, int semitones, bool transposeSegmentBack);
    ~SegmentLinkTransposeCommand() override;

    void execute() override;
    void unexecute() override;

    static QString getGlobalName() { return tr("Transpose Linked Segments"); }

private:
    std::vector<Segment *> m_linkedSegs;

    ///new parameters
    Segment::LinkTransposeParams m_linkTransposeParams;
    
    ///old parameters
    std::vector<Segment::LinkTransposeParams> m_oldLinkTransposeParams;
};

class SegmentLinkResetTransposeCommand : public MacroCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SegmentLinkResetTransposeCommand)

public:
    SegmentLinkResetTransposeCommand(std::vector<Segment *> &linkedSegs);

    static QString getGlobalName() {
        return tr("Reset Transpose on Linked Segments"); }
};

class SingleSegmentLinkResetTransposeCommand : public BasicSelectionCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SingleSegmentLinkResetTransposeCommand)

public:
    SingleSegmentLinkResetTransposeCommand(Segment &linkedSeg) :
        BasicSelectionCommand(getGlobalName(), linkedSeg, true),
        m_linkedSeg(&linkedSeg),
        m_linkTransposeParams(m_linkedSeg->getLinkTransposeParams()) { }

    static QString getGlobalName() {
        return tr("Reset Transpose on Linked Segment"); }

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
