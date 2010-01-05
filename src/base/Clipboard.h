/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2010 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _CLIPBOARD_H_
#define _CLIPBOARD_H_

#include <set>
#include "base/Segment.h"
#include "Selection.h"

namespace Rosegarden
{
class EventSelection;

/**
 * Simple container for segments, that can serve as a clipboard for
 * editing operations.  Conceptually it has two "modes",
 * single-segment and multiple-segment, although there's no particular
 * distinction behind the scenes.  The Clipboard owns all the segments
 * it contains -- they should always be deep copies, not aliases.
 */

class Clipboard
{
public:
    typedef std::multiset<Segment *, Segment::SegmentCmp> segmentcontainer;
    typedef segmentcontainer::iterator iterator;
    typedef segmentcontainer::const_iterator const_iterator;

    Clipboard();
    Clipboard(const Clipboard &);
    Clipboard &operator=(const Clipboard &);
    virtual ~Clipboard();

    /**
     * Empty the clipboard.
     */
    void clear();
 
    /**
     * Return true if the clipboard is empty.
     */
    bool isEmpty() const;

    iterator       begin()       { return m_segments.begin(); }
    const_iterator begin() const { return m_segments.begin(); }
    iterator       end()         { return m_segments.end(); }
    const_iterator end() const   { return m_segments.end(); }
    
    /**
     * Return true if the clipboard only contains a single segment.
     * Single-segment and multi-segment are conceptually rather
     * separate -- for example, you can only paste into a segment
     * from a single-segment clipboard.
     */
    bool isSingleSegment() const;

    /**
     * Return true if the clipboard contains at least one segment
     * that originated as only part of another segment.  If a
     * paste is made from a clipboard with isPartial true, the
     * paste command will generally want to be sure to normalize
     * rests etc on the pasted region afterwards.
     */
    bool isPartial() const;

    /**
     * Return the single segment contained by the clipboard.
     * If the clipboard is empty or contains more than one segment,
     * returns null.  (Use the iterator accessors begin()/end() to
     * read from a clipboard for which isSingleSegment is false.)
     */
    Segment *getSingleSegment() const;

    /**
     * Add a new empty segment to the clipboard, and return a
     * pointer to it.  (The clipboard retains ownership.)
     */
    Segment *newSegment();

    /**
     * Add a new segment to the clipboard, containing copies of
     * the events in copyFrom.  (The clipboard retains ownership
     * of the new segment.)
     */
    Segment *newSegment(const Segment *copyFrom);

    /**
     * Add one or more new segments to the clipboard, containing
     * copies of the events in copyFrom found between from and to.  If
     * expandRepeats is true, include any events found in the
     * repeating trail of the segment within this time.  (The
     * clipboard retains ownership of the new segment(s).)
     *
     * This may insert more than one new segment, if it is required to
     * insert a repeating section of an audio segment.  For this
     * reason it does not return the inserted segment (even though in
     * most situations it will only insert one).
     */
    void newSegment(const Segment *copyFrom, timeT from, timeT to,
                    bool expandRepeats);

    /**
     * Add a new segment to the clipboard, containing copied of
     * the events in the given selection.
     */
    Segment *newSegment(const EventSelection *copyFrom);

    /**
     * Add a time signature selection to this clipboard, replacing any
     * that already exists.
     */
    void setTimeSignatureSelection(const TimeSignatureSelection &);

    bool hasTimeSignatureSelection() const { return m_haveTimeSigSelection; }

    /**
     * Remove any time signature selection from the clipboard.
     */
    void clearTimeSignatureSelection();

    /**
     * Retrieve any time signature selection found in the clipboard.
     */
    const TimeSignatureSelection &getTimeSignatureSelection() const;

    /**
     * Add a tempo selection to this clipboard, replacing any
     * that already exists.
     */
    void setTempoSelection(const TempoSelection &);

    bool hasTempoSelection() const { return m_haveTempoSelection; }

    /**
     * Remove any tempo selection from the clipboard.
     */
    void clearTempoSelection();

    /**
     * Retrieve any tempo selection found in the clipboard.
     */
    const TempoSelection &getTempoSelection() const;

    /**
     * Clear the current clipboard and re-fill it by copying from c.
     */
    void copyFrom(const Clipboard *c);

    /**
     * Get the earliest start time for anything in this clipboard,
     * or the start of the nominal range if there is one.
     */
    timeT getBaseTime() const;

    /**
     * Set nominal start and end times for the range in the clipboard,
     * if it is intended to cover a particular time range regardless
     * of whether the data in it covers the full range or not.
     */
    void setNominalRange(timeT start, timeT end);

    void clearNominalRange() { setNominalRange(0, 0); }

    bool hasNominalRange() const { return m_nominalStart != m_nominalEnd; }
    
    void getNominalRange(timeT &start, timeT &end);

private:
    segmentcontainer m_segments;
    bool m_partial;

    TimeSignatureSelection m_timeSigSelection;
    bool m_haveTimeSigSelection;

    TempoSelection m_tempoSelection;
    bool m_haveTempoSelection;

    timeT m_nominalStart;
    timeT m_nominalEnd;
};

}

#endif
