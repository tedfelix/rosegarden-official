
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

#ifndef RG_SEGMENTCOLOURCOMMAND_H
#define RG_SEGMENTCOLOURCOMMAND_H

#include "base/Segment.h"
#include "base/Selection.h"
#include "document/Command.h"
#include <QString>
#include <vector>
#include <QCoreApplication>


namespace Rosegarden
{


class SegmentColourCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SegmentColourCommand)

public:
    SegmentColourCommand(SegmentSelection &segments,
                         const unsigned int index);
    virtual ~SegmentColourCommand();

    static QString getGlobalName()
        { return tr("Change Segment Color..."); }

    virtual void execute();
    virtual void unexecute();
protected:

    std::vector<Segment*>     m_segments;
    std::vector<unsigned int>             m_oldColourIndexes;
    unsigned int                          m_newColourIndex;
};


}

#endif
