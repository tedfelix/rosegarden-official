
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

#ifndef RG_SEGMENTFORNOTATIONCOMMAND_H
#define RG_SEGMENTFORNOTATIONCOMMAND_H

#include "base/Segment.h"
#include "base/Selection.h"
#include "document/Command.h"
#include <QString>
#include <vector>
#include <QCoreApplication>


namespace Rosegarden
{


class SegmentForNotationCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SegmentForNotationCommand)

public:
    SegmentForNotationCommand(SegmentSelection &segments,
			      const bool flag);
    ~SegmentForNotationCommand() override;
    
    static QString getGlobalName()
        { return tr("Change Segment Notation flag..."); }

    void execute() override;
    void unexecute() override;
protected:

    std::vector<Segment*>     m_segments;
    std::vector<bool>         m_oldForNotationFlags;
    bool                      m_newForNotationFlag;
};


}

#endif
