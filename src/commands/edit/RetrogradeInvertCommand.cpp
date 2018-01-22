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

#define RG_MODULE_STRING "[RetrogradeInvertCommand]"

#include "RetrogradeInvertCommand.h"

#include "base/Event.h"
#include "misc/Debug.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "document/BasicSelectionCommand.h"
#include <QString>
#include "base/BaseProperties.h"


namespace Rosegarden
{

using namespace BaseProperties;

void
RetrogradeInvertCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    long highestPitch, lowestPitch;

    // Avoid "may be used uninitialized" compilation warnings
    highestPitch = 0;
    lowestPitch = 0;
    
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
    std::vector<Event *> toErase;
    std::vector<Event *> toInsert;

    timeT a0 = m_selection->getStartTime();
    timeT a1 = m_selection->getEndTime();

    bool useNotationTimings = false;

    for (i = m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        RG_DEBUG << "RetrogradeCommand::modifySegment: event at " << (*i)->getAbsoluteTime() << " type " << (*i)->getType();

        if ((*i)->isa(Note::EventRestType))
            continue;

        toErase.push_back(*i);
        timeT newTime = a0 + a1 - (*i)->getDuration() -
                        (useNotationTimings ?
                         (*i)->getNotationAbsoluteTime() : (*i)->getAbsoluteTime());

        Event *e;
        if (useNotationTimings) {
            e = new Event(**i, newTime, (*i)->getDuration(), (*i)->getSubOrdering(),
                          newTime, (*i)->getNotationDuration());
        } else {
            e = new Event(**i, newTime);
        }

        toInsert.push_back(e);
    }

    Segment &segment(m_selection->getSegment());

    for (size_t j = 0; j < toErase.size(); ++j) {
        Segment::iterator jtr(segment.findSingle(toErase[j]));
        if (jtr != segment.end()) {
            RG_DEBUG << "found event " << j;
            segment.erase(jtr);
        } else {
            RG_DEBUG << "failed to find event " << j;
        }
    }

    for (size_t j = 0; j < toInsert.size(); ++j) {

        Segment::iterator jtr = segment.end();

        // somewhat like the NoteOverlay part of PasteEventsCommand::modifySegment
        /* nah -- let's do a de-counterpoint afterwards perhaps
        	if (m_useNotationTimings && toInsert[j]->isa(Note::EventType)) {
        	    long pitch = 0;
        	    Accidental explicitAccidental = NoAccidental;
        	    toInsert[j]->get<String>(ACCIDENTAL, explicitAccidental);
        	    if (toInsert[j]->get<Int>(PITCH, pitch)) {
        		jtr = SegmentNotationHelper(segment).insertNote
        		    (toInsert[j]->getAbsoluteTime(),
        		     Note::getNearestNote(toInsert[j]->getDuration()),
        		     pitch, explicitAccidental);
        		delete toInsert[j];
        		toInsert[j] = *jtr;
        	    }
        	} else {
        */
        jtr = segment.insert(toInsert[j]);
        //	}

        // insert new event back into selection
        m_selection->addEvent(toInsert[j]);

        // if (jtr != segment.end()) m_lastInsertedEvent = toInsert[j];
    }

    segment.normalizeRests(a0, a1);
}

}
