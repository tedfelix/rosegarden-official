/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2020 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MEBITERATOR_H
#define RG_MEBITERATOR_H

#include "MappedEventBuffer.h"

#include <QSharedPointer>

namespace Rosegarden {


/// MappedEventBuffer iterator.
/**
 * MappedBufMetaIterator creates and manages these.
 * MappedBufMetaIterator::m_iterators is a std::vector of these.
 */
class MEBIterator
{
public:
    MEBIterator(QSharedPointer<MappedEventBuffer> mappedEventBuffer);

    /// Go back to the beginning of the MappedEventBuffer
    void reset()  { m_index = 0; }

    bool atEnd() const
        { return (m_index >= m_mappedEventBuffer->size()); }

    /// Prefix operator++
    MEBIterator& operator++();

    void moveTo(const RealTime &time);

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
     * Callers should lock getLock() with QReadLocker for as long
     * as they are holding the pointer.
     *
     * @see operator*()
     */
    MappedEvent *peek() const;

    /// Insert the MappedEvent into the MappedInserterBase.
    /**
     * Adjusts the MEBIterator's m_currentTime.
     * Sets the MEBIterator as ready.
     *
     * Delegates to MappedEventBuffer::doInsert().
     *
     * Guarantees the caller that appropriate preparation will be
     * done for evt, such as first inserting other events to set
     * the program.
     */
    void doInsert(MappedInserterBase &inserter, MappedEvent &evt);

    /// Access to the MappedEventBuffer the MEBIterator is connected to.
    QSharedPointer<MappedEventBuffer> getMappedEventBuffer()
            { return m_mappedEventBuffer; }

    /**
     * Called by MappedBufMetaIterator::fetchEventsNoncompeting().
     *
     * @see setInactive()
     * @see getActive()
     */
    void setActive(bool value, RealTime currentTime) {
        m_active = value;
        m_currentTime = currentTime;
    }
    /**
     * Called by MappedBufMetaIterator::fetchEventsNoncompeting().
     *
     * @see setActive()
     * @see getActive()
     */
    void setInactive()  { m_active = false; }
    /**
     * Whether this iterator has more events to give within the current
     * time slice.
     *
     * Called by MappedBufMetaIterator::fetchEventsNoncompeting().
     *
     * @see setActive()
     * @see setInactive()
     * @see m_active
     */
    bool getActive() const  { return m_active; }

    /// Whether makeReady() needs to be called.
    /**
     * @see isReady() and makeReady()
     */
    void setReady(bool value)  { m_ready = value; };

    /// Does makeReady() need to be called?
    /**
     * MappedBufMetaIterator::fetchEventsNoncompeting() is the only
     * real user.  There is another caller, but the result is ignored.
     *
     * @see setReady() and makeReady()
     */
    bool isReady() const  { return m_ready; }

    /// Prepares an Instrument for playback.
    /**
     * Only InternalSegmentMapper and MetronomeMapper handle this.
     * They send out a channel setup (BS/PC/CCs) when asked to make
     * an Instrument ready.
     *
     * @see setReady() and isReady()
     */
    void makeReady(MappedInserterBase &inserter, RealTime time) {
        m_mappedEventBuffer->makeReady(inserter, time);
        setReady(true);
    }

    /// Return whether the event should be played at all
    /**
     * For instance, it might be on a muted track and shouldn't
     * actually sound.  Delegates to MappedEventBuffer::shouldPlay().
     */
    bool shouldPlay(MappedEvent *evt, RealTime startTime)
        { return m_mappedEventBuffer->shouldPlay(evt, startTime); }

    /// Get a pointer to the MappedEventBuffer's lock.
    /**
     * This is used when reading to prevent MappedEventBuffer from
     * reallocating the buffer while we are holding a pointer.
     */
    QReadWriteLock *getLock() const
        { return &m_mappedEventBuffer->m_lock; }

private:
    /// The buffer this iterator points into.
    QSharedPointer<MappedEventBuffer> m_mappedEventBuffer;

    /// Position of the iterator in the buffer.
    int m_index;

    // Additional non-iterator information.

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

    /// RealTime when the current event starts sounding.
    /**
     * Either the current event's time or the time the loop starts,
     * whichever is greater.  Used for calculating the correct
     * controllers
     */
    RealTime m_currentTime;
};


}


#endif
