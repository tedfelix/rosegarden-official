/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SEGMENTERASECOMMAND_H
#define RG_SEGMENTERASECOMMAND_H

#include <string>
#include "document/Command.h"

#include <QCoreApplication>


namespace Rosegarden
{

class Segment;
class Composition;
class AudioFileManager;


////////////////////////////////////////////////////////////

class SegmentEraseCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SegmentEraseCommand);

public:
    /// for removing segment normally
    explicit SegmentEraseCommand(Segment *segment);

    /// for removing audio segment when removing an audio file
    SegmentEraseCommand(Segment *segment,
                        AudioFileManager *mgr);
    ~SegmentEraseCommand() override;

    void execute() override;
    void unexecute() override;

private:
    Composition *m_composition;
    Segment *m_segment;
    AudioFileManager *m_mgr;
    QString m_audioFileName;
    bool m_detached;
};


}

#endif
