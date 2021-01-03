
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

#ifndef RG_AUDIOSEGMENTINSERTCOMMAND_H
#define RG_AUDIOSEGMENTINSERTCOMMAND_H

#include "base/RealTime.h"
#include "base/Track.h"
#include "sound/AudioFile.h"
#include "document/Command.h"
#include "base/Event.h"

#include <QCoreApplication>


namespace Rosegarden
{

class Studio;
class Segment;
class RosegardenDocument;
class Composition;
class AudioFileManager;


class AudioSegmentInsertCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::AudioSegmentInsertCommand)

public:
    AudioSegmentInsertCommand(RosegardenDocument *doc,
                              TrackId track,
                              timeT startTime,
                              AudioFileId audioFileId,
                              const RealTime &audioStartTime,
                              const RealTime &audioEndTime);
    ~AudioSegmentInsertCommand() override;

    Segment *getNewSegment() { return m_segment; }

    void execute() override;
    void unexecute() override;
    
private:
    Composition      *m_composition;
    AudioFileManager *m_audioFileManager;
    Segment          *m_segment;
    int               m_track;
    timeT             m_startTime;
    AudioFileId       m_audioFileId;
    RealTime          m_audioStartTime;
    RealTime          m_audioEndTime;
    bool              m_detached;
};


}

#endif
