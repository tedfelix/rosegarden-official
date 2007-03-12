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


#include "TransposeCommand.h"

#include <iostream>
#include "base/NotationTypes.h"
#include "base/Selection.h"
#include "document/BasicSelectionCommand.h"
#include <qstring.h>
#include "base/BaseProperties.h"


namespace Rosegarden
{

using namespace BaseProperties;
using namespace Accidentals;

void
TransposeCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i = m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        if ((*i)->isa(Note::EventType)) {
	    if (m_diatonic)
	    { 
		// Finding out the new pitch and accidental:
		// - find out the old accidental and pitch
		// - calculate the old step from that
		// - calculate the new step and pitch using m_semitones and m_steps
		// - calculate the new accidental by step and pitch
		
		// - find out the old accidental and pitch
		//Accidental oldAccidental = (*i)->get<String>(ACCIDENTAL, Accidentals::NoAccidental);
		Accidental oldAccidental = NoAccidental;
		(*i)->get<String>(ACCIDENTAL, oldAccidental);
		long oldPitch = (*i)->get<Int>(PITCH);

		// - calculate the old step from that
		int oldAccidentalPitchOffset = Accidentals::getPitchOffset(oldAccidental);
		int oldStepNaturalPitch = oldPitch - oldAccidentalPitchOffset;
		static int steps[] = { 0,0,1,2,2,3,3,4,4,5,6,6 };
		int oldStep = steps[oldStepNaturalPitch % 12] + (oldStepNaturalPitch / 12) * 7;

		// - calculate the new step and pitch using m_semitones and m_steps
		long newPitch = oldPitch + m_semitones;
		int newStep   = oldStep  + m_steps;
		(*i)->set<Int>(PITCH,newPitch);

		// - calculate the new accidental by step and pitch
		static int stepIntervals[] = { 0,2,4,5,7,9,11 };
		int newAccidentalOffset = newPitch - ((newStep / 7) * 12 + stepIntervals[newStep % 7]);
		(*i)->set<String>(ACCIDENTAL,Accidentals::getAccidental(newAccidentalOffset));
	    }
	    else
	    {
		try {
		    long pitch = (*i)->get<Int>(PITCH);
		    pitch += m_semitones;
		    (*i)->set<Int>(PITCH, pitch);
		    if ((m_semitones % 12) != 0) {
			(*i)->unset(ACCIDENTAL);
		    }
		} catch (...) { }
	    }
        }
    }
}

}
