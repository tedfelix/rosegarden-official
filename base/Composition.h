
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#include "FastVector.h"

#include "RealTime.h"
#include "Segment.h"
#include "Quantizer.h"
#include "Instrument.h"
#include "Track.h"
#include "XmlExportable.h"

namespace Rosegarden 
{

typedef std::map<int, Instrument*> instrumentcontainer;
typedef std::map<int, Track*> trackcontainer;

typedef instrumentcontainer::iterator instrumentiterator;
typedef trackcontainer::iterator trackiterator;


/**
 * Composition contains a complete representation of a piece of music.
 * It is a container for multiple Segments, as well as any associated
 * non-Event data.
 * 
 * The Composition owns the Segments it holds, and deletes them on
 * destruction.  When Segments are removed, it will also delete them.
 */

class Composition : public SegmentObserver,
		    public XmlExportable
{
    
public:
    typedef std::set<Segment*, Segment::SegmentCmp> segmentcontainer;

    typedef segmentcontainer::iterator iterator;
    typedef segmentcontainer::const_iterator const_iterator;

    Composition();
    virtual ~Composition();

    /// swap the contents with another composition
    void swap(Composition&);

    /**
     * Remove all Segments from the Composition and destroy them
     */
    void clear();

    /**
     * Return the absolute end time of the segment that ends last
     */
    timeT getDuration() const;

    const Quantizer *getQuantizer() const {
	return &m_quantizer;
    }


    //////
    //
    //  INSTRUMENT & TRACK

    Track* getTrackByIndex(const int &track) { return m_tracks[track]; }
    Instrument* getInstrumentByIndex(const int &instr)
            { return m_instruments[instr]; }
 
    trackcontainer* getTracks() { return &m_tracks; }
    instrumentcontainer* getInstruments() { return &m_instruments; }

    int getRecordTrack() const { return m_recordTrack; }
    void setRecordTrack(const int &recordTrack) { m_recordTrack = recordTrack; }

    int getNbTracks() const { return m_tracks.size(); }

    /**
     * Clear out the Track container
     */
    void clearTracks();

    /**
     * Clear out the Instruments
     */
    void clearInstruments();

    /**
     * Insert a new Instrument
     */
    void addInstrument(Instrument *inst);

    /**
     * Insert a new Track
     */
    void addTrack(Track *track);
 
    /**
     * Delete a Track by index
     */
    void deleteTrack(const int &track);

    /*
     * Delete instrument by index
     */
    void deleteInstrument(const int &instrument);



    //////
    //
    //  SEGMENT

    segmentcontainer& getSegments() { return m_segments; }
    const segmentcontainer& getSegments() const { return m_segments; }

    unsigned int getNbSegments() const { return m_segments.size(); }

    /**
     * Add a new Segment and return an iterator pointing to it
     * The inserted Segment is owned by the Composition object
     */
    iterator addSegment(Segment*);

    /**
     * Delete the Segment pointed to by the specified iterator
     *
     * NOTE: The Segment is deleted from the Composition and
     * destroyed
     */
    void deleteSegment(iterator);

    /**
     * Delete the Segment if it is part of the Composition
     * \return true if the Segment was found and deleted
     *
     * NOTE: The Segment is deleted from the composition and
     * destroyed
     */
    bool deleteSegment(Segment*);

    /**
     * Set the formal starting time of the Segment.  You should
     * normally use this in preference to Segment::setStartIndex
     * because it correctly updates the location of the Segment
     * in the Composition's container (whose ordering depends on
     * the start index).
     * \return false if the Segment was not found
     */
    bool setSegmentStartIndex(Segment *, timeT);

    /**
     * Set the track number the given Segment is associated with.
     * You should normally use this in preference to Segment::setTrack
     * because it correctly updates the location of the Segment
     * in the Composition's container (whose ordering depends on
     * the track number).
     * \return false if the Segment was not found
     */
    bool setSegmentTrack(Segment *, unsigned int);



    //////
    //
    //  BAR

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
     * Return the time range of bar n.
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
     * time t.  Unlike getBarRange(int, bool) this will only work for
     * bars that actually exist and will stop working at the end of
     * the composition.
     */
    std::pair<timeT, timeT> getBarRange(timeT t) const;



    //////
    //
    //  TIME SIGNATURE

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
     * Return the time signature in effect in bar n.  Also sets
     * isNew to true if the time signature is a new one that did
     * not appear in the previous bar.
     */
    TimeSignature getTimeSignatureInBar(int n, bool &isNew) const;

    /**
     * Return the total number of time signature changes in the
     * composition.
     */
    int getTimeSignatureCount() const;

    /**
     * Return the absolute time of and time signature introduced
     * by time-signature change n.
     */
    std::pair<timeT, TimeSignature> getTimeSignatureChange(int n) const;

    /**
     * Remove time signature change event n from the composition.
     */
    void removeTimeSignature(int n);



    //////
    //
    //  TEMPO

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
     * Add a tempo-change event at the given time, to the given
     * tempo (in beats per hour).  Removes any existing tempo
     * event at that time.
     */
    void addRawTempo(timeT time, int tempo);

    /**
     * Return the number of tempo changes in the composition.
     */
    int getTempoChangeCount() const;

    /**
     * Return the absolute time of and tempo introduced by tempo
     * change number n, in beats per hour (this is the value that's
     * actually stored)
     */
    std::pair<timeT, long> getRawTempoChange(int n) const;

    /**
     * Remove tempo change event n from the composition.
     */
    void removeTempoChange(int n);



    //////
    //
    //  REAL TIME

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



    /////////////
    //
    // START AND END MARKERS
    //
    timeT getStartMarker() const { return m_startMarker; }
    timeT getEndMarker() const { return m_endMarker; }

    void setStartMarker(const timeT &sM) { m_startMarker = sM; }
    void setEndMarker(const timeT &eM) { m_endMarker = eM; }


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
    virtual std::string toXmlString();

    static RealTime getTempoTimestamp(const Event *e);
    static void setTempoTimestamp(Event *e, RealTime r);

protected:

    class ReferenceSegment;

    /**
     * This is a bit like a segment, but can only contain one sort of
     * event, and can only have one event at each absolute time
     */
    class ReferenceSegment :
	private FastVector<Event *> // not a set: want random access for bars
    {
	typedef FastVector<Event *> Impl;

    public:
	ReferenceSegment(std::string eventType);
	virtual ~ReferenceSegment();
	
	typedef Impl::iterator iterator;

	Impl::size;
	Impl::begin;
	Impl::end;
	Impl::operator[];

	void clear();

	timeT getDuration() const;
	
	/// Inserts a single event, removing any existing one at that time
	iterator insert(Event *e); // may throw Event::BadType

	void erase(Event *e);

	void swap(ReferenceSegment &);

	iterator findTime(timeT time);
	iterator findNearestTime(timeT time);

	iterator findRealTime(RealTime time);
	iterator findNearestRealTime(RealTime time);

    private:
  	iterator find(Event *e);
	std::string m_eventType;
    };

    static const std::string BarEventType;
    static const PropertyName BarNumberProperty;
    static const PropertyName BarHasTimeSigProperty;

    static const std::string TempoEventType; 
    static const PropertyName TempoProperty; // stored in beats per hour
    static const PropertyName TempoTimestampSecProperty;
    static const PropertyName TempoTimestampUsecProperty;


    struct ReferenceSegmentEventCmp
    {
	bool operator()(const Event &e1, const Event &e2) const;
	bool operator()(const Event *e1, const Event *e2) const;
    };
    

    trackcontainer m_tracks;
    segmentcontainer m_segments;
    instrumentcontainer m_instruments;
    int m_recordTrack;

    /// Contains new-bar events -- not saved, but based on the timesigs
    mutable ReferenceSegment m_barSegment;

    /// Contains time signature events
    mutable ReferenceSegment m_timeSigSegment;

    /// Contains tempo events
    mutable ReferenceSegment m_tempoSegment;

    // called from calculateBarPositions
    ReferenceSegment::iterator addNewBar(timeT time, timeT duration, int barNo,
					 bool hasTimeSig) const;

    Quantizer m_quantizer;

    timeT m_position;
    double m_defaultTempo;

    // Notional Composition markers - these define buffers for the
    // start and end of the piece, Segments can still exist outside
    // of these markers - these are for visual and playback cueing.
    //
    timeT m_startMarker;
    timeT m_endMarker;

    /// affects m_barSegment
    void calculateBarPositions() const;
    mutable bool m_barPositionsNeedCalculating;

    /// affects m_tempoSegment
    void calculateTempoTimestamps() const;
    mutable bool m_tempoTimestampsNeedCalculating;
    RealTime time2RealTime(timeT time, double tempo) const;
    timeT realTime2Time(RealTime rtime, double tempo) const;

private:
    Composition(const Composition &);
    Composition &operator=(const Composition &);

};

}


#endif

