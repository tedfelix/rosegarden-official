
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

#ifndef RG_AUDIOSEGMENTAUTOSPLITCOMMAND_H
#define RG_AUDIOSEGMENTAUTOSPLITCOMMAND_H

#include "document/Command.h"
#include <QString>
#include <vector>
#include <QCoreApplication>




namespace Rosegarden
{

class Segment;
class RosegardenDocument;
class Composition;
class AudioFileManager;


class AudioSegmentAutoSplitCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::AudioSegmentAutoSplitCommand)

public:
    AudioSegmentAutoSplitCommand(RosegardenDocument *doc,
                                 Segment *segment,
                                 int threshold);
    virtual ~AudioSegmentAutoSplitCommand();

    virtual void execute();
    virtual void unexecute();
    
    static QString getGlobalName() { return tr("&Split on Silence"); }

private:
    Segment                *m_segment;
    Composition            *m_composition;
    AudioFileManager       *m_audioFileManager;
    std::vector<Segment *>  m_newSegments;
    bool                                m_detached;
    int                                 m_threshold;
};


}

#endif
