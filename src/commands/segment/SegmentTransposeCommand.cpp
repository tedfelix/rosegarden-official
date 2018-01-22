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


#include "SegmentTransposeCommand.h"

#include "base/Selection.h"
#include "commands/notation/KeyInsertionCommand.h"
#include "commands/edit/TransposeCommand.h"
#include "commands/segment/SegmentChangeTransposeCommand.h"

namespace Rosegarden
{
SegmentTransposeCommand::SegmentTransposeCommand(Segment &segment, bool changeKey, int steps, int semitones, bool transposeSegmentBack) :
        MacroCommand(tr("Change segment transposition"))
{
    processSegment(segment, changeKey, steps, semitones, transposeSegmentBack);
}

SegmentTransposeCommand::SegmentTransposeCommand(SegmentSelection selection, bool changeKey, int steps, int semitones, bool transposeSegmentBack) :
        MacroCommand(tr("Change segment transposition"))
{
    //SegmentSelection selection(m_view->getSelection());
    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) 
    {
        Segment &segment = **i;    
        processSegment(segment, changeKey, steps, semitones, transposeSegmentBack);
    }
}
     
void 
SegmentTransposeCommand::processSegment(Segment &segment, bool changeKey, int steps, int semitones, bool transposeSegmentBack)
{
    MacroCommand * macroCommand = this;

    // TODO delete it somewhere.
    EventSelection * wholeSegment = new EventSelection(segment, segment.getStartTime(), segment.getEndMarkerTime());
    macroCommand->addCommand(new TransposeCommand
        (semitones, steps, *wholeSegment));
    
    // Key insertion can do transposition, but a C4 to D becomes a D4, while
    //  a C4 to G becomes a G3. Because we let the user specify an explicit number
    //  of octaves to move the notes up/down, we add the keys without transposing
    //  and handle the transposition seperately:
    if (changeKey)
    {
        Rosegarden::Key initialKey = segment.getKeyAtTime(segment.getStartTime());
        Rosegarden::Key newInitialKey = initialKey.transpose(semitones, steps);

        EventSelection::eventcontainer::iterator i;
        //std::list<KeyInsertionCommand*> commands;

        for (i = wholeSegment->getSegmentEvents().begin();
            i != wholeSegment->getSegmentEvents().end(); ++i) {
                // transpose key
                if ((*i)->isa(Rosegarden::Key::EventType)) {
                    Rosegarden::Key trKey = (Rosegarden::Key (**i)).transpose(semitones, steps); 
                    //commands.push_front
                    macroCommand->addCommand
                        (new KeyInsertionCommand
                         (segment,
                           (*i)->getAbsoluteTime(),
                           trKey,
                          false,
                          false,
                          false,
			  true));
                    }
            }
        std::list<KeyInsertionCommand*>::iterator ci;
        //for (ci=commands.begin(); ci!=commands.end(); ci++)
        //{
        //    commandHistory->addCommand(*ci);
        //}
            
        KeyInsertionCommand *firstKeyCommand = new KeyInsertionCommand
             (segment,
              segment.getStartTime(),
              newInitialKey,
              false,
              false,
              false,
	      true);
        //commandHistory->addCommand(firstKeyCommand);
        macroCommand->addCommand(firstKeyCommand);
    }
        
    if (transposeSegmentBack)
    {
        // Transpose segment in opposite direction
        int newTranspose = segment.getTranspose() - semitones;
        macroCommand->addCommand(new SegmentChangeTransposeCommand(newTranspose, &segment));
    }
}


SegmentTransposeCommand::~SegmentTransposeCommand()
{}


}
