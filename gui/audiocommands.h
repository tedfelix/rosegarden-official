// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef AUDIO_COMMANDS_H
#define AUDIO_COMMANDS_H


#include <klocale.h>
#include <kcommand.h>
#include "Composition.h"
#include "Selection.h"

namespace Rosegarden
{
    class Segment;
    class AudioFile;
}

/**
 *
 * DistributeAudioCommand - Distribute an Audio Segment triggered
 * against the MIDI Note ons in a SegmentSelection.
 *
 */
class DistributeAudioCommand : public KNamedCommand
{
public:
    DistributeAudioCommand(Rosegarden::Composition *comp,
                           Rosegarden::SegmentSelection &inputSelection,
                           Rosegarden::Segment *audioSegment);

    DistributeAudioCommand(Rosegarden::Composition *comp,
                           Rosegarden::SegmentSelection &inputSelection,
                           Rosegarden::AudioFile *audioFile);

    virtual ~DistributeAudioCommand();

    static QString getGlobalName() 
        { return i18n("Distribute Audio Segments over MIDI"); }


    virtual void execute();
    virtual void unexecute();

protected:
    Rosegarden::Composition          *m_composition;
    Rosegarden::SegmentSelection      m_selection;
    Rosegarden::AudioFile            *m_audioFile;
    Rosegarden::Segment              *m_audioSegment;
    std::vector<Rosegarden::Segment*> m_newSegments;
    bool                              m_executed;

};


#endif  // AUDIO_COMMANDS_H
