/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "TransposeCommand.h"

#include "base/NotationTypes.h"
#include "base/Selection.h"
#include "base/BaseProperties.h"


namespace Rosegarden
{


void
TransposeCommand::modifySegment()
{
    EventContainer::iterator i;
    
    for (i = m_selection->getSegmentEvents().begin();
         i != m_selection->getSegmentEvents().end(); ++i) {
        
        if ((*i)->isa(Note::EventType)) {
            
            if (m_diatonic) { 
            
                Pitch oldPitch(**i);
        
                timeT noteTime = (*i)->getAbsoluteTime();
                Key key = m_selection->getSegment().getKeyAtTime(noteTime);
                Pitch newPitch = oldPitch.transpose(key, m_semitones, m_steps);

                // fix #1415: constrain results to valid MIDI pitches
                if (newPitch.getPerformancePitch() > 127) newPitch = Pitch(127);
                if (newPitch.getPerformancePitch() < 0) newPitch = Pitch(0);

                Event *newNoteEvent = newPitch.getAsNoteEvent(0, 0);
                Accidental newAccidental;
                newNoteEvent->get<String>(BaseProperties::ACCIDENTAL, newAccidental);
                delete newNoteEvent;

                (*i)->set<Int>(BaseProperties::PITCH, newPitch.getPerformancePitch());
                (*i)->set<String>(BaseProperties::ACCIDENTAL, newAccidental);
            } else {
                try {
                    long pitch = (*i)->get<Int>(BaseProperties::PITCH);
                    pitch += m_semitones;

                    // fix #1415: constrain results to valid MIDI pitches
                    if (pitch > 127) pitch = 127;
                    if (pitch < 0) pitch = 0;

                    (*i)->set<Int>(BaseProperties::PITCH, pitch);
                    if ((m_semitones % 12) != 0) {
                        (*i)->unset(BaseProperties::ACCIDENTAL);
                    }
                } catch (...) { }
            }

        }
    }
}


}
