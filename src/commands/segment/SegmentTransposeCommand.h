
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

#ifndef RG_SEGMENTTRANSPOSECOMMAND_H
#define RG_SEGMENTTRANSPOSECOMMAND_H

#include "document/Command.h"
#include "base/Event.h"
#include "base/Selection.h"
#include "document/CommandHistory.h"
#include <QCoreApplication>

namespace Rosegarden
{

class Segment;


class ROSEGARDENPRIVATE_EXPORT SegmentTransposeCommand : public MacroCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SegmentTransposeCommand)

public:
    SegmentTransposeCommand(Segment &segment,
        bool changeKey, int steps, int semitones, bool transposeSegmentBack);

    SegmentTransposeCommand(SegmentSelection selection,
        bool changeKey, int steps, int semitones, bool transposeSegmentBack);

    virtual ~SegmentTransposeCommand();

    static QString getGlobalName(int semitones = 0) {
        switch (semitones) {
        default:  return tr("Transpose by &Interval...");
        }
    }

protected:
    void processSegment(Segment &segment, bool changeKey, int steps, int semitones, bool transposeSegmentBack);
};

}

#endif
