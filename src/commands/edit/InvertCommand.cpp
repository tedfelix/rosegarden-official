/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "InvertCommand.h"

#include "base/NotationTypes.h"
#include "base/Selection.h"
#include "document/BasicSelectionCommand.h"
#include <QString>
#include "base/BaseProperties.h"


namespace Rosegarden
{

using namespace BaseProperties;

void
InvertCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;
    long highestPitch, lowestPitch;
	highestPitch=0; lowestPitch=0;	// remove compiler warning;
	
    bool firstNote = true;
    for (i = m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        if ((*i)->isa(Note::EventType)) {
            try {
                long pitch = (*i)->get
                             <Int>(PITCH);
                if (firstNote) {
                    highestPitch = pitch;
                    lowestPitch = pitch;
                    firstNote = false;
                } else {
                    if (pitch > highestPitch)
                        highestPitch = pitch;
                    else if (pitch < lowestPitch)
                        lowestPitch = pitch;
                }
            } catch (...) { }
        }
    }

    for (i = m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        if ((*i)->isa(Note::EventType)) {
            try {
                long pitch = (*i)->get
                             <Int>(PITCH);
                pitch = lowestPitch + (highestPitch - pitch);
                pitch += m_semitones;
                (*i)->set
                <Int>(PITCH, pitch);
                (*i)->unset(ACCIDENTAL);
            } catch (...) { }
        }
    }
}

}
