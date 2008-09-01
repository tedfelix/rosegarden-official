
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_SEGMENTERASECOMMAND_H_
#define _RG_SEGMENTERASECOMMAND_H_

#include <string>
#include "document/Command.h"




namespace Rosegarden
{

class Segment;
class Composition;
class AudioFileManager;


////////////////////////////////////////////////////////////

class SegmentEraseCommand : public NamedCommand
{
public:
    /// for removing segment normally
    SegmentEraseCommand(Segment *segment);

    /// for removing audio segment when removing an audio file
    SegmentEraseCommand(Segment *segment,
                        AudioFileManager *mgr);
    virtual ~SegmentEraseCommand();

    virtual void execute();
    virtual void unexecute();
    
private:
    Composition *m_composition;
    Segment *m_segment;
    AudioFileManager *m_mgr;
    std::string m_audioFileName;
    bool m_detached;
};


}

#endif
