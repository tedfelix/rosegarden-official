
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_AUDIOSEGMENTRESIZEFROMSTARTCOMMAND_H
#define RG_AUDIOSEGMENTRESIZEFROMSTARTCOMMAND_H

#include "document/Command.h"
#include "base/Event.h"

#include <QCoreApplication>


namespace Rosegarden
{

class Segment;


/**
 * As above, but for audio segments.
 */
class AudioSegmentResizeFromStartCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::AudioSegmentResizeFromStartCommand)

public:
    AudioSegmentResizeFromStartCommand(Segment *segment,
                                       timeT newStartTime);
    ~AudioSegmentResizeFromStartCommand() override;

    void execute() override;
    void unexecute() override;

private:
    Segment *m_segment;
    Segment *m_newSegment;
    bool m_detached;
    timeT m_oldStartTime;
    timeT m_newStartTime;
};



}

#endif
