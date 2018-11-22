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

#ifndef RG_INTERNALSEGMENTMAPPER_H
#define RG_INTERNALSEGMENTMAPPER_H

#include "base/ControllerContext.h"
#include "gui/seqmanager/MappedEventBuffer.h"
#include "gui/seqmanager/SegmentMapper.h"
#include "gui/seqmanager/ChannelManager.h"

#include <set>

namespace Rosegarden
{

class TriggerSegmentRec;
class Composition;
class RealTime;
 
/// Converts (maps) Event objects into MappedEvent objects for a Segment
/**
 * @author Tom Breton (Tehom)
 *
 * InternalSegmentMapper.  What's in a name?  "Internal" qualifies a
 * Segment as containing Rosegarden's internal representation of MIDI
 * notes etc, as opposed to an audio segment.  The term "Mapper" is
 * used largely for historical reasons, but can be understood as
 * mapping (converting) Event objects from a Segment into MappedEvent
 * objects which are ready for playback.  The key mapping routine is
 * InternalSegmentMapper::fillBuffer().
 *
 * This class might be better named MappedMIDISegment as that is what it is
 * rather than what it does.  It's a more "O-O" name, which in this case feels
 * better.  Of course, the base class and other related classes would need
 * to adopt similar names.  The entire MappedEventBuffer hierarchy probably
 * needs to be examined to see if there are opportunities for simplification
 * and renaming for increased clarity.
 *
 * This is the first part of a two-part process to convert the Event objects
 * in a Composition into MappedEvent objects that can be sent to ALSA.  For
 * the second part of this conversion, see MappedBufMetaIterator.
 */
class InternalSegmentMapper : public SegmentMapper
{
public:
    InternalSegmentMapper(RosegardenDocument *doc, Segment *segment);
    ~InternalSegmentMapper() override;

private:
    // Hide copy ctor and op= since dtor is non-trivial.
    InternalSegmentMapper(const InternalSegmentMapper &);
    InternalSegmentMapper operator=(const InternalSegmentMapper &);

    friend class ControllerSearch;
    friend class ControllerContextMap;

    /// Get CCs and PitchBend at a time for this Segment.
    /**
     * ??? This returns a copy.  Consider taking in a reference instead to
     *     avoid the copy.
     */
    ControllerAndPBList getControllers(Instrument *instrument, RealTime start);

    typedef std::pair<timeT, int> Noteoff;
    struct NoteoffCmp
    {
        typedef InternalSegmentMapper::Noteoff Noteoff;
        bool operator()(const Noteoff &e1, const Noteoff &e2) const {
            return e1.first < e2.first;
        }
        bool operator()(const Noteoff *e1, const Noteoff *e2) const {
            return operator()(*e1, *e2);
        }
    };
    // This wants to be a priority_queue but we need to sometimes
    // filter noteoffs so it can't be.
    typedef std::multiset<Noteoff, NoteoffCmp>
        NoteoffContainer;

    // Do channel-setup for Auto channel mode.
    void makeReady(MappedInserterBase &inserter, RealTime time) override;

    // Do channel-setup for Fixed channel mode.
    void insertChannelSetup(MappedInserterBase &) override;

    // Insert the event "evt"
    void doInsert(MappedInserterBase &inserter, MappedEvent &evt,
                         RealTime start, bool firstOutput) override;

    // Return whether the event should be played.
    bool shouldPlay(MappedEvent *evt, RealTime startTime) override;

    int calculateSize() override;

    int addSize(int size, Segment *);

    /// dump all segment data in the file
    void fillBuffer() override;

    Instrument *getInstrument() const
    { return m_channelManager.getInstrument(); }

    void popInsertNoteoff(int trackid, Composition &comp);
    void enqueueNoteoff(timeT time, int pitch);

    bool haveEarlierNoteoff(timeT t);
    RealTime toRealTime(Composition &comp, timeT t);
    int getControllerValue(timeT searchTime,
                           const std::string eventType,
                           int controllerId);

    /** Data members **/

    ChannelManager m_channelManager;

    // Separate storage for triggered events.  This storage, like
    // original segment, contains just one time thru; logic in "fillBuffer"
    // turns it into repeats as needed.
    Segment               *m_triggeredEvents;
    
    ControllerContextMap   m_controllerCache;

    // Queue of noteoffs.
    NoteoffContainer       m_noteOffs;
};
  
}

#endif /* ifndef RG_INTERNALSEGMENTMAPPER_H */
