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

#ifndef RG_MAPPEDEVENTBUFFER_H
#define RG_MAPPEDEVENTBUFFER_H

#include "base/RealTime.h"
#include <QReadWriteLock>
#include <QAtomicInt>

namespace Rosegarden
{

class MappedEvent;
class MappedInserterBase;
class RosegardenDocument;

/// Abstract Base Class container for MappedEvent objects.
/**
 * MappedEventBuffer is an Abstract Base Class.  Consequently,
 * MappedEventBuffer objects are never created.  See the three
 * derived classes: MetronomeMapper, SegmentMapper, and
 * SpecialSegmentMapper.
 *
 * MappedEventBuffer is the container class for sound-making events
 * that have been mapped linearly into memory for ease of reading by
 * the sequencer code (after things like tempo mappings, repeats etc
 * have been mapped and unfolded).
 *
 * The mapping logic is handled by mappers derived from this class; this
 * class provides the basic container and the reading logic.
 *
 * Reading and writing may take place simultaneously without locks (we are
 * prepared to accept the lossage from individual mangled MappedEvent
 * reads) but a read/write lock (m_lock) on the buffer space guards
 * against bad
 * access caused by resizes.  [Actually, it doesn't at all since the
 * caller of peek() has a pointer they can access after the lock has
 * been released.  See comments on m_lock below and in iterator::peek().]
 *
 * MappedEventBuffer only concerns itself with the state of the
 * composition, as opposed to the state of performance.  No matter how
 * the user jumps around in time, it shouldn't change.  For the
 * current state of performance, see MappedEventBuffer::iterator.
 *
 * A MappedEventBuffer-derived object is jointly owned by one or more
 * metaiterators (MappedBufMetaIterator?) and by ChannelManager and deletes
 * itself when the last owner is removed.  See addOwner() and removeOwner().
 */
class MappedEventBuffer
{
public:
    MappedEventBuffer(RosegardenDocument *);
    virtual ~MappedEventBuffer();

    /// Two-phase initialization.
    /**
     * Actual setup, must be called after ctor, calls virtual methods.
     * Dynamic Binding During Initialization idiom.
     */
    void init();

    /// Initialization particular to various classes.  (UNUSED)
    /**
     * The only overrider is SegmentMapper and it just debug-prints.
     */
    virtual void initSpecial(void)  { }

    /// Access to the internal buffer of events.  NOT LOCKED
    /**
     * un-locked, use only from write/resize thread
     */
    MappedEvent *getBuffer() { return m_buffer; }

    /// Capacity of the buffer in MappedEvent's.  (STL's capacity().)
    /**
     * The use of "size" and "fill" in this class is in conflict with STL
     * terminology.  Recommend changing "size" to "capacity" and "fill"
     * to "size" in keeping with the STL.  So this routine would be
     * renamed to "capacity()".
     *
     * rename: capacity() (per STL)
     */
    int getBufferSize() const;
    /// Number of MappedEvent's in the buffer.  (STL's size().)
    /**
     * rename: size() (per STL)
     */
    int getBufferFill() const;

    /// Sets the buffer capacity.
    /**
     * Ignored if smaller than old capacity.
     *
     * @see getBufferSize()
     *
     * rename: reserve() (per STL)
     */
    void resizeBuffer(int newSize);

    /// Sets the number of events in the buffer.
    /**
     * Must be no bigger than buffer capacity.
     *
     * @see getBufferFill()
     *
     * rename: resize() (per STL)
     */
    void setBufferFill(int newFill);

    /// Refresh the buffer
    /**
     * Called after the segment has been modified.  Resizes the buffer if
     * needed, then calls dump() to fill it from the segment.
     *
     * Returns true if buffer size changed (and thus the sequencer
     * needs to be told about it).
     */
    bool refresh();

    /* Virtual functions */

    /**
     * Only SegmentMapper::getSegmentRepeatCount() does something interesting,
     * the other derivers just return 1.
     */
    virtual int getSegmentRepeatCount() = 0;

    /// Calculates the required capacity based on the current document.
    /**
     * Overridden by each deriver to compute the number of MappedEvent's
     * needed to hold the contents of the current segment.
     *
     * Used by init() and refresh() to adjust the buffer capacity to be
     * big enough to hold all the events in the current segment.
     *
     * @see m_doc
     * @see resizeBuffer()
     */
    virtual int calculateSize() = 0;

    /// Fill buffer with data from the segment
    /**
     * This is provided by derivers to handle whatever sort of data is
     * specific to each of them.  E.g. InternalSegmentMapper::dump()
     * processes note events while TempoSegmentMapper processes tempo
     * change events.
     *
     * rename: fillBuffer()  (or just fill())
     */
    virtual void dump() = 0;

    /// Insert a MappedEvent with appropriate setup for channel.
    /**
     * refTime is not necessarily the same as MappedEvent's
     * getEventTime() because we might jump into the middle of a long
     * note.
     *
     * Only used by MappedEventBuffer::iterator::doInsert().
     */
    virtual void doInsert(MappedInserterBase &inserter, MappedEvent &evt,
                          RealTime refTime, bool firstOutput);

    /// Record one more owner of this mapper.
    /**
     * This increases the reference count to prevent deletion of a mapper
     * that is still owned.
     *
     * Called by:
     *   - MappedEventBuffer::iterator's ctor
     *   - CompositionMapper::mapSegment()
     *   - SequenceManager::resetMetronomeMapper()
     *   - SequenceManager::resetTempoSegmentMapper()
     *   - SequenceManager::resetTimeSigSegmentMapper()
     *
     * @see removeOwner()
     */
    void addOwner(void);

    /// Record one fewer owner of this mapper.
    /**
     * When the owner count reaches 0, this object will destroy itself
     * with a "delete this".
     *
     * @see addOwner()
     */
    void removeOwner(void);

    /// Get the earliest and latest sounding times.
    /**
     * Called by MappedBufMetaIterator::fillCompositionWithEventsUntil() and
     * MappedBufMetaIterator::fillNoncompeting().
     *
     * @see setStartEnd()
     */
    void getStartEnd(RealTime &start, RealTime &end) const {
        start = m_start;
        end   = m_end;
    }

    /// Is this object a MetronomeMapper?
    /**
     * Used by MappedBufMetaIterator::fillNoncompeting() for special
     * handling of the metronome.
     *
     * This might be implemented as a virtual function that just returns
     * false, but is overridden in MetronomeMapper to return true.
     * setMetronome() and m_isMetronome could then be removed.
     * Switching on type like this is usually considered a Bad Thing.
     * Examination of MappedBufMetaIterator might lead to a more useful
     * function that could be overridden by MetronomeMapper.
     *
     * @see setMetronome()
     */
    bool isMetronome() const { return m_isMetronome; }

    /// Enables special handling related to MetronomeMapper
    /**
     * Used by MetronomeMapper to communicate with
     * MappedBufMetaIterator::fillNoncompeting().
     *
     * @see isMetronome()
     */
    void setMetronome(bool isMetronome) { m_isMetronome = isMetronome; }

    class iterator 
    {
    public:
        iterator(MappedEventBuffer* s);

        /// Destructor.
        /**
         * Since this destructor is "non-trivial", we are required to provide
         * (or hide) the other two of the "big three".  I.e. the copy ctor
         * and op=.
         */
        ~iterator(void)  { m_s->removeOwner(); }

        /// Copy ctor.  (UNTESTED)
        /**
         * Copy ctors are not for the faint of heart, and should be avoided
         * whenever possible.  In this case, the postfix op++ needs the copy
         * ctor in its first line.  We could probably get rid of the postfix
         * op++ and force users to use the prefix op++ which would then let
         * us remove this copy ctor.
         *
         * UNTESTED.  Use carefully.
         */
        iterator(const iterator&);

        /// Assignment operator.  (UNTESTED)
        /**
         * Turns out this is never used.  However, since there's a copy ctor
         * and the non-trivial dtor that rounds out the "big three", we might
         * as well be complete and correct.
         *
         * UNTESTED.  Use carefully.
         */
        iterator& operator=(const iterator&);  // never used

        /// Equality
        /**
         * Only checks m_s (the iterator's MappedEventBuffer) and m_index.
         */
        bool operator==(const iterator&);
        bool operator!=(const iterator& it) { return !operator==(it); }

        bool atEnd() const;

        /// Go back to the beginning of the stream
        void reset();

        /// Prefix operator++
        iterator& operator++();

        /// Postfix operator++  (UNTESTED)
        /**
         * This is never actually used anywhere.  But, unfortunately it
         * triggers the need for a copy ctor.  It would probably be best to
         * get rid of this and force all clients to use the prefix
         * operator++.  But then someone might try to implement this and do
         * it incorrectly.
         *
         * UNTESTED.  Use carefully.
         */
        iterator  operator++(int);

        iterator& operator+=(int);
        iterator& operator-=(int);

        /// Dereference operator
        /**
         * Allows an expression like (*i) to give access to the element an
         * iterator points to.
         *
         * Returns a default constructed MappedEvent if atEnd().
         *
         * @see peek()
         */
        MappedEvent operator*();

        /// Dereference function
        /**
         * Returns a pointer to the MappedEvent the iterator is currently
         * pointing to.
         *
         * Returns 0 if atEnd().
         *
         * @see operator*()
         */
        MappedEvent *peek() const;

        /// Access to the segment the iterator is connected to.
        MappedEventBuffer *getSegment() { return m_s; }
        /// Access to the segment the iterator is connected to.
        const MappedEventBuffer *getSegment() const { return m_s; }

        /**
         * Called by MappedBufMetaIterator::fillNoncompeting().
         *
         * @see setInactive()
         * @see getActive()
         */
        void setActive(bool value, RealTime currentTime) {
            m_active = value;
            m_currentTime = currentTime;
        }
        /**
         * Called by MappedBufMetaIterator::fillNoncompeting().
         *
         * @see setActive()
         * @see getActive()
         */
        void setInactive()  { m_active = false; }
        /**
         * Called by MappedBufMetaIterator::fillNoncompeting().
         *
         * @see setActive()
         * @see setInactive()
         * @see m_active
         */
        bool getActive() const  { return m_active; }

        /**
         * Set to true by doInsert().  Set to false by
         * MappedBufMetaIterator::moveIteratorToTime() and
         * MappedBufMetaIterator::resetIteratorForSegment().
         *
         * @see getReady()
         */
        void setReady(bool value)  { m_ready = value; };

        /**
         * Called by MappedEventBuffer::iterator::doInsert().
         *
         * @see m_ready
         */
        bool getReady() const  { return m_ready; }

        /**
         * Delegates to MappedEventBuffer::doInsert().
         *
         * The old comments seem misleading.  It doesn't do appear to do any
         * of this:
         * "Do appropriate preparation for inserting event, including
         * possibly setting up the channel."
         */
        void doInsert(MappedInserterBase &inserter, MappedEvent &evt);

    protected:
        /// The buffer this iterator points into.
        MappedEventBuffer *m_s;

        /// Position of the iterator in the buffer.
        int m_index;

        /// Whether we are ready with regard to performance time.
        /**
         * We always are except when starting or jumping in time.  Making us
         * ready is derived classes' job via doInsert().
         */
        bool m_ready;

        /**
         * Whether this iterator has more events to give within the current
         * time slice.
         */
        bool m_active;

        // RealTime when the current event starts sounding.  Either
        // the current event's time or the time the loop starts,
        // whichever is greater.  Used for calculating the correct controllers
        RealTime m_currentTime;

        // !!! WARNING !!!
        // If any member objects are added to this class, the ctor, copy ctor,
        // and op= must be updated.
        // !!! WARNING !!!
    };

protected:
    friend class iterator;

    /// Capacity of the buffer.
    /**
     * To be consistent with the STL, this would be better named m_capacity.
     */
    mutable QAtomicInt m_size;

    /// Number of events in the buffer.
    /**
     * To be consistent with the STL, this would be better named m_size.
     */
    mutable QAtomicInt m_fill;

    /// The Mapped Event Buffer
    MappedEvent *m_buffer;

    bool m_isMetronome;

    /// Lock for iterator::peek() and resizeBuffer()
    /**
     * Used by resizeBuffer() to lock the swapping of the old for the new
     * and the changing of the buffer capacity.
     *
     * Used by MappedEventBuffer::iterator::peek() to ensure that the
     * buffer's capacity doesn't change while it is getting the current item.
     *
     * ??? Honestly, this lock doesn't seem very useful.  The worst case
     *     scenario is that peek() is trying to get a pointer to the element,
     *     and resizeBuffer() swaps the entire buffer out from under it.
     *     This lock does nothing to prevent that.  Even if the locking
     *     were made more coarse-grained to prevent this (maybe at the very
     *     beginning of each routine), the caller of peek() would be
     *     holding a pointer that could be pointing to deleted memory at
     *     any moment.
     */
    QReadWriteLock m_lock;

    /// Not used here.  Convenience for derivers.
    /**
     * Derivers should probably use RosegardenMainWindow::self() to get to
     * RosegardenDocument.
     */
    RosegardenDocument *m_doc;

    /// Earliest sounding time.
    /**
     * It is the responsibility of "dump()" to keep this field up to date.
     *
     * @see m_end
     */
    RealTime m_start;

    /// Latest sounding time.
    /**
     * It is the responsibility of "dump()" to keep this field up to date.
     *
     * @see m_start
     */
    RealTime m_end;

    /// How many metaiterators share this mapper.
    /**
     * We won't delete while it has any owner.  This is changed just in
     * owner's ctors and dtors.
     *
     * @see addOwner()
     * @see removeOwner()
     */
    int m_refCount;

    /// Add an event to the buffer.
    void mapAnEvent(MappedEvent *e);

    /// Set the sounding times.
    /**
     * InternalSegmentMapper::dump() keeps this updated.
     *
     * @see getStartEnd()
     */
    void setStartEnd(RealTime &start, RealTime &end) {
        m_start = start;
        m_end   = end;
    }

private:
    // Hide copy ctor and op= (dtor is non-trivial)
    MappedEventBuffer(const MappedEventBuffer &);
    MappedEventBuffer &operator=(const MappedEventBuffer &);

};

}

#endif /* ifndef RG_MAPPEDEVENTBUFFER_H */
