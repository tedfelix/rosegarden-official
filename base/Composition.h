
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
#include <map>

#include "Segment.h"
#include "Quantizer.h"
#include "Instrument.h"
#include "Track.h"

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
    static const PropertyName BarNumberProperty;

    static const std::string TempoEventType; 
    static const PropertyName TempoProperty; // stored in beats per hour

    typedef std::set<Segment*, Segment::SegmentCmp> segmentcontainer;

    typedef std::map<int, Instrument> instrumentcontainer;
    typedef std::map<int, Track> trackcontainer;

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
	calculateBarPositions();
	return &m_referenceSegment;
    }

    const Quantizer *getQuantizer() const {
	return &m_quantizer;
    }

    segmentcontainer& getSegments() { return m_segments; }
    const segmentcontainer& getSegments() const { return m_segments; }

    Track* getTrack(const int &track) { return &(m_tracks[track]); }
    Instrument* getInstrument(const int &instr)
            { return &(m_instruments[instr]); }
 
    trackcontainer getTracks() { return m_tracks; }
    instrumentcontainer getInstruments() { return m_instruments; }

    unsigned int getNbSegments() const { return m_segments.size(); }
    int getNbTracks() const { return m_tracks.size(); }

    /**
     * Add a new segment and return an iterator pointing to it
     * The inserted Segment is owned by the Composition object
     */
    iterator addSegment(Segment*);

    
    /**
     * Insert a new Instrument
     */
    void addInstrument(const Instrument &inst);


    /**
     * Insert a new Track
     */
    void addTrack(const Track &track);

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

    /**
     * Remove all Segments from the Composition and destroy them
     */
    void clear();

    /**
     * Return the absolute end time of the segment that ends last
     */
    timeT getDuration() const;

    /**
     * Return the total number of bars in the composition
     */
    unsigned int getNbBars() const {
	return getBarNumber(getDuration(), false) + 1;
    }

    /**
     * Return the starting time of the bar that contains time t
     */
    timeT getBarStart(timeT t) const;

    /**
     * Return the ending time of the bar that contains time t
     */
    timeT getBarEnd(timeT t) const;

    /**
     * Return the number of the bar that starts at or contains time t.
     *
     * If truncate is true, will stop at the end of the composition
     * and return the last real bar number if t is out of range;
     * otherwise will happily return computed bar numbers for times
     * beyond the real end of the composition.
     */
    int getBarNumber(timeT t, bool truncate) const;

    /**
     * Return the time range of bar n.  Relatively inefficient.
     * 
     * If truncate is true, will stop at the end of the composition
     * and return last real bar if n is out of range; otherwise will
     * happily return theoretical timings for bars beyond the real end
     * of composition (this is used when extending segments on the
     * segment editor).
     */
    std::pair<timeT, timeT> getBarRange(int n, bool truncate) const;

    /**
     * Return the starting and ending times of the bar that contains
     * time t.  Unlike getBarRange, this will only work for bars 
     * that actually exist and will stop working at the end of the
     * composition.  It's much, much quicker though.
     */
    std::pair<timeT, timeT> getBarRange(timeT t) const;

    /**
     * Return the time signature in effect at time t
     */
    TimeSignature getTimeSignatureAt(timeT t) const;

    /**
     * Return the time signature in effect at time t, and the time at
     * which it came into effect
     */
    timeT getTimeSignatureAt(timeT, TimeSignature &) const;

    /**
     * Return the tempo in effect at time t.  Can be very slow;
     * for playback, prefer setPosition plus getTempo.
     */
    double getTempoAt(timeT t) const;

    /**
     * Return the tempo in effect at the current playback position.
     * Equivalent to getTempoAt(getPosition()), but much quicker.
     */
    double getTempo() const { return m_currentTempo; }

    /**
     * Set a default tempo for the composition.  This will be
     * overridden by any tempo events we encounter subsequently.
     */
    void setDefaultTempo(double tempo) { m_defaultTempo = tempo; }

    /**
     * Get the current playback position
     */
    timeT getPosition() { return m_position; }

    /**
     * Set the current playback position (causing the current tempo
     * to be updated also)
     */
    void setPosition(timeT position);


    // Some set<> API delegation
    iterator       begin()       { return m_segments.begin(); }
    const_iterator begin() const { return m_segments.begin(); }
    iterator       end()         { return m_segments.end(); }
    const_iterator end() const   { return m_segments.end(); }


    // SegmentObserver methods:

    virtual void eventAdded(const Segment *, Event *);
    virtual void eventRemoved(const Segment *, Event *);

protected:
    trackcontainer m_tracks;
    segmentcontainer m_segments;
    instrumentcontainer m_instruments;

    /// Contains time signature and new-bar events.
    mutable Segment m_referenceSegment;

    // called from calculateBarPositions
    Segment::iterator addNewBar(timeT time, int barNo) const;

    Quantizer m_quantizer;

    timeT m_position;
    double m_currentTempo;
    double m_defaultTempo;

    /// affects the reference segment in m_referenceSegment
    void calculateBarPositions() const;
    mutable bool m_barPositionsNeedCalculating;

private:
    Composition(const Composition &);
    Composition &operator=(const Composition &);

};

}


#endif

