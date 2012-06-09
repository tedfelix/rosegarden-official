/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _INTERNALSEGMENTMAPPER_H_
#define _INTERNALSEGMENTMAPPER_H_

#include "base/ControllerContext.h"
#include "gui/seqmanager/MappedEventBuffer.h"
#include "gui/seqmanager/SegmentMapper.h"
#include "gui/seqmanager/ChannelManager.h"

#include <set>

namespace Rosegarden
{

class TriggerSegmentRec;
class Composition;
 
// @class InternalSegmentMapper Mapper for an internal segment.
// @author Tom Breton (Tehom)
class InternalSegmentMapper : public SegmentMapper
{
    friend class SegmentMapperFactory;
    friend class ControllerSearch;
    friend class ControllerContextMap;

    class Callback : public ChannelManager::MapperFunctionality
        {
        public:
    
        Callback(InternalSegmentMapper *mapper) :
            m_mapper(mapper) {}
        private:
            virtual ControllerAndPBList
                getControllers(Instrument *instrument, RealTime start);
            InternalSegmentMapper *m_mapper;
        };

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

    InternalSegmentMapper(RosegardenDocument *doc, Segment *segment);
    ~InternalSegmentMapper(void);

    // Do channel-setup
    virtual void doInsert(MappedInserterBase &inserter, MappedEvent &evt,
                         RealTime start, bool firstOutput);

    virtual int calculateSize();

    int addSize(int size, Segment *);

    /// dump all segment data in the file
    virtual void dump();

    Instrument *getInstrument(void)
    { return m_channelManager.m_instrument; }

    void mergeTriggerSegment(Segment *target,
                             Event *trigger,
                             timeT performanceDuration,
                             TriggerSegmentRec *rec);

    void popInsertNoteoff(int trackid, Composition &comp);
    void enqueueNoteoff(timeT time, int pitch);

    bool haveEarlierNoteoff(timeT t);

    /** Data members **/

    IntervalChannelManager m_channelManager;

    // Separate storage for triggered events.  This storage, like
    // original segment, contains just one time thru; logic in "dump"
    // turns it into repeats as needed.
    Segment               *m_triggeredEvents;
    
    ControllerContextMap   m_controllerCache;

    // Queue of noteoffs.
    NoteoffContainer       m_noteOffs;
};
  
}

#endif /* ifndef _INTERNALSEGMENTMAPPER_H_ */
