
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
#include "Track.h"
#include "XmlExportable.h"

namespace Rosegarden 
{

/**
 * Composition contains a complete representation of a piece of music.
 * It is a container for multiple Segments, as well as any associated
 * non-Event data.
 * 
 * The Composition owns the Segments it holds, and deletes them on
 * destruction.  When Segments are removed, it will also delete them.
 */

class Composition : public XmlExportable
{
public:
    typedef std::multiset<Segment*, Segment::SegmentCmp> segmentcontainer;
    typedef segmentcontainer::iterator iterator;
    typedef segmentcontainer::const_iterator const_iterator;

    typedef std::map<TrackId, Track*> trackcontainer;
    typedef trackcontainer::iterator trackiterator;

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


    //////
    //
    // START AND END MARKERS

    timeT getStartMarker() const { return m_startMarker; }
    timeT getEndMarker() const { return m_endMarker; }

    void setStartMarker(const timeT &sM) { m_startMarker = sM; }
    void setEndMarker(const timeT &eM) { m_endMarker = eM; }



    //////
    //
    //  INSTRUMENT & TRACK

    Track* getTrackByIndex(TrackId track) {
	return m_tracks[track];
    }

    Track* getTrackByPosition(TrackId position);
 
    trackcontainer* getTracks() {
	return &m_tracks;
    }

    TrackId getRecordTrack() const {
	return m_recordTrack;
    }
    void setRecordTrack(TrackId recordTrack) {
	m_recordTrack = recordTrack;
    }

    // Get and set Solo Track
    //
    TrackId getSelectedTrack() const {
        return m_selectedTrack;
    }

    void setSelectedTrack(TrackId track) { m_selectedTrack = track; }

    // Are we soloing a Track?
    //
    bool isSolo() const { return m_solo; }
    void setSolo(bool value) { m_solo = value; }

    int getNbTracks() const {
	return m_tracks.size();
    }

    /**
     * Clear out the Track container
     */
    void clearTracks();

    /**
     * Insert a new Track
     */
    void addTrack(Track *track);
 
    /**
     * Delete a Track by index
     */
    void deleteTrack(const int &track);

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
     * Test whether a Segment exists in this Composition.
     */
    bool contains(const Segment *);

    /**
     * Return an iterator pointing at the given Segment, or end()
     * if it does not exist in this Composition.
     */
    const_iterator findSegment(const Segment *);

    /**
     * Return an iterator pointing at the given Segment, or end()
     * if it does not exist in this Composition.
     */
    iterator findSegment(Segment *);

    /**
     * Delete the Segment if it is part of the Composition
     * \return true if the Segment was found and deleted
     *
     * NOTE: The Segment is deleted from the composition and
     * destroyed
     */
    bool deleteSegment(Segment*);

    /**
     * Remove the Segment if it is part of the Composition,
     * but do not destroy it (passing it to addSegment again
     * would restore it correctly).
     * \return true if the Segment was found and removed
     *
     * NOTE: Many of the Segment methods will fail if the
     * Segment is not in a Composition.  You should not
     * expect to do anything meaningful with a Segment that
     * has been detached from the Composition in this way.
     */
    bool detachSegment(Segment*);

    /**
     * Set the start index and track number the given Segment is
     * associated with.  You should normally use this in preference to
     * Segment::setTrack and Segment::setStartTime because it
     * correctly updates the location of the Segment in the
     * Composition's container (whose ordering depends on the track
     * number).
     *
     * \return false if the Segment was not found
     */
    bool setSegmentStartTimeAndTrack(Segment *, timeT, unsigned int);



    //////
    //
    //  BAR

    /**
     * Return the total number of bars in the composition
     */
    int getNbBars() const;

    /**
     * Return the number of the bar that starts at or contains time t.
     *
     * Will happily return computed bar numbers for times before
     * the start or beyond the real end of the composition.
     */
    int getBarNumber(timeT t) const;

    /**
     * Return the starting time of bar n
     */
    timeT getBarStart(int n) const {
	return getBarRange(n).first;
    } 

    /**
     * Return the ending time of bar n
     */
    timeT getBarEnd(int n) const {
	return getBarRange(n).second;
    }

    /**
     * Return the time range of bar n.
     * 
     * Will happily return theoretical timings for bars before the
     * start or beyond the end of composition (i.e. there is no
     * requirement that 0 <= n < getNbBars()).
     */
    std::pair<timeT, timeT> getBarRange(int n) const;

    /**
     * Return the starting time of the bar that contains time t
     */
    timeT getBarStartForTime(timeT t) const {
	return getBarRangeForTime(t).first;
    }

    /**
     * Return the ending time of the bar that contains time t
     */
    timeT getBarEndForTime(timeT t) const {
	return getBarRangeForTime(t).second;
    }

    /**
     * Return the starting and ending times of the bar that contains
     * time t.
     * 
     * Will happily return theoretical timings for bars before the
     * start or beyond the end of composition.
     */
    std::pair<timeT, timeT> getBarRangeForTime(timeT t) const;



    //////
    //
    //  TIME SIGNATURE

    /**
     * Add the given time signature at the given time.  Returns the
     * resulting index of the time signature (suitable for passing
     * to removeTimeSignature, for example)
     */
    int addTimeSignature(timeT t, TimeSignature timeSig);

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
     * Return the index of the last time signature change before
     * the given time, in a range suitable for passing to 
     * getTimeSignatureChange.  Return -1 if there has been no
     * time signature by this time.
     */
    int getTimeSignatureNumberAt(timeT time) const;

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
    double getDefaultTempo() const { return m_defaultTempo; }

    /**
     * Add a tempo-change event at the given time, to the given
     * tempo (in beats per minute).  Removes any existing tempo
     * event at that time.  Returns the index of the new tempo
     * event in a form suitable for passing to removeTempoChange.
     */
    int addTempo(timeT time, double tempo);

    /**
     * Add a tempo-change event at the given time, to the given
     * tempo (in beats per hour).  Removes any existing tempo
     * event at that time.  Returns the index of the new tempo
     * event in a form suitable for passing to removeTempoChange.
     */
    int addRawTempo(timeT time, int tempo);

    /**
     * Return the number of tempo changes in the composition.
     */
    int getTempoChangeCount() const;

    /**
     * Return the index of the last tempo change before the given
     * time, in a range suitable for passing to getRawTempoChange.
     * Return -1 if the default tempo is in effect at this time.
     */
    int getTempoChangeNumberAt(timeT time) const;

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



    //////
    //
    // LOOP 

    timeT getLoopStart() const { return m_loopStart; }
    timeT getLoopEnd() const { return m_loopEnd;}

    void setLoopStart(const timeT &lS) { m_loopStart = lS; }
    void setLoopEnd(const timeT &lE) { m_loopEnd = lE; }

    // Determine if we're currently looping
    //
    bool isLooping() const { return (m_loopStart != m_loopEnd); }


    
    //////
    //
    // OTHER STUFF


    // Some set<> API delegation
    iterator       begin()       { return m_segments.begin(); }
    const_iterator begin() const { return m_segments.begin(); }
    iterator       end()         { return m_segments.end(); }
    const_iterator end() const   { return m_segments.end(); }


    // XML exportable method
    //
    virtual std::string toXmlString();

    static RealTime getTempoTimestamp(const Event *e);
    static void setTempoTimestamp(Event *e, RealTime r);

    // Who's making this racket?
    //
    std::string getCopyrightNote() const { return m_copyright; }
    void setCopyrightNote(const std::string &cr) { m_copyright = cr; }

//     // Recording count-in
//     //
//     int getCountInBars() const { return m_countInBars; }
//     void setCountInBars(int countInBars) { m_countInBars = countInBars; }


    // We can have the metronome on or off while playing or
    // recording - get and set values from here
    //
    bool usePlayMetronome() const { return m_playMetronome; }
    bool useRecordMetronome() const { return m_recordMetronome; }

    void setPlayMetronome(bool value) { m_playMetronome = value; }
    void setRecordMetronome(bool value) { m_recordMetronome = value; }

    // Expose these
    //
    static const std::string TempoEventType; 
    static const PropertyName TempoProperty; // stored in beats per hour


    //////
    //
    // QUANTIZERS

    /**
     * Return a quantizer that quantizes to the our most basic
     * units (i.e. a unit quantizer whose unit is our shortest
     * note duration).
     */
    const Quantizer *getBasicQuantizer() const {
	return &m_basicQuantizer;
    }

    /**
     * Return a quantizer that does note-quantization with the
     * default number of dots.
     */
    const Quantizer *getNoteQuantizer() const {
	return &m_noteQuantizer;
    }

    /**
     * Return a quantizer that does legato-quantization with the
     * default number of dots.
     */
    const Quantizer *getLegatoQuantizer() const {
	return &m_legatoQuantizer;
    }

    void setLegatoQuantizerDuration(timeT duration);



    //////
    //
    // REFRESH STATUS

    // delegate RefreshStatusArray API
    unsigned int getNewRefreshStatusId() { return m_refreshStatusArray.getNewRefreshStatusId(); }
    RefreshStatus& getRefreshStatus(unsigned int id) { return m_refreshStatusArray.getRefreshStatus(id); }
    /// Set all refresh statuses to true
    void updateRefreshStatuses() { m_refreshStatusArray.updateRefreshStatuses(); }

protected:

    class ReferenceSegment;

    /**
     * This is a bit like a segment, but can only contain one sort of
     * event, and can only have one event at each absolute time
     */
    class ReferenceSegment :
	public FastVector<Event *> // not a set: want random access for bars
    {
	typedef FastVector<Event *> Impl;

    public:
	ReferenceSegment(std::string eventType);
	virtual ~ReferenceSegment();
	
	typedef Impl::iterator iterator;
	typedef Impl::size_type size_type;
	typedef Impl::difference_type difference_type;

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

    static const PropertyName NoAbsoluteTimeProperty;
    static const PropertyName BarNumberProperty;
    static const PropertyName TempoTimestampProperty;

    static const unsigned int DefaultCountInBars;


    struct ReferenceSegmentEventCmp
    {
	bool operator()(const Event &e1, const Event &e2) const;
	bool operator()(const Event *e1, const Event *e2) const {
	    return operator()(*e1, *e2);
	}
    };
    
    struct BarNumberComparator
    {
	bool operator()(const Event &e1, const Event &e2) const {
	    return (e1.get<Int>(BarNumberProperty) <
		    e2.get<Int>(BarNumberProperty));
	}
	bool operator()(const Event *e1, const Event *e2) const {
	    return operator()(*e1, *e2);
	}
    };
 

    trackcontainer m_tracks;
    segmentcontainer m_segments;

    // The track we can record on
    //
    Rosegarden::TrackId m_recordTrack;

    // Are we soloing and if so which Track?
    //
    bool                m_solo;
    Rosegarden::TrackId m_selectedTrack;

    /// Contains time signature events
    mutable ReferenceSegment m_timeSigSegment;

    /// Contains tempo events
    mutable ReferenceSegment m_tempoSegment;

    Quantizer m_basicQuantizer;
    Quantizer m_noteQuantizer;
    Quantizer m_legatoQuantizer;

    timeT m_position;
    double m_defaultTempo;

    // Notional Composition markers - these define buffers for the
    // start and end of the piece, Segments can still exist outside
    // of these markers - these are for visual and playback cueing.
    //
    timeT m_startMarker;
    timeT m_endMarker;

    // Loop start and end positions.  If they're both the same
    // value (usually 0) then there's no loop set.
    //
    timeT m_loopStart;
    timeT m_loopEnd;

    /// affects m_timeSigSegment
    void calculateBarPositions() const;
    mutable bool m_barPositionsNeedCalculating;
    ReferenceSegment::iterator getTimeSignatureAtAux(timeT t) const;

    /// affects m_tempoSegment
    void calculateTempoTimestamps() const;
    mutable bool m_tempoTimestampsNeedCalculating;
    RealTime time2RealTime(timeT time, double tempo) const;
    timeT realTime2Time(RealTime rtime, double tempo) const;

    std::string m_copyright;
//     unsigned int m_countInBars;

    bool m_playMetronome;
    bool m_recordMetronome;

    RefreshStatusArray<RefreshStatus> m_refreshStatusArray;

    bool m_needsRefresh;

private:
    Composition(const Composition &);
    Composition &operator=(const Composition &);

};

}


#endif

