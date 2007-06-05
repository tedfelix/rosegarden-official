
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_SEGMENTTRANSPOSECOMMAND_H_
#define _RG_SEGMENTTRANSPOSECOMMAND_H_

#include <kcommand.h>
#include "base/Event.h"
#include "document/MultiViewCommandHistory.h"
#include <klocale.h>

namespace Rosegarden
{

class Segment;
class SegmentSelection;


class SegmentTransposeCommand : public KMacroCommand
{
public:
    SegmentTransposeCommand(Segment &segment,
        bool changeKey, int steps, int semitones, bool transposeSegmentBack);

    SegmentTransposeCommand(SegmentSelection selection,
        bool changeKey, int steps, int semitones, bool transposeSegmentBack);

    virtual ~SegmentTransposeCommand();

    static QString getGlobalName(int semitones = 0, int step = 0) {
        switch (semitones) {
        default:  return i18n("Transpose by &Interval (EXPERIMENTAL)...");
        }
    }

protected:
    void processSegment(Segment &segment, bool changeKey, int steps, int semitones, bool transposeSegmentBack);
};

}

#endif
