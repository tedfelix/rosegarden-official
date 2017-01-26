/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_COMPOSITION_H
#define RG_COMPOSITION_H


#include "FastVector.h"

#include "RealTime.h"
#include "base/Segment.h"
#include "Track.h"
#include "Configuration.h"
#include "XmlExportable.h"
#include "ColourMap.h"
#include "TriggerSegment.h"

#include "Marker.h"

// Qt
#include <QtCore/QWeakPointer>

// System
#include <set>
#include <map>

namespace Rosegarden 
{
// We store tempo in quarter-notes per minute * 10^5 (hundred
// thousandths of a quarter-note per minute).  This means the maximum
// tempo in a 32-bit integer is about 21400 qpm.  We use a signed int
// for compatibility with the Event integer type -- but note that we
// use 0 (rather than -1) to indicate "tempo not set", by convention
// (though see usage of target tempo in e.g. addTempoAtTime).
typedef int tempoT;

class Quantizer;
class BasicQuantizer;
class NotationQuantizer;

class CompositionObserver;

/// Composition contains a complete representation of a piece of music.
/**
 * It is a container for multiple Segment objects (m_segments), as well as
 * any associated non-Event data.
 * 
 * The Composition owns the Segment objects it holds, and deletes them on
 * destruction.  See deleteSegment() and detachSegment().
 */
class ROSEGARDENPRIVATE_EXPORT Composition : public XmlExportable
{
    friend class Track; // to call notifyTrackChanged()
    friend class Segment; // to call notifySegmentRepeatChanged()
    
public:
    typedef SegmentMultiSet::iterator iterator;
    typedef SegmentMultiSet::const_iterator const_iterator;

    typedef std::vector<Segment *> SegmentVec;

    typedef std::map<TrackId, Track*> trackcontainer;
    typedef trackcontainer::iterator trackiterator;
    typedef trackcontainer::const_iterator trackconstiterator;

    typedef std::vector<Marker*> markercontainer;
    typedef markercontainer::iterator markeriterator;
    typedef markercontainer::const_iterator markerconstiterator;

    typedef std::set<TriggerSegmentRec *, TriggerSegmentCmp> triggersegmentcontainer;
    typedef triggersegmentcontainer::iterator triggersegmentcontaineriterator;
    typedef triggersegmentcontainer::const_iterator triggersegmentcontainerconstiterator;

    typedef std::set<TrackId> recordtrackcontainer;
    typedef recordtrackcontainer::iterator recordtrackiterator;
    typedef recordtrackcontainer::const_iterator recordtrackconstiterator;

    Composition();
    virtual ~Composition();

private:
    Composition(const Composition &);
    Composition &operator=(const Composition &);
public:

    /**
     * Remove everything from the Composition.
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
    bool autoExpandEnabled() { return m_autoExpand; }

    void setStartMarker(const timeT &sM);
    void setEndMarker(const timeT &eM);
    void setAutoExpand(bool autoExpand) { m_autoExpand = autoExpand; }


    //////
    //
    //  INSTRUMENT & TRACK

    Track* getTrackById(TrackId track) const;

    bool haveTrack(TrackId track) const;

    Track* getTrackByPosition(int position) const;
 
    int getTrackPositionById(TrackId track) const; // -1 if not found

    trackcontainer& getTracks() { return m_tracks; }
 
    const trackcontainer& getTracks() const { return m_tracks; }

    // Reset id and position
    // unused
//    void resetTrackIdAndPosition(TrackId oldId, TrackId newId, int position);

    TrackId getMinTrackId() const;
    TrackId getMaxTrackId() const;

    const recordtrackcontainer &getRecordTracks() const { return m_recordTracks; }
    void setTrackRecording(TrackId track, bool recording);
    bool isTrackRecording(TrackId track) const;

    /**
     * rename: getSelectedTrackId()
     *
     * @see setSelectedTrack()
     */
    TrackId getSelectedTrack() const { return m_selectedTrackId; }
    /**
     * rename: setSelectedTrackId()
     *
     * @see getSelectedTrack()
     */
    void setSelectedTrack(TrackId trackId);

    /// Total number of tracks in the composition.
    unsigned int getNbTracks() const { return m_tracks.size(); }

    /**
     * Clear out the Track container
     */
    void clearTracks();

    /**
     * Insert a new Track.  The Composition takes over ownership of
     * the track object.
     */
    void addTrack(Track *track);
 
    /**
     * Delete a Track by index
     */
    // unused
//    void deleteTrack(TrackId track);

    /**
     * Detach a Track (revert ownership of the Track object to the
     * caller).
     */
    bool detachTrack(Track *track);

    /**
     * Get the highest running track id (generated and kept
     * through addTrack)
     */
    TrackId getNewTrackId() const;

    bool hasTrack(InstrumentId) const;

    /**
     * Get the Instrument Id of a given segment.
     **/
    InstrumentId getInstrumentId(Segment *s) const {
        Track* track = getTrackById(s->getTrack());
        return track->getInstrument();
    };

    /**
     * Get all segments that play on the same instrument segment s
     * plays on and start before t.
     */
    SegmentMultiSet getInstrumentSegments(Segment *s, timeT t) const;

    //////
    //
    // MARKERS

    markercontainer& getMarkers() { return m_markers; }
    const markercontainer& getMarkers() const { return m_markers; }

    /**
     * Add a new Marker.  The Composition takes ownership of the
     * marker object.
     */
    void addMarker(Marker *marker);

    /**
     * Detach a Marker (revert ownership of the Marker object to the
     * caller).
     */
    bool detachMarker(Marker *marker);

    // unused
//    bool isMarkerAtPosition(timeT time) const;

    void clearMarkers();


    //////
    //
    //  SEGMENT

    SegmentMultiSet& getSegments() { return m_segments; }
    const SegmentMultiSet& getSegments() const { return m_segments; }

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
     * DO NOT USE THIS METHOD
     *
     * Set a Segment's start time while keeping the integrity of the
     * Composition multiset.
     *
     * The segment is removed and re-inserted from the composition
     * so the ordering is preserved.
     */
    void setSegmentStartTime(Segment*, timeT);

    /**
     * Test whether a Segment exists in this Composition.
     */
    bool contains(const Segment *);

    /**
     * Return an iterator pointing at the given Segment, or end()
     * if it does not exist in this Composition.
     */
    iterator findSegment(const Segment *);

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
     * Add a new Segment which has been "weakly detached"
     *
     * Like addSegment(), but doesn't send the segmentAdded signal
     * nor updating refresh statuses
     */
    iterator weakAddSegment(Segment*);

    /**
     * Detach a segment which you're going to re-add (with weakAddSegment)
     * later.
     * Like detachSegment(), but without sending the segmentDeleted signal
     * nor updating refresh statuses.
     */
    bool weakDetachSegment(Segment*);

    /**
     * Get the largest number of segments that "overlap" at any one
     * time on the given track.  I have given this function a nice
     * long name to make it feel important.
     */
    int getMaxContemporaneousSegmentsOnTrack(TrackId track) const;

    /**
     * Retrieve a "vertical" index for this segment within its track.
     * Currently this is based on studying the way that segments on
     * the track overlap and returning the lowest integer such that no
     * prior starting segment that overlaps with this one would use
     * the same integer.  In future this could use proper voice
     * ordering.
     */
    int getSegmentVoiceIndex(const Segment *) const;

    /**
     * Add every segment in SegmentMultiSet
     */
    void addAllSegments(SegmentMultiSet segments);
    void addAllSegments(SegmentVec segments);

    /**
     * Detach every segment in SegmentMultiSet
     */
    void detachAllSegments(SegmentMultiSet segments);
    void detachAllSegments(SegmentVec segments);

    //////
    //
    //  TRIGGER SEGMENTS

    triggersegmentcontainer &getTriggerSegments() { return m_triggerSegments; }
    const triggersegmentcontainer &getTriggerSegments() const { return m_triggerSegments; }

    /**
     * Add a new trigger Segment with a given base pitch and base
     * velocity, and return its record.  If pitch or velocity is -1,
     * it will be taken from the first note event in the segment
     */
    TriggerSegmentRec *addTriggerSegment(Segment *, int pitch = -1, int velocity = -1);

    /**
     * Delete a trigger Segment.
     */
    void deleteTriggerSegment(TriggerSegmentId);

    /**
     * Detach a trigger Segment from the Composition.
     */
    void detachTriggerSegment(TriggerSegmentId);

    /**
     * Delete all trigger Segments.
     */
    void clearTriggerSegments();
    
    /**
     * Return the TriggerSegmentId for the given Segment, or -1 if it is
     * not a trigger Segment.
     */
    int getTriggerSegmentId(Segment *);
    
    /**
     * Return the Segment for a given TriggerSegmentId
     */
    Segment *getTriggerSegment(TriggerSegmentId);

    /**
     * Return the TriggerSegmentRec (with Segment, base pitch, base velocity,
     * references etc) for a given TriggerSegmentId
     */
    TriggerSegmentRec *getTriggerSegmentRec(TriggerSegmentId);

    /**
     * As above for a given Event, or NULL if none.
     **/
    TriggerSegmentRec *getTriggerSegmentRec(Event* e);
    /**
     * Add a new trigger Segment with a given ID and base pitch and
     * velocity.  Fails and returns 0 if the ID is already in use.
     * This is intended for use from file load or from undo/redo.
     */
    TriggerSegmentRec *addTriggerSegment(Segment *, TriggerSegmentId,
                                         int basePitch = -1, int baseVelocity = -1);

    /**
     * Get the ID of the next trigger segment that will be inserted.
     */
    TriggerSegmentId getNextTriggerSegmentId() const;

    /**
     * Specify the next trigger ID.  This is intended for use from file
     * load only.  Do not use this function unless you know what you're
     * doing.
     */
    void setNextTriggerSegmentId(TriggerSegmentId);

    /**
     * Update the trigger segment references for all trigger segments.
     * To be called after file load.
     */
    void updateTriggerSegmentReferences();

    /**
     * Clear refresh statuses of SegmentLinker after file load.
     */
    void resetLinkedSegmentRefreshStatuses();
    
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

    /**
     * Get the default number of bars in a new empty composition
     */
    static int getDefaultNbBars() { return m_defaultNbBars; }
    
    /**
     * Set the default number of bars in a new empty composition
     */
    static void setDefaultNbBars(int b) { m_defaultNbBars = b; }


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
     * or at the given time, in a range suitable for passing to 
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
     * Return the (approximate) number of quarters per minute for a
     * given tempo.
     */
    static double getTempoQpm(tempoT tempo) { return double(tempo) / 100000.0; }
    static tempoT getTempoForQpm(double qpm) { return tempoT(qpm * 100000 + 0.01); }

    /**
     * Return the tempo in effect at time t.  If a ramped tempo change
     * is in effect at the time, it will be properly interpolated and
     * a computed value returned.
     */
    tempoT getTempoAtTime(timeT t) const;

    /**
     * Return the tempo in effect at the current playback position.
     */
    tempoT getCurrentTempo() const { return getTempoAtTime(getPosition()); }

    /**
     * Set a default tempo for the composition.  This will be
     * overridden by any tempo events encountered during playback.
     */
    void setCompositionDefaultTempo(tempoT tempo) { m_defaultTempo = tempo; }
    tempoT getCompositionDefaultTempo() const { return m_defaultTempo; }

    /**
     * Add a tempo-change event at the given time, to the given tempo.
     * Removes any existing tempo event at that time.  Returns the
     * index of the new tempo event in a form suitable for passing to
     * removeTempoChange.
     *
     * If targetTempo == -1, adds a single constant tempo change.
     * If targetTempo == 0, adds a smooth tempo ramp from this tempo
     * change to the next.
     * If targetTempo > 0, adds a smooth tempo ramp from this tempo
     * ending at targetTempo at the time of the next tempo change.
     */
    int addTempoAtTime(timeT time, tempoT tempo, tempoT targetTempo = -1);

    /**
     * Return the number of tempo changes in the composition.
     */
    int getTempoChangeCount() const;

    /**
     * Return the index of the last tempo change before the given
     * time, in a range suitable for passing to getTempoChange.
     * Return -1 if the default tempo is in effect at this time.
     */
    int getTempoChangeNumberAt(timeT time) const;

    /**
     * Return the absolute time of and tempo introduced by tempo
     * change number n.  If the tempo is ramped, this returns only
     * the starting tempo.
     */
    std::pair<timeT, tempoT> getTempoChange(int n) const;

    /**
     * Return whether the tempo change number n is a ramped tempo or
     * not, and if it is, return the target tempo for the ramp.
     * 
     * If calculate is false, return a target tempo of 0 if the tempo
     * change is defined to ramp to the following tempo.  If calculate
     * is true, return a target tempo equal to the following tempo in
     * this case.
     */
    std::pair<bool, tempoT> getTempoRamping(int n, bool calculate = true) const;

    /**
     * Remove tempo change event n from the composition.
     */
    void removeTempoChange(int n);

    /**
     * Get the slowest assigned tempo in the composition.
     */
    tempoT getMinTempo() const {
        return ((m_minTempo != 0) ? m_minTempo : m_defaultTempo);
    }

    /**
     * Get the fastest assigned tempo in the composition.
     */
    tempoT getMaxTempo() const { 
        return ((m_maxTempo != 0) ? m_maxTempo : m_defaultTempo);
    }


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
        else         return getElapsedRealTime(t0) - getElapsedRealTime(t1);
    }

    static tempoT
        timeRatioToTempo(RealTime &realTime,
                         timeT beatTime, tempoT rampTo);

    //////
    //
    //  OTHER TIME CONVERSIONS

    /**
     * Return (by reference) the bar number and beat/division values
     * corresponding to a given absolute time.
     */
    void getMusicalTimeForAbsoluteTime(timeT absoluteTime,
                                       int &bar, int &beat,
                                       int &fraction, int &remainder);

    /**
     * Return (by reference) the number of bars and beats/divisions
     * corresponding to a given duration.  The absolute time at which
     * the duration starts is also required, so as to know the correct
     * time signature.
     */
    void getMusicalTimeForDuration(timeT absoluteTime, timeT duration,
                                   int &bars, int &beats,
                                   int &fractions, int &remainder);

    /**
     * Return the absolute time corresponding to a given bar number
     * and beat/division values.
     */
    timeT getAbsoluteTimeForMusicalTime(int bar, int beat,
                                        int fraction, int remainder);

    /**
     * Return the duration corresponding to a given number of bars and
     * beats/divisions.  The absolute time at which the duration
     * starts is also required, so as to know the correct time
     * signature.
     */
    timeT getDurationForMusicalTime(timeT absoluteTime,
                                    int bars, int beats,
                                    int fractions, int remainder);


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
    /// Segment begin iterator.
    iterator       begin()       { return m_segments.begin(); }
    /// Segment begin iterator.
    const_iterator begin() const { return m_segments.begin(); }
    /// Segment end iterator.
    iterator       end()         { return m_segments.end(); }
    /// Segment end iterator.
    const_iterator end() const   { return m_segments.end(); }


    // XML exportable method
    //
    virtual std::string toXmlString() const;

    // Who's making this racket?
    //
    Configuration &getMetadata() {
        return m_metadata;
    }
    const Configuration &getMetadata() const {
        return m_metadata;
    }
    
    std::string getCopyrightNote() const { 
        return m_metadata.get<String>(CompositionMetadataKeys::Copyright,
                                      "");
    }
    void setCopyrightNote(const std::string &cr) {
        m_metadata.set<String>(CompositionMetadataKeys::Copyright, cr);
    }


    // We can have the metronome on or off while playing or
    // recording - get and set values from here
    //
    bool usePlayMetronome() const { return m_playMetronome; }
    bool useRecordMetronome() const { return m_recordMetronome; }

    void setPlayMetronome(bool value);
    void setRecordMetronome(bool value);


    // Colour stuff
    ColourMap& getSegmentColourMap() { return m_segmentColourMap; }
    const ColourMap& getSegmentColourMap() const { return m_segmentColourMap; }
    void setSegmentColourMap(ColourMap &newmap);

    // General colourmap for non-segments
    //
    ColourMap& getGeneralColourMap() { return m_generalColourMap; }
    void setGeneralColourMap(ColourMap &newmap);


    //////
    //
    // QUANTIZERS

    /**
     * Return a quantizer that quantizes to the our most basic
     * units (i.e. a unit quantizer whose unit is our shortest
     * note duration).
     */
    const BasicQuantizer *getBasicQuantizer() const {
        return m_basicQuantizer;
    }

    /**
     * Return a quantizer that does quantization for notation
     * only.
     */
    const NotationQuantizer *getNotationQuantizer() const {
        return m_notationQuantizer;
    }


    //////
    //
    // REFRESH STATUS

    // delegate RefreshStatusArray API
    unsigned int getNewRefreshStatusId() {
        return m_refreshStatusArray.getNewRefreshStatusId();
    }
    
    RefreshStatus& getRefreshStatus(unsigned int id) {
        return m_refreshStatusArray.getRefreshStatus(id);
    }

    /// Set all refresh statuses to true
    void updateRefreshStatuses() {
        m_refreshStatusArray.updateRefreshStatuses();
    }


    /// Change notification mechanism.
    /// @see removeObserver()
    /// @see notifyTracksAdded()
    void    addObserver(CompositionObserver *obs) { m_observers.push_back(obs); }
    /// Change notification mechanism.
    /// @see addObserver()
    void removeObserver(CompositionObserver *obs) { m_observers.remove(obs); }

    /// Change notification mechanism.
    /**
     * See the various other "notify*" functions.
     *
     * These functions have been made public in the interests of improving
     * performance.  There are at least two main approaches to sending change
     * notifications to observers.  The first is to have each modifier
     * function (e.g. deleteTrack()) send the change notification.  The second
     * is to make the change notification functions public and let the code
     * that modifies the object also call the change notification function.
     * The first approach is convenient and less likely to be forgotten.  The
     * second approach has the advantage of performance in situations where
     * there are many changes being made at once.  All of the changes can be
     * made, then a single notification can be sent out once the changes are
     * complete.  With the first approach, many change notifications might get
     * sent out, each of which might lead to a potentially costly UI update.
     */
    void notifyTracksAdded(std::vector<TrackId> trackIds) const;
    /// Change notification mechanism.
    /**
     * @see notifyTracksAdded()
     */
    void notifyTrackChanged(Track*);
    /// Change notification mechanism.
    /**
     * @see notifyTracksAdded()
     */
    void notifyTracksDeleted(std::vector<TrackId> trackIds) const;
    /// Call when the selected track is changed.
    /**
     * @see setSelectedTrack()
     * @see notifyTracksAdded()
     */
    void notifyTrackSelectionChanged(TrackId) const;

    //////
    // LYRICS WITH REPEATED SEGMENTS
    void distributeVerses();

    //////
    // DEBUG FACILITIES
    void dump(std::ostream&, bool full=false) const;
    
protected:

    static const std::string TempoEventType; 
    static const PropertyName TempoProperty;
    static const PropertyName TargetTempoProperty;

    static const PropertyName NoAbsoluteTimeProperty;
    static const PropertyName BarNumberProperty;
    static const PropertyName TempoTimestampProperty;


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
 
    /**
     * Ensure the selected and record trackids still point to something valid
     * Must be called after deletion of detach of a track
     */
    void checkSelectedAndRecordTracks();
    TrackId getClosestValidTrackId(TrackId id) const;
    

    //--------------- Data members ---------------------------------
    //
    trackcontainer m_tracks;
    SegmentMultiSet m_segments;

    // The tracks we are armed for record on
    //
    recordtrackcontainer m_recordTracks;

    TrackId m_selectedTrackId;

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
    private:
        ReferenceSegment(const ReferenceSegment &);
        ReferenceSegment& operator=(const ReferenceSegment &);
    public:
        typedef Impl::iterator iterator;
        typedef Impl::size_type size_type;
        typedef Impl::difference_type difference_type;

        void clear();

        timeT getDuration() const;
        
        /// Inserts a single event, removing any existing one at that time
        iterator insertEvent(Event *e); // may throw Event::BadType

        void eraseEvent(Event *e);

        iterator findTime(timeT time);
        iterator findNearestTime(timeT time);

        iterator findRealTime(RealTime time);
        iterator findNearestRealTime(RealTime time);

        std::string getEventType() const { return m_eventType; }

    private:
        iterator find(Event *e);
        std::string m_eventType;
    };

    /// Contains time signature events
    mutable ReferenceSegment m_timeSigSegment;

    /// Contains tempo events
    mutable ReferenceSegment m_tempoSegment;

    /// affects m_timeSigSegment
    void calculateBarPositions() const;
    mutable bool m_barPositionsNeedCalculating;
    ReferenceSegment::iterator getTimeSignatureAtAux(timeT t) const;

    /// affects m_tempoSegment
    void calculateTempoTimestamps() const;
    mutable bool m_tempoTimestampsNeedCalculating;
    RealTime time2RealTime(timeT time, tempoT tempo) const;
    RealTime time2RealTime(timeT time, tempoT tempo,
                           timeT targetTempoTime, tempoT targetTempo) const;
    timeT realTime2Time(RealTime rtime, tempoT tempo) const;
    timeT realTime2Time(RealTime rtime, tempoT tempo,
                        timeT targetTempoTime, tempoT targetTempo) const;
    bool getTempoTarget(ReferenceSegment::const_iterator i,
                        tempoT &target,
                        timeT &targetTime) const;

    static RealTime getTempoTimestamp(const Event *e);
    static void setTempoTimestamp(Event *e, RealTime r);

    /// No more than one armed track per instrument.
    void enforceArmRule(const Track *track);

    typedef std::list<CompositionObserver *> ObserverSet;
    ObserverSet m_observers;

    void notifySegmentAdded(Segment *) const;
    void notifySegmentRemoved(Segment *) const;
    void notifySegmentRepeatChanged(Segment *, bool) const;
    void notifySegmentRepeatEndChanged(Segment *, timeT) const;
    void notifySegmentEventsTimingChanged(Segment *s, timeT delay, RealTime rtDelay) const;
    void notifySegmentTransposeChanged(Segment *s, int transpose) const;
    void notifySegmentTrackChanged(Segment *s, TrackId oldId, TrackId newId) const;
    void notifySegmentStartChanged(Segment *, timeT);
    void notifySegmentEndMarkerChange(Segment *s, bool shorten);
    void notifyEndMarkerChange(bool shorten) const;
    void notifyMetronomeChanged() const;
    void notifyTimeSignatureChanged() const;
    void notifyTempoChanged() const;
    void notifySelectedTrackChanged() const;
    void notifySourceDeletion() const;

    void clearVoiceCaches();
    void rebuildVoiceCaches() const;

    void updateExtremeTempos();

    BasicQuantizer                   *m_basicQuantizer;
    NotationQuantizer                *m_notationQuantizer;

    timeT                             m_position;
    tempoT                            m_defaultTempo;
    tempoT                            m_minTempo; // cached from tempo segment
    tempoT                            m_maxTempo; // cached from tempo segment

    // Notional Composition markers - these define buffers for the
    // start and end of the piece, Segments can still exist outside
    // of these markers - these are for visual and playback cueing.
    //
    timeT                             m_startMarker;
    timeT                             m_endMarker;
    bool                              m_autoExpand;

    static int                        m_defaultNbBars;

    // Loop start and end positions.  If they're both the same
    // value (usually 0) then there's no loop set.
    //
    timeT                             m_loopStart;
    timeT                             m_loopEnd;

    Configuration                     m_metadata;

    bool                              m_playMetronome;
    bool                              m_recordMetronome;

    RefreshStatusArray<RefreshStatus> m_refreshStatusArray;

    // User defined markers in the composition
    //
    markercontainer                   m_markers;

    // Trigger segments (unsorted segments fired by events elsewhere)
    //
    triggersegmentcontainer           m_triggerSegments;
    TriggerSegmentId                  m_nextTriggerSegmentId;
    
    ColourMap                         m_segmentColourMap;
    ColourMap                         m_generalColourMap;

    // Caches of segment voice indices and track voice counts
    //
    mutable std::map<TrackId, int>    m_trackVoiceCountCache;
    mutable std::map<const Segment *, int>  m_segmentVoiceIndexCache;
};


/// Base class for those that want notification of Composition changes.
/**
 * If you subclass from CompositionObserver, you can then attach to a
 * Composition to receive notification when something changes.
 *
 * Normally all the methods in this class would be pure virtual.  But
 * because there are so many, that imposes far too much work on the
 * subclass implementation in a case where it only really wants to
 * know about one thing, such as segments being deleted.  So we have
 * empty default implementations, and you'll just have to take a bit
 * more care to make sure you really are making the correct
 * declarations in the subclass.
 */
class CompositionObserver
{
public:
    CompositionObserver() : m_compositionDeleted(false)  { }
    
    virtual ~CompositionObserver()  { }
    
    /// A segment has been added to the Composition.
    virtual void segmentAdded(const Composition *, Segment *) { }

    /// A Segment has been removed from the Composition.
    /**
     * Called before the Segment is deleted.
     */
    virtual void segmentRemoved(const Composition *, Segment *) { }

    /// The Segment's repeat status has changed
    virtual void segmentRepeatChanged(const Composition *, Segment *, bool) { }

    /// The Segment's repeat end time has changed.
    virtual void segmentRepeatEndChanged(const Composition *, Segment *, timeT) { }

    /// The Segment's delay timing has changed.
    virtual void segmentEventsTimingChanged(const Composition *, Segment *,
                                            timeT /* delay */,
                                            RealTime /* rtDelay */) { }

    /// The Segment's transpose value has changed
    virtual void segmentTransposeChanged(const Composition *, Segment *,
                                         int /* transpose */) { }

    /// The Segment's start time has changed.
    virtual void segmentStartChanged(const Composition *, Segment *,
				     timeT /* newStartTime */) { }

    /// The Segment's end marker time has changed
    virtual void segmentEndMarkerChanged(const Composition *, Segment *,
                                         bool /* shorten */) { }

    /// The Segment's track has changed.
    virtual void segmentTrackChanged(const Composition *, Segment *,
                                     TrackId) { }

    /// The Composition's end time has been changed.
    /**
     * ??? rename: endTimeChanged() to differentiate from SegmentObserver.
     */
    virtual void endMarkerTimeChanged(const Composition *,
                                      bool /* shorten */) { }

    /// A different track has been selected.
    /**
     * This can happen when the user clicks on a track label, or presses
     * the up/down arrow keys to change which track is currently selected.
     *
     * See selectedTrackChanged().
     */
    virtual void trackSelectionChanged(const Composition *, TrackId) { }

    /// A Track has changed (instrument id, muted status...)
    virtual void trackChanged(const Composition *, Track *) { }

    virtual void tracksDeleted(const Composition *,
                               std::vector<TrackId> & /*trackIds*/) { }

    virtual void tracksAdded(const Composition *,
                             std::vector<TrackId> & /*trackIds*/) { }

    /// Some time signature has changed.
    virtual void timeSignatureChanged(const Composition *) { }
    
    /// Metronome status has changed (on/off)
    virtual void metronomeChanged(const Composition *) { }

    virtual void tempoChanged(const Composition *) { }
    
    /// Like trackChanged() but for the Track that is selected.
    /**
     * This avoids the need to check the TrackId with trackChanged().
     *
     * See trackChanged().
     *
     * ??? We can probably get rid of this easily and just use
     *     trackChanged().
     */
    virtual void selectedTrackChanged(const Composition *) { }


    /// Called from the Composition dtor.
    virtual void compositionDeleted(const Composition *) {
        m_compositionDeleted = true;
    }

    bool isCompositionDeleted()  { return m_compositionDeleted; }

protected:
    bool m_compositionDeleted;
};

}


#endif

