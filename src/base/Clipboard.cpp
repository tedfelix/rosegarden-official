/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Clipboard.h"
#include "Selection.h"
#include "misc/Debug.h"

namespace Rosegarden
{

Clipboard::Clipboard() :
    m_partial(false),
    m_haveTimeSigSelection(false),
    m_haveTempoSelection(false),
    m_nominalStart(0),
    m_nominalEnd(0)
{
    // nothing
}

Clipboard::Clipboard(const Clipboard &c) :
    m_partial(false)
{
    copyFrom(&c);
}

Clipboard &
Clipboard::operator=(const Clipboard &c)
{
    copyFrom(&c);
    return *this;
}

Clipboard::~Clipboard()
{
    clear();
}

void
Clipboard::clear()
{
    for (iterator i = begin(); i != end(); ++i) {
        delete *i;
    }
    m_segments.clear();
    clearTimeSignatureSelection();
    clearTempoSelection();
    clearNominalRange();
    m_partial = false;
}

bool
Clipboard::isEmpty() const
{
    return (m_segments.empty() &&
        !m_haveTimeSigSelection &&
        !m_haveTempoSelection &&
        m_nominalStart == m_nominalEnd);
}

bool
Clipboard::isSingleSegment() const
{
    return (m_segments.size() == 1 &&
        !m_haveTimeSigSelection &&
        !m_haveTempoSelection);
}

Segment *
Clipboard::getSingleSegment() const
{
    if (isSingleSegment()) return *begin();
    else return 0;
}

bool
Clipboard::isPartial() const
{
    return m_partial;
}

Segment *
Clipboard::newSegment()
{
    Segment *s = new Segment();
    m_segments.insert(s);
    // don't change m_partial as we are inserting a complete segment
    return s;
}

Segment *
Clipboard::newSegment(const Segment *copyFrom, bool deep)
{
    Segment *s = copyFrom->clone(deep);
    m_segments.insert(s);
    // don't change m_partial as we are inserting a complete segment
    return s;
}

void
Clipboard::newSegment(const Segment *copyFrom, timeT from, timeT to,
    bool expandRepeats)
{
    // create with copy ctor so as to inherit track, instrument etc
    Segment *s = copyFrom->clone(false);

    // If the segment is within the time range
    if (from <= s->getStartTime() && to >= s->getEndMarkerTime()) {
        // Insert the whole thing.
        m_segments.insert(s);

        // don't change m_partial as we are inserting a complete segment

        return;
    }

    // Only a portion of the source segment will be used.

    const timeT segStart = copyFrom->getStartTime();
    const timeT segEndMarker = copyFrom->getEndMarkerTime();
    timeT segDuration = segEndMarker - segStart;
    
    // We can't copy starting prior to the start of the segment.
    if (from < segStart)
        from = segStart;

    int firstRepeat = 0;
    int lastRepeat = 0;

    if (!copyFrom->isRepeating() || segDuration <= 0) {
        expandRepeats = false;
    }

    if (expandRepeats) {
        firstRepeat = (from - segStart) / segDuration;
        to = std::min(to, copyFrom->getRepeatEndTime());
        lastRepeat = (to - segStart) / segDuration;
    }

    s->setRepeating(false);
    
    if (s->getType() == Segment::Audio) {

        Composition *c = copyFrom->getComposition();

        for (int repeat = firstRepeat; repeat <= lastRepeat; ++repeat) {

            timeT wrappedFrom = segStart;
            timeT wrappedTo = segEndMarker;

            if (!expandRepeats) {
                wrappedFrom = from;
                wrappedTo = to;
            } else {
                if (repeat == firstRepeat) {
                    wrappedFrom = segStart + (from - segStart) % segDuration;
                }
                if (repeat == lastRepeat) {
                    wrappedTo = segStart + (to - segStart) % segDuration;
                }
            }

            if (wrappedFrom > segStart) {
                if (c) {
                    s->setAudioStartTime
                        (s->getAudioStartTime() +
                         c->getRealTimeDifference(segStart + repeat * segDuration,
                                                  from));
                }
                s->setStartTime(from);
            } else {
                s->setStartTime(segStart + repeat * segDuration);
            }

            if (wrappedTo < segEndMarker) {
                s->setEndMarkerTime(to);
                if (c) {
                    s->setAudioEndTime
                        (s->getAudioStartTime() +
                         c->getRealTimeDifference(segStart + repeat * segDuration,
                                                  to));
                }
            } else {
                s->setEndMarkerTime(segStart + (repeat + 1) * segDuration);
            }

            m_segments.insert(s);
            if (repeat < lastRepeat) {
                s = copyFrom->clone(false);
                s->setRepeating(false);
            }
        }

        m_partial = true;
        return;
    }

    // We have a normal (MIDI) segment.

    s->erase(s->begin(), s->end());
    
    for (int repeat = firstRepeat; repeat <= lastRepeat; ++repeat) {

        Segment::const_iterator ifrom = copyFrom->begin();
        Segment::const_iterator ito = copyFrom->end();

        if (!expandRepeats) {
            ifrom = copyFrom->findTime(from);
            ito = copyFrom->findTime(to);
        } else {
            if (repeat == firstRepeat) {
                ifrom = copyFrom->findTime
                    (segStart + (from - segStart) % segDuration);
            }
            if (repeat == lastRepeat) {
                ito = copyFrom->findTime
                    (segStart + (to - segStart) % segDuration);
            }
        }

        // For each event in the time range and before the end marker.
        for (Segment::const_iterator i = ifrom;
             i != ito && copyFrom->isBeforeEndMarker(i); ++i) {

            Event *e = (*i)->copyMoving(repeat * segDuration);

            s->insert(e);
        }
    }

    if (expandRepeats)
        s->setEndMarkerTime(to);

    // Make sure the end of the segment doesn't go past the end of the range.
    // Need to use the end marker time from the original segment, not s, 
    // because its value may depend on the composition it's in.
    if (segEndMarker > to)
        s->setEndMarkerTime(to);

    // Fix the beginning.
    
    timeT firstEventTime = s->getStartTime();
    
    // if the beginning was chopped off and the first event isn't at the start
    if (from > segStart  &&  firstEventTime > from) {
        // Expand the beginning to the left so that it starts at the expected
        // time (from).
        s->fillWithRests(from, firstEventTime);
    }

    // Fix zero-length segments.
    
    // if s is zero length
    if (s->getStartTime() == s->getEndMarkerTime()) {
        // Figure out what start and end time would look right.
        timeT finalStartTime = ((segStart > from) ? segStart : from);
        timeT finalEndTime = ((segEndMarker < to) ? segEndMarker : to);
        // Fill it up so it appears.
        s->fillWithRests(finalStartTime, finalEndTime);
    }

    m_segments.insert(s);

    m_partial = true;
}

Segment *
Clipboard::newSegment(const EventSelection *copyFrom)
{
    // create with clone function so as to inherit track, instrument etc
    // but clone as a segment only, even if it's actually a linked segment
    Segment *s = copyFrom->getSegment().clone(false);
    s->erase(s->begin(), s->end());

    const EventSelection::eventcontainer &events(copyFrom->getSegmentEvents());
    for (EventSelection::eventcontainer::const_iterator i = events.begin();
         i != events.end(); ++i) {
        s->insert(new Event(**i));
    }

    m_segments.insert(s);
    m_partial = true;
    return s;
}

void
Clipboard::setTimeSignatureSelection(const TimeSignatureSelection &ts)
{
    m_timeSigSelection = ts;
    m_haveTimeSigSelection = true;
}

void
Clipboard::clearTimeSignatureSelection()
{
    m_timeSigSelection = TimeSignatureSelection();
    m_haveTimeSigSelection = false;
}

const TimeSignatureSelection &
Clipboard::getTimeSignatureSelection() const
{
    return m_timeSigSelection;
}
 
void
Clipboard::setTempoSelection(const TempoSelection &ts)
{
    m_tempoSelection = ts;
    m_haveTempoSelection = true;
}

void
Clipboard::clearTempoSelection()
{
    m_tempoSelection = TempoSelection();
    m_haveTempoSelection = false;
}

const TempoSelection &
Clipboard::getTempoSelection() const
{
    return m_tempoSelection;
}
    
void
Clipboard::copyFrom(const Clipboard *c)
{
    if (c == this) return;
    clear();

    for (Clipboard::const_iterator i = c->begin(); i != c->end(); ++i) {
        newSegment(*i);
    }

    m_partial = c->m_partial;

    m_timeSigSelection = c->m_timeSigSelection;
    m_haveTimeSigSelection = c->m_haveTimeSigSelection;

    m_tempoSelection = c->m_tempoSelection;
    m_haveTempoSelection = c->m_haveTempoSelection;
    
    m_nominalStart = c->m_nominalStart;
    m_nominalEnd = c->m_nominalEnd;
}

timeT
Clipboard::getBaseTime() const
{
    if (hasNominalRange()) {
        return m_nominalStart;
    }

    timeT t = 0;

    for (const_iterator i = begin(); i != end(); ++i) {
        if (i == begin() || (*i)->getStartTime() < t) {
            t = (*i)->getStartTime();
        }
    }

    if (m_haveTimeSigSelection && !m_timeSigSelection.empty()) {
        if (m_timeSigSelection.begin()->first < t) {
            t = m_timeSigSelection.begin()->first;
        }
    }

    if (m_haveTempoSelection && !m_tempoSelection.empty()) {
        if (m_tempoSelection.begin()->first < t) {
            t = m_tempoSelection.begin()->first;
        }
    }
    
    return t;
}

void
Clipboard::setNominalRange(timeT start, timeT end)
{
    m_nominalStart = start;
    m_nominalEnd = end;
}

void
Clipboard::getNominalRange(timeT &start, timeT &end)
{
    start = m_nominalStart;
    end = m_nominalEnd;
}

void
Clipboard::removeAudioSegments()
{
    iterator i = begin();

    // For each segment
    while (i != end()) {
        // Make sure we don't erase what the iterator is pointing to.
        iterator j = i;
        ++i;

        // If this is an audio segment, erase it.
        if ((*j)->getType() == Segment::Audio) {
            m_segments.erase(j);
        }
    }
    
    // If there are no segments, clear the clipboard.
    if (m_segments.empty())
        clear();
}

Clipboard *Clipboard::mainClipboard()
{
    static Clipboard s_mainClipboard;
    return &s_mainClipboard;
}

}
