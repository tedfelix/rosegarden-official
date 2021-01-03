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

#define RG_MODULE_STRING "[SegmentTransposeCommand]"

#include "SegmentTransposeCommand.h"

#include "base/Selection.h"
#include "commands/notation/KeyInsertionCommand.h"
#include "commands/edit/TransposeCommand.h"
#include "commands/segment/SegmentChangeTransposeCommand.h"

namespace Rosegarden
{


SegmentTransposeCommand::SegmentTransposeCommand(
        Segment &segment,
        bool changeKey,
        int steps,
        int semitones,
        bool transposeSegmentBack) :
    MacroCommand(tr("Change segment transposition"))
{
    processSegment(segment, changeKey, steps, semitones, transposeSegmentBack);
}

SegmentTransposeCommand::SegmentTransposeCommand(
        SegmentSelection selection,
        bool changeKey,
        int steps,
        int semitones,
        bool transposeSegmentBack) :
    MacroCommand(tr("Change segment transposition"))
{
    // For each Segment in the SegmentSelection
    for (SegmentSelection::iterator i = selection.begin();
         i != selection.end();
         ++i) {
        Segment &segment = **i;    
        processSegment(segment,
                       changeKey,
                       steps,
                       semitones,
                       transposeSegmentBack);
    }
}

SegmentTransposeCommand::~SegmentTransposeCommand()
{
}

void 
SegmentTransposeCommand::processSegment(
        Segment &segment,
        bool changeKey,
        int steps,
        int semitones,
        bool transposeSegmentBack)
{
    MacroCommand *macroCommand = this;

    // ??? MEMORY LEAK (confirmed)
    //     TransposeCommand holds on to a pointer.
    //     Need to make TransposeCommand take a QSharedPointer.
    EventSelection *wholeSegment = new EventSelection(
            segment, segment.getStartTime(), segment.getEndMarkerTime());

    // Transpose the notes.
    macroCommand->addCommand(new TransposeCommand(
            semitones, steps, *wholeSegment));

    // Key insertion can do transposition, but a C4 to D becomes a D4, while
    //  a C4 to G becomes a G3. Because we let the user specify an explicit number
    //  of octaves to move the notes up/down, we add the keys without transposing
    //  and handle the transposition separately:
    if (changeKey) {

        Rosegarden::Key initialKey =
                segment.getKeyAtTime(segment.getStartTime());
        Rosegarden::Key newInitialKey = initialKey.transpose(semitones, steps);

        // For each Event in the Segment
        for (EventSelection::eventcontainer::iterator i =
                     wholeSegment->getSegmentEvents().begin();
             i != wholeSegment->getSegmentEvents().end();
             ++i) {

            // Not a Key event?  Try the next.
            if (!(*i)->isa(Rosegarden::Key::EventType))
                continue;

            Rosegarden::Key transposedKey =
                    (Rosegarden::Key(**i)).transpose(semitones, steps);

            macroCommand->addCommand(new KeyInsertionCommand(
                    segment,
                    (*i)->getAbsoluteTime(),  // time
                    transposedKey,
                    false,  // shouldConvert
                    false,  // shouldTranspose
                    false,  // shouldTransposeKey
                    true));  // shouldIgnorePercussion
        }

        KeyInsertionCommand *firstKeyCommand = new KeyInsertionCommand(
                segment,
                segment.getStartTime(),  // time
                newInitialKey,
                false,  // shouldConvert
                false,  // shouldTranspose
                false,  // shouldTransposeKey
                true);  // shouldIgnorePercussion

        macroCommand->addCommand(firstKeyCommand);
    }

    if (transposeSegmentBack) {
        // Transpose segment in opposite direction
        int newTranspose = segment.getTranspose() - semitones;
        macroCommand->addCommand(new SegmentChangeTransposeCommand(
                newTranspose, &segment));
    }
}


}
