// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _COMPOSITION_H_
#define _COMPOSITION_H_

#include <set>

#include "Segment.h"
#include "Quantizer.h"

namespace Rosegarden 
{
    
/**
 * Composition contains a complete representation of a piece of music.
 * It is a container for multiple Segments, as well as any associated
 * non-Event data.
 * 
 * The Composition owns the Segments it holds, and deletes them on
 * destruction.  When segments are removed, it will also delete them.
 */

//!!! This could usefully do with a bit more tidying up.  We're
// gradually increasing the amount of stuff stored in the Composition
// as opposed to in individual Segments.

class Composition : public SegmentObserver
{
    
public:
    static const std::string BarEventType;

    typedef std::set<Segment*, Segment::SegmentCmp> segmentcontainer;

    typedef segmentcontainer::iterator iterator;
    typedef segmentcontainer::const_iterator const_iterator;

    Composition();
    virtual ~Composition();

    /// swap the contents with another composition
    void swap(Composition&);

    /**
     * Returns the segment storing Bar and TimeSignature events
     */
    Segment *getReferenceSegment() {
	referenceSegmentRequested(0);
	return &m_timeReference;
    }

    const Quantizer *getQuantizer() const {
	return &m_quantizer;
    }

    segmentcontainer& getSegments() { return m_segments; }
    const segmentcontainer& getSegments() const { return m_segments; }

    /**
     * Add a new segment and return an iterator pointing to it
     * The inserted Segment is owned by the Composition object
     */
    iterator addSegment(Segment*);

    /**
     * Delete the segment pointed to by the specified iterator
     *
     * NOTE: The segment is deleted from the composition and
     * destroyed
     */
    void deleteSegment(iterator);

    /**
     * Delete the segment if it is part of the Composition
     * \return true if the segment was found and deleted
     *
     * NOTE: The segment is deleted from the composition and
     * destroyed
     */
    bool deleteSegment(Segment*);

    unsigned int getNbSegments() const { return m_segments.size(); }

    /// returns the absolute end time of the segment that ends last
    timeT getDuration() const;

    /// returns the total number of bars in the composition
    unsigned int getNbBars() { return getBarNumber(getDuration()) + 1; }

    /// Removes all Segments from the Composition and destroys them
    void clear();


    /**
     * Return the number of the bar that starts at or contains time t
     */
    int getBarNumber(timeT t);

    /**
     * Return the starting time of the bar that contains time t
     */
    timeT getBarStart(timeT t);

    /**
     * Return the ending time of the bar that contains time t
     */
    timeT getBarEnd(timeT t);

    /**
     * Return the time range of bar n.  Relatively inefficient.
     * If truncate is true, will stop at end of segment and return last
     * real bar if n is out of range; otherwise will happily return
     * theoretical timings for bars beyond the real end of composition
     * (this is used when extending segments on the segment editor).
     */
    std::pair<timeT, timeT> getBarRange(int n, bool truncate = false);


    // Some set<> API delegation
    iterator       begin()       { return m_segments.begin(); }
    const_iterator begin() const { return m_segments.begin(); }
    iterator       end()         { return m_segments.end(); }
    const_iterator end() const   { return m_segments.end(); }

    // Tempo here is only our current Transport tempo which we use on
    // the GUI and is sent to the Sequencer.
    //
    double getTempo() const { return m_tempo; }
    void setTempo(const double &tempo) { m_tempo = tempo; }

    /// Get playback position
    timeT getPosition() { return m_position; }

    /// Set playback position
    void setPosition(const timeT& position) { m_position = position; }


    // SegmentObserver methods:

    virtual void eventAdded(const Segment *, Event *);
    virtual void eventRemoved(const Segment *, Event *);
    virtual void referenceSegmentRequested(const Segment *);

protected:
    segmentcontainer m_segments;

    /// Contains time signature and new-bar events.
    Segment m_timeReference;

    // called from calculateBarPositions
    Segment::iterator addNewBar(timeT time);

    Quantizer m_quantizer;

    double m_tempo;

    timeT m_position;

    /// affects the reference segment in m_timeReference
    void calculateBarPositions();
    bool m_barPositionsNeedCalculating;

private:
    Composition(const Composition &);
    Composition &operator=(const Composition &);
};

}


#endif

