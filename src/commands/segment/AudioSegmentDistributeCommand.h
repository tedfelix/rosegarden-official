
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

#ifndef RG_AUDIOSEGMENTDISTRIBUTECOMMAND_H
#define RG_AUDIOSEGMENTDISTRIBUTECOMMAND_H

#include "base/Selection.h"
#include "document/Command.h"
#include <QString>
#include <vector>
#include <QCoreApplication>




namespace Rosegarden
{

class Segment;
class Composition;
class AudioFile;


/**
 * AudioSegmentDistributeCommand - Distribute an Audio Segment triggered
 * against the MIDI Note ons in a SegmentSelection.
 *
 * (I think this is actually unused --cc)
 */
class AudioSegmentDistributeCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::AudioSegmentDistributeCommand)

public:
    AudioSegmentDistributeCommand(Composition *comp,
                                  SegmentSelection &inputSelection,
                                  Segment *audioSegment);

    AudioSegmentDistributeCommand(Composition *comp,
                                  SegmentSelection &inputSelection,
                                  AudioFile *audioFile);

    virtual ~AudioSegmentDistributeCommand();

    static QString getGlobalName() 
        { return tr("Distribute Audio Segments over MIDI"); }


    void execute() override;
    void unexecute() override;

protected:
    Composition          *m_composition;
    SegmentSelection      m_selection;
    AudioFile            *m_audioFile;
    Segment              *m_audioSegment;
    std::vector<Segment*> m_newSegments;
    bool                              m_executed;

};



}

#endif
