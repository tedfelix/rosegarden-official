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


#include "SegmentSyncCommand.h"

#include "base/Selection.h"
#include "commands/notation/KeyInsertionCommand.h"
#include "commands/edit/TransposeCommand.h"
#include "commands/segment/SegmentChangeTransposeCommand.h"
#include "commands/segment/SegmentTransposeCommand.h"
#include "commands/segment/SegmentSyncClefCommand.h"

namespace Rosegarden
{
SegmentSyncCommand::SegmentSyncCommand(Segment &segment, int newTranspose, int lowRange, int highRange, const Clef& clef) :
        KMacroCommand(i18n("Sync segment parameters"))
{
    processSegment(segment, newTranspose, lowRange, highRange, clef);
}

SegmentSyncCommand::SegmentSyncCommand(SegmentSelection selection, int newTranspose, int lowRange, int highRange, const Clef& clef) :
        KMacroCommand(i18n("Sync segment parameters"))
{
    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) 
    {
        Segment &segment = **i;    
        processSegment(segment, newTranspose, lowRange, highRange, clef);
    }
}

SegmentSyncCommand::SegmentSyncCommand(std::vector<Segment *> segments, int newTranspose, int lowRange, int highRange, const Clef& clef) :
        KMacroCommand(i18n("Sync segment parameters"))
{
	for (int i = 0; i < segments.size(); i++) {
        processSegment(*(segments[i]), newTranspose, lowRange, highRange, clef);
    }
}

SegmentSyncCommand::SegmentSyncCommand(Composition::segmentcontainer& segments, TrackId selectedTrack, int newTranspose, int lowRange, int highRange, const Clef& clef) :
        KMacroCommand(i18n("Sync segment parameters"))
{
    for (Composition::segmentcontainer::const_iterator si = segments.begin();
                        si != segments.end(); ++si) {
        if ((*si)->getTrack() == selectedTrack) {
            processSegment(**si, newTranspose, lowRange, highRange, clef);
        }
    }
}

void 
SegmentSyncCommand::processSegment(Segment &segment, int newTranspose, int lowRange, int highRange, const Clef& clef)
{
    KMacroCommand * macroCommand = this;
    
    // if the new desired setting for 'transpose' is higher, we need to 
    // transpose the notes upwards to compensate:
    int semitones = segment.getTranspose() - newTranspose;
    
    // Say the old transpose was -2 and the new is 0, this corresponds to
    // Bb and C. The height of the old transpose is 1 below the height of the new.
    int oldHeight = Pitch(segment.getTranspose()).getHeightOnStaff(Clef::Treble, Key("C major"));
    int newHeight = Pitch(newTranspose).getHeightOnStaff(Clef::Treble, Key("C major"));
    int steps = oldHeight - newHeight;

    SegmentTransposeCommand* command = new SegmentTransposeCommand(segment, true, steps, semitones, true);
    macroCommand->addCommand(command);
    
    // TODO do this in an undoable fashion:
    segment.setLowestPlayable(lowRange);
    segment.setHighestPlayable(highRange);
    
    macroCommand->addCommand(new SegmentSyncClefCommand(segment, clef));
}


SegmentSyncCommand::~SegmentSyncCommand()
{}


}
