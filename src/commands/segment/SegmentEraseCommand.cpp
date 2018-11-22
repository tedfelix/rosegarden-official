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

#define RG_MODULE_STRING "[SegmentEraseCommand]"

#include "SegmentEraseCommand.h"

#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/Segment.h"
#include "sound/AudioFile.h"
#include "sound/AudioFileManager.h"


namespace Rosegarden
{

SegmentEraseCommand::SegmentEraseCommand(Segment *segment) :
        NamedCommand(tr("Erase Segment")),
        m_composition(segment->getComposition()),
        m_segment(segment),
        m_mgr(nullptr),
        m_audioFileName(""),
        m_detached(false)
{
    // nothing else
}

SegmentEraseCommand::SegmentEraseCommand(Segment *segment,
        AudioFileManager *mgr) :
        NamedCommand(tr("Erase Segment")),
        m_composition(segment->getComposition()),
        m_segment(segment),
        m_mgr(mgr),
        m_detached(false)
{
    // If this is an audio segment, we want to make a note of
    // its associated file name in case we need to undo and restore
    // the file.
    if (m_segment->getType() == Segment::Audio) {
        unsigned int id = m_segment->getAudioFileId();
        AudioFile *file = mgr->getAudioFile(id);
        if (file)
            m_audioFileName = file->getFilename();
    }
}

SegmentEraseCommand::~SegmentEraseCommand()
{
    // This is the only place in this command that the Segment can
    // safely be deleted, and then only if it is not in the
    // Composition (i.e. if we executed more recently than we
    // unexecuted).  Can't safely call through the m_segment pointer
    // here; someone else might have got to it first

    if (m_detached) {
        delete m_segment;
    }
}

void
SegmentEraseCommand::execute()
{
    m_composition->detachSegment(m_segment);
    m_detached = true;
}

void
SegmentEraseCommand::unexecute()
{
    m_composition->addSegment(m_segment);
    m_detached = false;

    // If we are bringing back an audio segment
    if (m_segment->getType() == Segment::Audio &&
            m_audioFileName != "" &&
            m_mgr) {
        // Use the stashed audio filename to get the audio file ID.
        // The user may have deleted and re-added the audio file, which
        // would cause its ID to change.
        int id = m_mgr->fileExists(m_audioFileName);

        // If the audio file wasn't found, it may have been deleted
        // by the user.  Re-add it.
        // (When some parts of a system use the command history and others
        // don't (AudioFileManager), expect evil kludges such as this.)
        // In my testing, the audio file no longer plays after this.
        if (id == -1)
            id = (int)m_mgr->addFile(m_audioFileName);

        m_segment->setAudioFileId(id);
    }
}

}
