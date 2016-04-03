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


#include "NoteInsertionCommand.h"

#include <cmath>
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/SegmentMatrixHelper.h"
#include "base/SegmentNotationHelper.h"
#include "document/BasicCommand.h"
#include "gui/editors/notation/NotationProperties.h"
#include "gui/editors/notation/NoteStyleFactory.h"
#include "base/BaseProperties.h"
#include "base/Selection.h"
#include "misc/Strings.h"
#include "misc/Debug.h"


namespace Rosegarden
{

using namespace BaseProperties;

NoteInsertionCommand::NoteInsertionCommand(Segment &segment, timeT time,
                                           timeT endTime, Note note, int pitch,
                                           Accidental accidental,
                                           AutoBeamMode autoBeam,
                                           AutoTieBarlinesMode autoTieBarlines,
                                           MatrixMode matrixType,
                                           GraceMode grace,
                                           float targetSubordering,
                                           NoteStyleName noteStyle,
                                           int velocity) :
        BasicCommand(tr("Insert Note"), segment,
                     getModificationStartTime(segment, time),
                     (autoBeam ? segment.getBarEndForTime(endTime) : endTime)),
        m_insertionTime(time),
        m_note(note),
        m_pitch(pitch),
        m_accidental(accidental),
        m_autoBeam(autoBeam == AutoBeamOn),
        m_autoTieBarlines(autoTieBarlines == AutoTieBarlinesOn),
        m_matrixType(matrixType == MatrixModeOn),
        m_grace(grace),
        m_targetSubordering(targetSubordering),
        m_noteStyle(noteStyle),
        m_velocity(velocity),
        m_lastInsertedEvent(0)
{
    // nothing
}

NoteInsertionCommand::~NoteInsertionCommand()
{
    // nothing
}

EventSelection *
NoteInsertionCommand::getSubsequentSelection()
{
    EventSelection *selection = new EventSelection(getSegment());
    selection->addEvent(getLastInsertedEvent());
    return selection;
}

timeT
NoteInsertionCommand::getModificationStartTime(Segment &segment,
        timeT time)
{
    // We may be splitting a rest to insert this note, so we'll have
    // to record the change from the start of that rest rather than
    // just the start of the note

    timeT barTime = segment.getBarStartForTime(time);
    Segment::iterator i = segment.findNearestTime(time);

    if (i != segment.end() &&
            (*i)->getAbsoluteTime() < time &&
            (*i)->getAbsoluteTime() + (*i)->getDuration() > time &&
            (*i)->isa(Note::EventRestType)) {
        return std::min(barTime, (*i)->getAbsoluteTime());
    }

    return barTime;
}

void
NoteInsertionCommand::modifySegment()
{
    Segment &segment(getSegment());
    SegmentNotationHelper helper(segment);
    Segment::iterator i, j;

    // insert via a model event, so as to apply the note style

    // subordering is always negative for these insertions; round it down
    int actualSubordering = lrintf(floorf(m_targetSubordering + 0.01));
    if ((m_grace != GraceModeOff) && actualSubordering >= 0) {
        actualSubordering = -1;
    }

    // this is true if the subordering is "more or less" an integer,
    // as opposed to something like -0.5
    bool suborderingExact = (actualSubordering != 
                             (lrintf(floorf(m_targetSubordering - 0.01))));

    RG_DEBUG << "actualSubordering =" << actualSubordering
             << "suborderingExact =" << suborderingExact;

    Event *e;

    if (m_grace == GraceModeOff) {

        e = new Event
            (Note::EventType,
             m_insertionTime,
             m_note.getDuration(),
             0,
             m_insertionTime,
             m_note.getDuration());

    } else {

        segment.getTimeSlice(m_insertionTime, i, j);
        for (Segment::iterator k = i; k != j; ++k) {
            if ((*k)->isa(Indication::EventType) &&
                (*k)->getSubOrdering() <= actualSubordering) {
                // Decrement subordering to put the grace note
                // before the indication (i.e. slur), otherwise
                // it becomes part of the indication.
                actualSubordering = (*k)->getSubOrdering() - 1;
            }
        }
        e = new Event
            (Note::EventType,
             m_insertionTime,
             0,
             actualSubordering == 0 ? -1 : actualSubordering,
             m_insertionTime,
             m_note.getDuration());
    }

    e->set<Int>(PITCH, m_pitch);
    e->set<Int>(VELOCITY, m_velocity);

    if (m_accidental != Accidentals::NoAccidental) {
        e->set<String>(ACCIDENTAL, m_accidental);
    }

    if (m_noteStyle != NoteStyleFactory::DefaultStyle) {
        e->set<String>(NotationProperties::NOTE_STYLE,
                       qstrtostr(m_noteStyle));
    }

    if (m_grace != GraceModeOff) {

        if (!suborderingExact) {

            // Adjust suborderings of any existing grace notes, if there
            // is at least one with the same subordering and
            // suborderingExact is not set

            segment.getTimeSlice(m_insertionTime, i, j);
            bool collision = false;
            for (Segment::iterator k = i; k != j; ++k) {
                if ((*k)->getSubOrdering() == actualSubordering) {
                    collision = true;
                    break;
                }
            }
            
            if (collision) {
                std::vector<Event *> toInsert, toErase;
                for (Segment::iterator k = i; k != j; ++k) {
                    if ((*k)->isa(Note::EventType) &&
                        (*k)->getSubOrdering() <= actualSubordering) {
                        toErase.push_back(*k);
                        toInsert.push_back
                            (new Event(**k,
                                       (*k)->getAbsoluteTime(),
                                       (*k)->getDuration(),
                                       (*k)->getSubOrdering() - 1,
                                       (*k)->getNotationAbsoluteTime(),
                                       (*k)->getNotationDuration()));
                    }
                }
                for (std::vector<Event *>::iterator k = toErase.begin();
                     k != toErase.end(); ++k) segment.eraseSingle(*k);
                for (std::vector<Event *>::iterator k = toInsert.begin();
                     k != toInsert.end(); ++k) segment.insert(*k);
            }
        }

        e->set<Bool>(IS_GRACE_NOTE, true);
        i = segment.insert(e);

        Segment::iterator k;
        segment.getTimeSlice(m_insertionTime, j, k);
        Segment::iterator bg0 = segment.end(), bg1 = segment.end();
        while (j != k) {
            RG_DEBUG << "testing for truthiness: time " << (*j)->getAbsoluteTime() << ", subordering " << (*j)->getSubOrdering();
            if ((*j)->isa(Note::EventType) &&
                (*j)->getSubOrdering() < 0 &&
                (*j)->has(IS_GRACE_NOTE) &&
                (*j)->get<Bool>(IS_GRACE_NOTE)) {
                RG_DEBUG << "truthiful";
                if (bg0 == segment.end()) bg0 = j;
                bg1 = j;
            }
            ++j;
        }

        if (bg0 != segment.end() && bg1 != bg0) {
            if (bg1 != segment.end()) ++bg1;
            int count = 0;
            int pso = 0;
            for (Segment::iterator i = bg0; i != bg1; ++i) {
                (*i)->unset(BEAMED_GROUP_ID);
                (*i)->unset(BEAMED_GROUP_TYPE);
                (*i)->unset(BEAMED_GROUP_TUPLED_COUNT);
                (*i)->unset(BEAMED_GROUP_UNTUPLED_COUNT);
                if ((*i)->getSubOrdering() != pso) {
                    ++count;
                    pso = (*i)->getSubOrdering();
                }
            }
            if (m_grace == GraceAndTripletModesOn) {
                helper.makeBeamedGroupExact(bg0, bg1, GROUP_TYPE_TUPLED);
                if (count > 1) {
                    for (Segment::iterator i = bg0; i != bg1; ++i) {
                        (*i)->set<Int>(BEAMED_GROUP_TUPLED_COUNT, count-1);
                        (*i)->set<Int>(BEAMED_GROUP_UNTUPLED_COUNT, count);
                    }
                }
            } else {
                helper.makeBeamedGroupExact(bg0, bg1, GROUP_TYPE_BEAMED);
            }
        }
            
    } else {

        // If we're attempting to insert at the same time and pitch as
        // an existing note, then we remove the existing note first
        // (so as to change its duration, if the durations differ)
        segment.getTimeSlice(m_insertionTime, i, j);
        while (i != j) {
            if ((*i)->isa(Note::EventType)) {
                long pitch;
                if ((*i)->get<Int>(PITCH, pitch) && pitch == m_pitch) {
                    helper.deleteNote(*i);
                    break;
                }
            }
            ++i;
        }
        
        if (m_matrixType) {
            i = SegmentMatrixHelper(segment).insertNote(e);
        } else {
            i = helper.insertNote(e);
            // e is just a model for SegmentNotationHelper::insertNote
            delete e;
        }
    }

    if (i != segment.end()) m_lastInsertedEvent = *i;

    if (m_autoBeam) {

        // We auto-beam the bar if it contains no beamed groups
        // after the insertion point and if it contains no tupled
        // groups at all.

        timeT barStartTime = segment.getBarStartForTime(m_insertionTime);
        timeT barEndTime = segment.getBarEndForTime(m_insertionTime);

        for (Segment::iterator j = i;
                j != segment.end() && (*j)->getAbsoluteTime() < barEndTime;
                ++j) {
            if ((*j)->has(BEAMED_GROUP_ID))
                return ;
        }

        for (Segment::iterator j = i;
                j != segment.end() && (*j)->getAbsoluteTime() >= barStartTime;
                --j) {
            if ((*j)->has(BEAMED_GROUP_TUPLET_BASE))
                return ;
            if (j == segment.begin())
                break;
        }

        helper.autoBeam(m_insertionTime, m_insertionTime, GROUP_TYPE_BEAMED);
    }

    if (m_autoTieBarlines) {

        // Note: if m_lastInsertedEvent is null then no note was inserted
        if (m_lastInsertedEvent) {

            // Do the split
            Segment::iterator eventItr = segment.findSingle(m_lastInsertedEvent);
            
            if (eventItr != segment.end()) {
                m_lastInsertedEvent = helper.makeThisNoteViable(eventItr);
            }
        }
    }
}

}
