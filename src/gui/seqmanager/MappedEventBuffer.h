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

#ifndef _MAPPEDSEGMENT_H_
#define _MAPPEDSEGMENT_H_

#include "base/RealTime.h"
#include <QReadWriteLock>
#include <QAtomicInt>

namespace Rosegarden
{

class MappedEvent;
class MappedInserterBase;
class RosegardenDocument;

/**
 * MappedEventBuffer is the container class for sound-making events
 * that have been mapped linearly into memory for ease of reading by
 * the sequencer code (after things like tempo mappings, repeats etc
 * have been mapped and unfolded).
 *
 * The mapping logic is handled by mappers derived from this class; this
 * class provides the basic container and the reading logic.  Reading
 * and writing may take place simultaneously without locks (we are
 * prepared to accept the lossage from individual mangled MappedEvent
 * reads) but a read/write lock on the buffer space guards against bad
 * access caused by resizes.
 *
 * MappedEventBuffer only concerns itself with the state of the
 * composition, as opposed to the state of performance.  No matter how
 * the user jumps around in time, it shouldn't change.  For the
 * current state of performance, see MappedEventBuffer::iterator.
 *
 * A MappedEventBuffer is jointly owned by one or more metaiterators
 * and by ChannelManager and deletes itself when the last owner is
 * removed.
 *
 */
class MappedEventBuffer
{
public:
    MappedEventBuffer(RosegardenDocument *);
    // Some derived classes need to release channels, so dtor is
    // virtual.
    virtual ~MappedEventBuffer();

    bool isMetronome() const { return m_isMetronome; }
    void setMetronome(bool isMetronome) { m_isMetronome = isMetronome; }

    MappedEvent *getBuffer() { return m_buffer; } // un-locked, use only from write/resize thread

    int getBufferSize() const; // in mapped events
    int getBufferFill() const; // in mapped events

    void resizeBuffer(int newSize); // ignored if smaller than old size
    void setBufferFill(int newFill); // must be no bigger than buffer size

    /**
     * refresh the object after the segment has been modified
     * returns true if size changed (and thus the sequencer
     * needs to be told about it)
     */
    bool refresh();

    /* Virtual functions */

    virtual int getSegmentRepeatCount()=0;

    virtual int calculateSize()=0; // in MappedEvents

    /// actual setup, must be called after ctor, calls virtual methods
    void init();

    // Initialization particular to various classes.  In fact it just
    // debug-prints. 
    virtual void initSpecial(void) {};
    
    /// dump all segment data into m_buffer
    virtual void dump()=0;

    // Insert a MappedEvent with appropriate setup for channel.
    // refTime is not neccessarily the same as MappedEvent's
    // getEventTime() because we might jump into the middle of a long
    // note.
    virtual void
        doInsert(MappedInserterBase &inserter, MappedEvent &evt,
                 RealTime refTime, bool firstOutput);

    // Record one more owner of this mapper, so we don't delete it
    // while it's owned.
    void addOwner(void);
    // Record one fewer owner.
    void removeOwner(void);

    // Get the earliest and latest sounding times.
    void getStartEnd(RealTime &start, RealTime &end) {
        start = m_start;
        end   = m_end;
    }

 protected:
    void mapAnEvent(MappedEvent *e);

    // Set the sounding times.
    void setStartEnd(RealTime &start, RealTime &end) {
        m_start = start;
        m_end   = end;
    }
 public:
    class iterator 
    {
    public:
        iterator(MappedEventBuffer* s);
        ~iterator(void) { m_s->removeOwner(); };
        iterator& operator=(const iterator&);
        bool operator==(const iterator&);
        bool operator!=(const iterator& it) { return !operator==(it); }

        bool atEnd() const;

        /// go back to beginning of stream
        void reset();

        iterator& operator++();
        iterator  operator++(int);
        iterator& operator+=(int);
        iterator& operator-=(int);

        MappedEvent operator*();  // returns MappedEvent() if atEnd()
        MappedEvent *peek() const; // returns 0 if atEnd()

        MappedEventBuffer *getSegment() { return m_s; }
        const MappedEventBuffer *getSegment() const { return m_s; }

        void setActive(bool value, RealTime currentTime) {
            m_active = value;
            m_currentTime = currentTime;
        }
        void setInactive(void) { m_active = false; }
        bool getActive(void) { return m_active; }
        void setReady (bool value) { m_ready  = value; };
        bool getReady(void)  { return m_ready; }

        // Do appropriate preparation for inserting event, including
        // possibly setting up the channel.
        void doInsert(MappedInserterBase &inserter, MappedEvent &evt);

    private:
         iterator();

    protected:
        MappedEventBuffer *m_s;
        int m_index;

        // Whether we are ready with regard to performance time.  We
        // always are except when starting or jumping in time.  
        // Making us ready is derived classes' job via "doInsert",
        bool m_ready;
        // Whether this iterator has more events to give within the
        // current time slice.
        bool m_active;

        // RealTime when the current event starts sounding.  Either
        // the current event's time or the time the loop starts,
        // whichever is greater.  Used for calculating the correct controllers
        RealTime  m_currentTime;
    };

protected:
    friend class iterator;

    mutable QAtomicInt m_size;
    mutable QAtomicInt m_fill;
    MappedEvent *m_buffer;
    bool m_isMetronome;

    QReadWriteLock m_lock;
    RosegardenDocument *m_doc;

    // Earliest and latest sounding times.  It is the responsibility
    // of "dump" to keep these fields up to date.
    RealTime m_start;
    RealTime m_end;

    // How many metaiterators share this mapper.  We won't delete
    // while it has any owner.  This is changed just in owner's ctors
    // and dtors.
    int m_refCount;
};

}

#endif /* ifndef _MAPPEDSEGMENT_H_ */
