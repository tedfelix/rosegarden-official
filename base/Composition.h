
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
#include "XmlExportable.h"

namespace Rosegarden 
{

typedef unsigned long long RealTime; // usec
    
typedef std::map<int, Instrument> instrumentcontainer;
typedef std::map<int, Track> trackcontainer;

typedef instrumentcontainer::iterator instrumentiterator;
typedef trackcontainer::iterator trackiterator;

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

class Composition : public SegmentObserver, public XmlExportable
{
    
public:
    static const std::string BarEventType;
    static const PropertyName BarNumberProperty;

    static const std::string TempoEventType; 
    static const PropertyName TempoProperty; // stored in beats per hour
    static const PropertyName TempoTimestampProperty;

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
    Segment *getBarSegment() {
	calculateBarPositions();
	return &m_barSegment;
    }

    Segment *getTempoSegment() {
	return &m_tempoSegment;
    }

    const Quantizer *getQuantizer() const {
	return &m_quantizer;
    }

    segmentcontainer& getSegments() { return m_segments; }
    const segmentcontainer& getSegments() const { return m_segments; }

    Track* getTrackByIndex(const int &track) { return &(m_tracks[track]); }
    Instrument* getInstrumentByIndex(const int &instr)
            { return &(m_instruments[instr]); }
 
    trackcontainer* getTracks() { return &m_tracks; }
    instrumentcontainer* getInstruments() { return &m_instruments; }

    int getRecordTrack() const { return m_recordTrack; }
    void setRecordTrack(const int &recordTrack) { m_recordTrack = recordTrack; }

    unsigned int getNbSegments() const { return m_segments.size(); }
    int getNbTracks() const { return m_tracks.size(); }

    /* Clear out the track container
     *
     */
    void clearTracks() { m_tracks.clear(); }

    /**
     * Insert a new Instrument
     */
    void addInstrument(const Instrument &inst);

    /**
     * Insert a new Track
     */
    void addTrack(const Track &track);
 
    /**
     * Delete a Track by index
     */
    void deleteTrack(const int &track);

    /*
     * Delete instrument by index
     */
    void deleteInstrument(const int &instrument);

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
    int getNbBars() const;

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
     * Add the given time signature at the given time
     */
    void addTimeSignature(timeT t, TimeSignature timeSig);

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
     * Return the tempo in effect at time t, in beats per minute.
     */
    double getTempoAt(timeT t) const;

    /**
     * Return the tempo in effect at the current playback position.
     */
    double getTempo() const { return getTempoAt(getPosition()); }

    /**
     * Set a default tempo for the composition.  This will be
     * overridden by any tempo events encountered during playback.
     */
    void setDefaultTempo(double tempo) { m_defaultTempo = tempo; }

    /**
     * Add a tempo-change event at the given time, to the given
     * tempo (in beats per minute).  Removes any existing tempo
     * event at that time.
     */
    void addTempo(timeT time, double tempo);

    /**
     * Return the number of microseconds elapsed between
     * the beginning of the composition and the given timeT time.
     * (timeT units are independent of tempo; this takes into
     * account any tempo changes in the first t units of time.)
     *
     * This is a fairly efficient operation, not dependent on the
     * magnitude of t or the number of tempo changes in the piece.
     */
    RealTime getElapsedRealTime(timeT t) const;

    /**
     * Return the nearest time in timeT units to the point at the
     * given number of microseconds after the beginning of the
     * composition.  (timeT units are independent of tempo; this takes
     * into account any tempo changes in the first t microseconds.)
     * The result will be approximate, as timeT units are obviously
     * less precise than microseconds.
     *
     * This is a fairly efficient operation, not dependent on the
     * magnitude of t or the number of tempo changes in the piece.
     */
    timeT getElapsedTimeForRealTime(RealTime t) const;

    /**
     * Return the number of microseconds elapsed between
     * the two given timeT indices into the composition, taking
     * into account any tempo changes between the two times.
     */
    RealTime getRealTimeDifference(timeT t0, timeT t1) const {
	if (t1 > t0) return getElapsedRealTime(t1) - getElapsedRealTime(t0);
	else	     return getElapsedRealTime(t0) - getElapsedRealTime(t1);
    }

    /**
     * Get the current playback position.
     */
    timeT getPosition() const { return m_position; }

    /**
     * Set the current playback position.
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

    // XML exportable method
    //
    virtual string toXmlString();

protected:
    trackcontainer m_tracks;
    segmentcontainer m_segments;
    instrumentcontainer m_instruments;
    int m_recordTrack;

    /// Contains time signature and new-bar events.
    mutable Segment m_barSegment;

    /// Contains tempo events.
    mutable Segment m_tempoSegment;

    // called from calculateBarPositions
    Segment::iterator addNewBar(timeT time, int barNo) const;
    mutable int m_barCount;

    Quantizer m_quantizer;

    timeT m_position;
    double m_defaultTempo;

    /// affects m_barSegment
    void calculateBarPositions() const;
    mutable bool m_barPositionsNeedCalculating;

    /// affects m_tempoSegment
    void calculateTempoTimestamps() const;
    mutable bool m_tempoTimestampsNeedCalculating;
    RealTime time2RealTime(timeT time, double tempo) const;
    timeT realTime2Time(RealTime rtime, double tempo) const;
    static bool compareTempoTimestamps(const Event *, const Event *);

private:
    Composition(const Composition &);
    Composition &operator=(const Composition &);

};

}


#endif

