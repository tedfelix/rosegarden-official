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

#ifndef RG_MAPPEDEVENTBUFFER_H
#define RG_MAPPEDEVENTBUFFER_H

#include "base/RealTime.h"
#include "base/Track.h"

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

    /// Access to the internal buffer of events.  NOT LOCKED
    /**
     * un-locked, use only from write/resize thread
     *
     * This is always used along with [] to access a specific MappedEvent.
     *
     * ??? Unsafe.  This allows direct access to the buffer without any
     *     sort of range checking.  Recommend adding an operator[] and/or an
     *     at() that asserts on range problems.
     */
    MappedEvent *getBuffer()  { return m_buffer; }

    /// Capacity of the buffer in MappedEvent's.
    int capacity() const;
    /// Number of MappedEvent objects in the buffer.
    int size() const;

    /// Sets the buffer capacity.
    /**
     * Ignored if smaller than old capacity.
     *
     * @see capacity()
     *
     */
    void reserve(int newSize);

    /// Sets the number of events in the buffer.
    /**
     * Must be no bigger than buffer capacity.
     *
     * @see size()
     *
     */
    void resize(int newFill);

    /// Refresh the buffer
    /**
     * Called after the segment has been modified.  Resizes the buffer if
     * needed, then calls fillBuffer() to fill it from the segment.
     *
     * Returns true if buffer size changed (and thus the sequencer
     * needs to be told about it).
     */
    bool refresh();

    /// Get the earliest and latest sounding times.
    /**
     * Called by MappedBufMetaIterator::fetchEvents() and
     * MappedBufMetaIterator::fetchEventsNoncompeting().
     *
     * @see setStartEnd()
     */
    void getStartEnd(RealTime &start, RealTime &end) const {
        start = m_start;
        end   = m_end;
    }

    virtual TrackId getTrackID() const  { return UINT_MAX; }
    virtual void insertChannelSetup(MappedInserterBase &)  { }

protected:
    /* Virtual functions */

    /// Calculates the required capacity based on the current document.
    /**
     * Overridden by each deriver to compute the number of MappedEvent's
     * needed to hold the contents of the current segment.
     *
     * Used by init() and refresh() to adjust the buffer capacity to be
     * big enough to hold all the events in the current segment.
     *
     * @see m_doc
     * @see reserve()
     */
    virtual int calculateSize() = 0;

    /// Insert a MappedEvent with appropriate setup for channel.
    /**
     * InternalSegmentMapper and MetronomeMapper override this to bring
     * ChannelManager into the picture to dynamically allocate a channel
     * for the event before inserting.
     *
     * The inserter can be as simple as a MappedEventInserter which just
     * adds the events to a MappedEventList.  See MappedInserterBase and its
     * derivers for more.
     *
     * refTime is not necessarily the same as MappedEvent's
     * getEventTime() because we might jump into the middle of a long
     * note.
     *
     * Only used by MappedEventBuffer::iterator::doInsert().
     */
    virtual void doInsert(MappedInserterBase &inserter, MappedEvent &evt,
                          RealTime refTime, bool firstOutput);

    /// Make the channel, if any, ready to be played on.
    /**
     * InternalSegmentMapper and MetronomeMapper override this to bring
     * ChannelManager into the picture.
     *
     * @param time is the reference time in case we need to calculate
     * something time-dependent, which can happen if we jump into the
     * middle of playing.
     *
     * @see isReady
     */
    virtual void makeReady(MappedInserterBase &inserter, RealTime time);

    /// Fill buffer with data from the segment
    /**
     * This is provided by derivers to handle whatever sort of data is
     * specific to each of them.  E.g. InternalSegmentMapper::fillBuffer()
     * processes note events while TempoSegmentMapper processes tempo
     * change events.
     *
     */
    virtual void fillBuffer() = 0;

    /// Return whether the event would even sound.
    /**
     * For instance, it might be on a muted track and shouldn't be
     * played, or it might have already ended by startTime.  The exact
     * logic differs across derived classes.
     */
    virtual bool shouldPlay(MappedEvent *evt, RealTime startTime)=0;


    /// Not used here.  Convenience for derivers.
    /**
     * Derivers should probably use RosegardenMainWindow::self() to get to
     * RosegardenDocument.
     */
    RosegardenDocument *m_doc;

    /// Earliest sounding time.
    /**
     * It is the responsibility of "fillBuffer()" to keep this field
     * up to date.
     *
     * @see m_end
     */
    RealTime m_start;

    /// Latest sounding time.
    /**
     * It is the responsibility of "fillBuffer()" to keep this field
     * up to date.
     *
     * @see m_start
     */
    RealTime m_end;

    /// Add an event to the buffer.
    void mapAnEvent(MappedEvent *e);

    /// Set the sounding times (m_start, m_end).
    /**
     * InternalSegmentMapper::fillBuffer() keeps this updated.
     *
     * @see getStartEnd()
     */
    void setStartEnd(RealTime &start, RealTime &end) {
        m_start = start;
        m_end   = end;
    }

private:
    /// Hidden and not implemented as dtor is non-trivial.
    MappedEventBuffer(const MappedEventBuffer &);
    /// Hidden and not implemented as dtor is non-trivial.
    MappedEventBuffer &operator=(const MappedEventBuffer &);

    // MEBIterator needs:
    //   m_buffer
    //   m_lock
    //   makeReady()
    //   shouldPlay()
    //   doInsert()
    // ??? MappedEventBuffer is mostly a data class.  We should probably
    //     just make those public and get rid of this.
    friend class MEBIterator;

    /// The Mapped Event Buffer
    MappedEvent *m_buffer;

    /// Capacity of the buffer.
    /**
     * ??? While storeRelease() is used with this, it's somewhat dubious
     *     as to whether that is necessary.  There are no other uses
     *     of m_capacity anywhere near that one.  It's very likely that
     *     QAtomicInt is not needed here.
     */
    mutable QAtomicInt m_capacity;

    /// Number of events in the buffer.
    /**
     * ??? We only use load() and store().  This doesn't need to be a
     *     QAtomicInt.
     */
    mutable QAtomicInt m_size;

    /// Lock for reserve() and callers to iterator::peek()
    /**
     * Used by reserve() to lock the swapping of the old for the new
     * and the changing of the buffer capacity.
     *
     * Used by callers to MappedEventBuffer::iterator::peek() to
     * ensure that buffer isn't reallocated while the caller is
     * holding the current item.  Doing so avoids the scenario where
     * peek() gets a pointer to the element and then reserve() swaps
     * the entire buffer out from under it.
     */
    QReadWriteLock m_lock;

    /// How many metaiterators share this mapper.
    /**
     * We won't delete while it has any owner.  This is changed just in
     * owner's ctors and dtors.
     */
    int m_refCount;

};


}

#endif /* ifndef RG_MAPPEDEVENTBUFFER_H */
