/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */


/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SEGMENT_H
#define RG_SEGMENT_H

#include <set>
#include <list>
#include <string>

#include "Track.h"
#include "Event.h"
#include "base/NotationTypes.h"
#include "RefreshStatus.h"
#include "RealTime.h"
#include "MidiProgram.h"

#include <QColor>

namespace Rosegarden
{

/// A refresh flag with a time range.
class SegmentRefreshStatus : public RefreshStatus
{
public:
    SegmentRefreshStatus() : m_from(0), m_to(0) {}

    void push(timeT from, timeT to);

    timeT from() const { return m_from; }
    timeT to()   const { return m_to; }

protected:
    timeT m_from;
    timeT m_to;
};

class SegmentObserver;
class Quantizer;
class BasicQuantizer;
class Composition;
class SegmentLinker;
class BasicCommand;

/// Container of Event objects.
/**
 * EventContainer is a precursor to Segment, used in code that needs
 * to store events but doesn't need all the ancillary data and
 * behaviors that Segment provides.
 **/
class EventContainer : public std::multiset<Event*, Event::EventCmp>
{
 public:
    iterator findEventOfType(iterator i, const std::string &type);
};

/// Container of Event objects.
/**
 * Segment is the container for a set of Events that are all played on
 * the same track.  Each event has an absolute starting time,
 * which is used as the index within the segment.  Multiple events may
 * have the same absolute time.
 *
 * (For example, chords are represented simply as a sequence of notes
 * that share a starting time.  The Segment can contain counterpoint --
 * notes that overlap, rather than starting and ending together -- but
 * in practice it's probably too hard to display so we should make
 * more than one Segment if we want to represent true counterpoint.)
 *
 * If you want to carry out notation-related editing operations on
 * a Segment, take a look at SegmentNotationHelper.  If you want to play a
 * Segment, try SegmentPerformanceHelper for duration calculations.
 *
 * The Segment owns the Events its items are pointing at.
 */
class Segment : public QObject, public EventContainer
{
  Q_OBJECT

public:

    /// A Segment contains either Internal representation or Audio
    typedef enum {
        Internal,
        Audio
    } SegmentType;

    /**
     * The manners in which a segment can participate in the
     * composition.  The ones other than `normal' are for various
     * forms of dummies for display or editing.
     */
    typedef enum {
        normal,
        editableClone,
        readOnly,
        justForShow,
    } Participation;

    /**
     * Construct a Segment of a given type with a given formal starting time.
     */
    Segment(SegmentType segmentType = Internal,
            timeT startTime = 0);

    /**
     * Virtual copy constructor interface, in case this is actually a linked segment
     * (or potentially any other type derived from Segment)
     * @param deep true to return a deep copy, false to return just a segment
     * containing a copy of the events of this
     */
    Segment* clone(bool deep = true) const
    {
        if(deep) { return cloneImpl(); }
        else { return new Segment(*this); }
    }

protected:
    /**
     * Virtual copy constructor implementation
     */
    virtual Segment* cloneImpl() const;

    /**
     * Copy constructor - protected to encourage use of the clone function
     * when a copy is required
     */
    Segment(const Segment&);

public:
    virtual ~Segment();


    //////
    //
    // BASIC SEGMENT ATTRIBUTES

    /**
     * Get the Segment type (Internal or Audio)
     */
    SegmentType getType() const { return m_type; }

    /**
     * Get the element name this class will have when serialised
     */
    virtual QString getXmlElementName() const { return "segment"; }

    /**
     * Note that a Segment does not have to be in a Composition;
     * if it isn't, this will return zero
     */
    Composition *getComposition() const {
        if (isTmp()) {
            return getRealSegment()->getComposition();
        } else {
            return m_composition;
        }
    }

    // Get the track ID this Segment is associated with.
    /**
     * rename: getTrackId()
     */
    TrackId getTrack() const { return m_trackId; }

    /// Set the track ID this Segment is associated with.
    /**
     * rename: setTrackId()
     */
    void setTrack(TrackId trackId);

    /**
     * Values other than `normal' make the segment act like it's not
     * really in the composition.  Eg, setTrack won't actually move it
     * to track N.  Actually attaching/detaching it from the
     * Composition is not handled by this.
     */
    void setParticipation(Participation participation)
    { m_participation = participation; }

    Participation getParticipation(void)
    { return m_participation; }

    // label
    //
    void setLabel(const std::string &label);
    std::string getLabel() const { return m_label; }

    // Colour information
    void setColourIndex(const unsigned int input);
    unsigned int getColourIndex() const { return m_colourIndex; }
    /// Get a high-contrast color to use for segment previews
    QColor getPreviewColour() const;

    /**
     * Returns a numeric id of some sort
     * The id is guaranteed to be unique within the segment, but not to
     * have any other interesting properties
     */
    int getNextId() const;

    /**
     * Returns a MIDI pitch representing the highest suggested playable note for
     * notation contained in this segment, as a convenience reminder to composers.
     *
     * This property, and its corresponding lowest note counterpart, initialize by
     * default such that no limitation is imposed.  (lowest = 0, highest = 127)
     */
    int getHighestPlayable() { return m_highestPlayable; }

    /**
     * Set the highest suggested playable note for this segment
     */
    void setHighestPlayable(int pitch) { m_highestPlayable = pitch; }

    /**
     * Returns a MIDI pitch representing the lowest suggested playable note for
     * notation contained in this segment, as a convenience reminder to composers
     */
    int getLowestPlayable() { return m_lowestPlayable; }

    /**
     * Set the highest suggested playable note for this segment
     */
    void setLowestPlayable(int pitch) { m_lowestPlayable = pitch; }



    //////
    //
    // TIME & DURATION VALUES

    /**
     * Return the start time of the Segment.  For a non-audio
     * Segment, this is the start time of the first event in it.
     */
    timeT getStartTime() const;

    /**
     * Return the start time of the Segment, clipped so that if there is a
     * Composition, this will not return a time earlier than the Composition
     * start marker.  (Used to constrain edit views non-destructively.)
     */
    timeT getClippedStartTime() const;

    /**
     * Return the nominal end time of the Segment.  This must
     * be the same as or earlier than the getEndTime() value.
     * The return value will not necessarily be that last set
     * with setEndMarkerTime, as if there is a Composition its
     * end marker will also be used for clipping.
     * comp = true truncates endmarker time based on composition
     * end.  comp = false disregards composition end.
     */
    timeT getEndMarkerTime(bool comp = true) const;

    /**
     * Return the time of the end of the last event stored in the
     * Segment.  This time may be outside the audible/editable
     * range of the Segment, depending on the location of the end
     * marker.
     */
    timeT getEndTime() const;

    /**
     * Shift the start time of the Segment by moving the start
     * times of all the events in the Segment.
     */
    void setStartTime(timeT);

    /**
     * DO NOT USE THIS METHOD
     * Simple accessor for the m_startTime member. Used by
     * Composition#setSegmentStartTime
     */
    void setStartTimeDataMember(timeT t) { m_startTime = t; }

    /**
     * Set the end marker (nominal end time) of this Segment.
     *
     * If the given time is later than the current end of the
     * Segment's storage, extend the Segment by filling it with
     * rests; if earlier, simply move the end marker.  The end
     * marker time may not precede the start time.
     */
    void setEndMarkerTime(timeT);

    /**
     * Set the end time of the Segment.
     *
     * If the given time is later than the current end of the
     * Segment's storage, extend the Segment by filling it with
     * rests; if earlier, shorten it by throwing away events as
     * necessary (though do not truncate any events) and also move
     * the end marker to the given time.  The end time may not
     * precede the start time.
     *
     * Note that simply inserting an event beyond the end of the
     * Segment will also change the end time, although it does
     * not fill with rests in the desirable way.
     *
     * Consider using setEndMarkerTime in preference to this.
     */
    void setEndTime(timeT);

    /**
     * Return an iterator pointing to the nominal end of the
     * Segment.  This may be earlier than the end() iterator.
     */
    iterator getEndMarker() const;

    /**
     * Return true if the given iterator points earlier in the
     * Segment than the nominal end marker.  You can use this
     * as an extent test in code such as
     *
     *  while (segment.isBeforeEndMarker(my_iterator)) {
     *      // ...
     *      ++my_iterator;
     *  }
     *
     * It is not generally safe to write
     *
     *  while (my_iterator != segment.getEndMarker()) {
     *      // ...
     *      ++my_iterator;
     *  }
     *
     * as the loop will not terminate if my_iterator's initial
     * value is already beyond the end marker.  (Also takes the
     * Composition's end marker into account.)
     */
    bool isBeforeEndMarker(const_iterator) const;

    /**
     * Remove the end marker, thus making the Segment end
     * at its storage end time (unless the Composition's
     * end marker is earlier).
     */
    void clearEndMarker();

    /**
     * Return the end marker in raw form, that is, a pointer to
     * its value or null if none is set.  Does not take the
     * composition's end marker into account.
     */
    const timeT *getRawEndMarkerTime() const;

    //////
    //
    // QUANTIZATION

    /**
     * Switch quantization on or off.
     */
    void setQuantization(bool quantize);

    /**
     * Find out whether quantization is on or off.
     */
    bool hasQuantization() const;

    /**
     * Set the quantization level.
     * (This does not switch quantization on, if it's currently off,
     * it only changes the level that will be used when it's next
     * switched on.)
     */
    void setQuantizeLevel(timeT unit);

    /**
     * Get the quantizer currently in (or not in) use.
     */
    const BasicQuantizer *getQuantizer() const;



    //////
    //
    // EVENT MANIPULATION

    /// Insert a single Event
    iterator insert(Event *e);

    /// Erase a single Event
    void erase(iterator pos);

    /// Erase a set of Events
    void erase(iterator from, iterator to);

    /// Clear the segment.
    void clear() { erase(begin(), end()); }

    /**
     * Looks up an Event and if it finds it, erases it.
     * @return true if the event was found and erased, false otherwise.
     */
    bool eraseSingle(Event*);

    /**
     * Returns an iterator pointing to that specific element,
     * end() otherwise
     */
    iterator findSingle(Event*);

    const_iterator findSingle(Event *e) const {
        return const_iterator(((Segment *)this)->findSingle(e));
    }

    /**
     * Returns an iterator pointing to the first element starting at
     * or beyond the given absolute time
     */
    iterator findTime(timeT time);

    const_iterator findTime(timeT time) const {
        return const_iterator(((Segment *)this)->findTime(time));
    }

    /**
     * Returns an iterator pointing to the first element starting at
     * or before the given absolute time (so returns end() if the
     * time precedes the first event, not if it follows the last one)
     */
    iterator findNearestTime(timeT time);

    const_iterator findNearestTime(timeT time) const {
        return const_iterator(((Segment *)this)->findNearestTime(time));
    }


    //////
    //
    // ADVANCED, ESOTERIC, or PLAIN STUPID MANIPULATION
    
    /**
     * Returns the range [start, end[ of events which are at absoluteTime
     */
    void getTimeSlice(timeT absoluteTime, iterator &start, iterator &end);

    /**
     * Returns the range [start, end[ of events which are at absoluteTime
     */
    void getTimeSlice(timeT absoluteTime, const_iterator &start, const_iterator &end) const;

    /**
     * Return the starting time of the bar that contains time t.  This
     * differs from Composition's bar methods in that it will truncate
     * to the start and end times of this Segment, and is guaranteed
     * to return the start time of a bar that is at least partially
     * within this Segment.
     *
     * (See Composition for most of the generally useful bar methods.)
     */
    timeT getBarStartForTime(timeT t) const;

    /**
     * Return the ending time of the bar that contains time t.  This
     * differs from Composition's bar methods in that it will truncate
     * to the start and end times of this Segment, and is guaranteed
     * to return the end time of a bar that is at least partially
     * within this Segment.
     *
     * (See Composition for most of the generally useful bar methods.)
     */
    timeT getBarEndForTime(timeT t) const;

    /**
     * Fill up the segment with rests, from the end of the last event
     * currently on the segment to the endTime given.  Actually, this
     * does much the same as setEndTime does when it extends a segment.
     */
    void fillWithRests(timeT endTime);

    /**
     * Fill up a section within a segment with rests, from the
     * startTime given to the endTime given.  This may be useful if
     * you have a pathological segment that contains notes already but
     * not rests, but it is is likely to be dangerous unless you're
     * quite careful about making sure the given range doesn't overlap
     * any notes.
     */
    void fillWithRests(timeT startTime, timeT endTime);

    /**
     * For each series of contiguous rests found between the start and
     * end time, replace the series of rests with another series of
     * the same duration but composed of the theoretically "correct"
     * rest durations to fill the gap, in the current time signature.
     * The start and end time should be the raw absolute times of the
     * events, not the notation-quantized versions, although the code
     * will use the notation quantizations if it finds them.
     */
    void normalizeRests(timeT startTime, timeT endTime);

    /**
     * Return the clef in effect at the given time.  This is a
     * reasonably quick call.
     */
    Clef getClefAtTime(timeT time) const;

    /**
     * Return the clef in effect at the given time, and set ctime to
     * the time of the clef change.  This is a reasonably quick call.
     */
    Clef getClefAtTime(timeT time, timeT &ctime) const;

    /**
     * If there is another clef change following the given time,
     * return its time in nextTime and return true.  If there is no
     * further clef change after the given time, return false.  This
     * is a reasonably quick call.
     */
    bool getNextClefTime(timeT time, timeT &nextTime) const;

    /**
     * Return the key signature in effect at the given time.  This is
     * a reasonably quick call.
     */
    Key getKeyAtTime(timeT time) const;

    /**
     * Return the key signature in effect at the given time, and set
     * ktime to the time of the key change.  This is a reasonably
     * quick call.
     */
    Key getKeyAtTime(timeT time, timeT &ktime) const;

    /**
     * If there is another key change following the given time, return
     * its time in nextTime and return true.  If there is no further
     * key change after the given time, return false.  This is a
     * reasonably quick call.
     */
    bool getNextKeyTime(timeT time, timeT &nextTime) const;

    /**
     * Return the clef and key signature in effect at the beginning of the
     * segment using the following rules :
     *
     *    - Return the default clef if no clef change is preceding the first
     *      note or rest event,
     *    - else return the first clef event in the segment,
     *    - else return the default clef if the segment has no note event nor
     *      clef change in it.
     *
     *    - Use the same rules with the key signature.
     */
    void getFirstClefAndKey(Clef &clef, Key &key);

   /**
    * If segment doesn't begin with a clef and a key signature, insert
    * default clef and/or key signature as needed.
    */
    void enforceBeginWithClefAndKey();
    
    /**
     * Stop sending move or resize notifications to the observers.
     * (May be useful to avoid sending lot of unnecessary resize notifications
     * when a segment is deleted then rebuild event by event while a linked
     * segment is processed).
     * Should be used with caution!
     */
    void lockResizeNotifications();
    
    /**
     * Revert lockResizeNotifications() effect. If segment has been move
     * or resized, send one, and only one, notification to the observers.
     * Should only be called after lockResizeNotifications() has been called.
     * Nested lock/unlock calls are not allowed currently.
     */ 
    void unlockResizeNotifications();    
    
    /**
     * YG: This one is only for debug
     */
    void dumpObservers();


    //////
    //
    // REPEAT, DELAY, TRANSPOSE

    // Is this Segment repeating?
    //
    bool isRepeating() const { return m_repeating; }
    void setRepeating(bool value);


    /**
     * If this Segment is repeating, calculate and return the time at
     * which the repeating stops.
     * This is the time of the first part of another Segment on the same
     * Track, if any, which follows the end of this Segment.
     * If there is not such a Segment, instead return the end time of the
     * Segment.
     */
    timeT getRepeatEndTime() const;

    timeT getDelay() const { return m_delay; }
    void setDelay(timeT delay);

    RealTime getRealTimeDelay() const { return m_realTimeDelay; }
    void setRealTimeDelay(RealTime delay);

    int getTranspose() const { return m_transpose; }
    void setTranspose(int transpose);

    /**
     * Return the number of verses in the lyrics
     * (call countVerses() when needed)
     */
    int getVerseCount();
    
    /**
     * (Re)compute the internally remembered verse count
     */
    void countVerses();

    /**
     * Return the verse index of lyrics associated with the segment if
     * this one is unfolded in the notation editor.
     * (ie the verse which have to be written under the staff among all the
     * verses included as text of lyrics inside the segment)
     */
    int getVerse() const { return m_verse; }

    /**
     * Return the verse index of lyrics associated with the segment if
     * this one is unfolded in the notation editor.
     * As getVerse(), but return again the first verses when going beyond
     * the total verses count.
     */
    int getVerseWrapped();

    /**
     * Used to set the value returned by getVerse().
     * Should only be called from a method recomputing verse for all
     * the segments in composition (or for the temporary segments in
     * a notation editor)
     */
    void setVerse (int verse) { m_verse = verse; }



    //////
    //
    // AUDIO

    // Get and set Audio file Id (see the AudioFileManager)
    //
    unsigned int getAudioFileId() const { return m_audioFileId; }
    void setAudioFileId(unsigned int id);

    unsigned int getUnstretchedFileId() const { return m_unstretchedFileId; }
    void setUnstretchedFileId(unsigned int id);

    float getStretchRatio() const { return m_stretchRatio; }
    void setStretchRatio(float ratio);

    // The audio start and end times tell us how far into
    // audio file "m_audioFileId" this Segment starts and
    // how far into the sample the Segment finishes.
    //
    RealTime getAudioStartTime() const { return m_audioStartTime; }
    RealTime getAudioEndTime() const { return m_audioEndTime; }
    void setAudioStartTime(const RealTime &time);
    void setAudioEndTime(const RealTime &time);

    bool isAutoFading() const { return m_autoFade; }
    void setAutoFade(bool value);

    RealTime getFadeInTime() const { return m_fadeInTime; }
    void setFadeInTime(const RealTime &time);

    RealTime getFadeOutTime() const { return m_fadeOutTime; }
    void setFadeOutTime(const RealTime &time);

    //////
    //
    // MISCELLANEOUS

    /// Should only be called by Composition
    void setComposition(Composition *composition) {
        m_composition = composition;
    }

    // The runtime id for this segment
    //
    int getRuntimeId() const { return m_runtimeSegmentId; }

    // Grid size for matrix view (and others probably)
    //
    void setSnapGridSize(int size) { m_snapGridSize = size; }
    int getSnapGridSize() const { return m_snapGridSize; }

    // Other view features we might want to set on this Segment
    //
    void setViewFeatures(int features) { m_viewFeatures = features; }
    int getViewFeatures() const { return m_viewFeatures; }

    /**
     * The compare class used by Composition
     */
    struct SegmentCmp
    {
        bool operator()(const Segment* a, const Segment* b) const
        {
            if (a->getTrack() == b->getTrack())
                return a->getStartTime() < b->getStartTime();

            return a->getTrack() < b->getTrack();
        }
    };

    typedef std::multiset<Segment*, Segment::SegmentCmp> segmentcontainer;

    // Get the segments in the current composition.
    static segmentcontainer& getCompositionSegments(void);
    
    void  addObserver(SegmentObserver *obs) { m_observers.push_back(obs); }
    void removeObserver(SegmentObserver *obs) { m_observers.remove(obs); }

    // List of visible EventRulers attached to this segment
    //
    class EventRuler
    {
    public:
        EventRuler(const std::string &type, int controllerValue, bool active):
            m_type(type), m_controllerValue(controllerValue), m_active(active) {;}

        std::string m_type;            // Event Type
        int         m_controllerValue; // if controller event, then which value
        bool        m_active;          // is this Ruler active?
    };

    typedef std::vector<EventRuler*> EventRulerList;
    typedef std::vector<EventRuler*>::iterator EventRulerListIterator;
    typedef std::vector<EventRuler*>::const_iterator EventRulerListConstIterator;

    EventRulerList& getEventRulerList() { return m_eventRulerList; }
    EventRuler* getEventRuler(const std::string &type, int controllerValue = -1);

    void addEventRuler(const std::string &type, int controllerValue = -1, bool active = 0);
    bool deleteEventRuler(const std::string &type, int controllerValue = -1);

    //////
    //
    // REFRESH STATUS

    // delegate part of the RefreshStatusArray API

    unsigned int getNewRefreshStatusId() {
        return m_refreshStatusArray.getNewRefreshStatusId();
    }

    SegmentRefreshStatus &getRefreshStatus(unsigned int id) {
        return m_refreshStatusArray.getRefreshStatus(id);
    }

    void updateRefreshStatuses(timeT startTime, timeT endTime);

    //////
    //
    // LINKED SEGMENTS

   /**
    * Return true if the segment is connected to a SegmentLinker.
    * This doesn't always mean that the segment is really linked : 
    *    - The segment may be the only one referenced by the SegmentLinker.
    *      (Probaly this should not be, but nevertheless is not impossible.)
    *    - The segment is a repeating one opened in the notation editor.
    *      It is a linked segment, but linked with temporary segments which
    *      composition doesn't know.
    */
    bool isLinked() const { return m_segmentLinker; }

    /**
     * Return true if the segment is link to at least one other segment
     * which is not a temporary one nor being outside ofthe composition
     * (i.e. deleted).
     */ 
    bool isTrulyLinked() const;

    /**
     * Return true if the segment is "truly link" and doesn't embbed any
     * local change (as transpositon...).
     * This method is intended to help exporting linked segments as repeat with
     * volta in LilyPond.
     */ 
    bool isPlainlyLinked() const;

    /**
     * Return true if the given segment is linked to this.
     */ 
    bool isLinkedTo(Segment *) const;

    /**
     * Return true if the given segment is a plain link linked to the current
     * object which is equally a plain link
     */ 
    bool isPlainlyLinkedTo(Segment *) const;

    SegmentLinker * getLinker() const { return m_segmentLinker; }
    void setLinker(SegmentLinker *linker) { m_segmentLinker = linker; }

    struct LinkTransposeParams
    {
        LinkTransposeParams() : m_changeKey(false), m_steps(0),
            m_semitones(0), m_transposeSegmentBack(false) { }
        LinkTransposeParams(bool chKey, int steps, int stones, bool transBack) :
            m_changeKey(chKey), m_steps(steps), m_semitones(stones),
            m_transposeSegmentBack(transBack) { }
        bool m_changeKey;
        int m_steps;
        int m_semitones;
        bool m_transposeSegmentBack;
    };
    LinkTransposeParams getLinkTransposeParams() const {
                                                return m_linkTransposeParams; }
    void setLinkTransposeParams(LinkTransposeParams params) {
                                              m_linkTransposeParams = params; }

    /**
     * Set the segment as a temporary one.
     * A temporary segment is always linked to some "real segment" and is not
     * known from the composition. Such a segment is only intended to
     * live inside one view.
     */
    void setTmp();

    /**
     * Report if the segment is "tmp"
     */
    bool isTmp() const { return m_isTmp; }

    /**
     * Set the segment to display as greyed out, a visual indication
     * that it is temporary or read-only.
     **/
    void setGreyOut(void);
    
    /**
     * Set the current segment as the reference of the linked segment group and
     * return true.
     * Return false if the segment is not linked and can't have a reference.
     */
    bool setAsReference();

    /**
     * Return pointer to current segment if not linked else return pointer
     * to segment used as reference.
     * May return 0 if segment is linked but no reference is defined.
     */
    Segment * getRealSegment();

    /**
     * Return pointer to current segment if not linked else return pointer
     * to segment used as reference.
     * May return 0 if segment is linked but no reference is defined.
     */
    const Segment * getRealSegment() const;
    


private:
    void checkInsertAsClefKey(Event *e) const;
    
    Composition *m_composition; // owns me, if it exists

    timeT  m_startTime;
    timeT *m_endMarkerTime;     // points to end time, or null if none
    timeT  m_endTime;

    void updateEndTime();       // called after erase of item at end

    TrackId m_trackId;
    SegmentType m_type;         // identifies Segment type
    std::string m_label;        // segment label

    unsigned int m_colourIndex; // identifies Colour Index (default == 0)

    mutable int m_id; // not id of Segment, but a value for return by getNextId

    unsigned int m_audioFileId; // audio file ID (see AudioFileManager)
    unsigned int m_unstretchedFileId;
    float m_stretchRatio;
    RealTime m_audioStartTime;   // start time relative to start of audio file
    RealTime m_audioEndTime;     // end time relative to start of audio file

    bool m_repeating;           // is this segment repeating?

    BasicQuantizer *const m_quantizer;
    bool m_quantize;

    int m_transpose;            // all Events tranpose
    timeT m_delay;              // all Events delay
    RealTime m_realTimeDelay;   // all Events delay (the delays are cumulative)

    int m_highestPlayable;      // suggestion for highest playable note (notation)
    int m_lowestPlayable;       // suggestion for lowest playable note (notation)

    RefreshStatusArray<SegmentRefreshStatus> m_refreshStatusArray;

    struct ClefKeyCmp {
        bool operator()(const Event *e1, const Event *e2) const;
    };
    typedef std::multiset<Event*, ClefKeyCmp> ClefKeyList;
    mutable ClefKeyList *m_clefKeyList;

    // EventRulers currently selected as visible on this segment
    //
    EventRulerList                m_eventRulerList;

private: // stuff to support SegmentObservers

    typedef std::list<SegmentObserver *> ObserverSet;
    ObserverSet m_observers;

    void notifyAdd(Event *) const;
    void notifyRemove(Event *) const;
    void notifyAppearanceChange() const;
    void notifyStartChanged(timeT);
    void notifyEndMarkerChange(bool shorten);
    void notifyTransposeChange();
    void notifySourceDeletion() const;
    
    bool m_notifyResizeLocked;
    timeT m_memoStart;
    timeT *m_memoEndMarkerTime;

signals:
    void contentsChanged(timeT start, timeT end);
 public:
    void signalChanged(timeT start, timeT end)
    { emit contentsChanged(start,end); }
    
private:

    // assignment operator not provided
    Segment &operator=(const Segment &);

    // Used for mapping the segment to runtime things like PlayableAudioFiles at
    // the sequencer.
    //
    int     m_runtimeSegmentId;

    // Remember the last used snap grid size for this segment
    //
    int     m_snapGridSize;

    // Switch for other view-specific features we want to remember in the segment
    //
    int     m_viewFeatures;

    // Audio autofading
    //
    bool      m_autoFade;
    RealTime  m_fadeInTime;
    RealTime  m_fadeOutTime;

    // Linked segments
    SegmentLinker *m_segmentLinker;
    LinkTransposeParams m_linkTransposeParams;
    bool m_isTmp;      // Mark a segment (must be a link) as temporary

    /**
     * Values other than `normal' make the segment act like it's not
     * really in the composition.  Eg, setTrack won't actually move it
     * to track N.
     */
    Participation m_participation;
    int m_verseCount;  // -1 means not computed still
    int m_verse;       // Used to distribute lyrics among repeated segments
};

typedef Segment::segmentcontainer segmentcontainer;

/// Base class interface for Segment notifications.
/**
 * See Segment::addObserver() and Segment::m_observers.
 */
class SegmentObserver
{
public:
    virtual ~SegmentObserver() {}

    /// Called after an event has been added to the segment.
    virtual void eventAdded(const Segment *, Event *) { }

    /**
     * Called after the event has been removed from the segment,
     * and just before it is deleted
     */
    virtual void eventRemoved(const Segment *, Event *) { }

    // Exists just for performance reasons.  Called in lieu of calling
    // eventAdded or eventRemoved many times.  The default just calls
    // both eventRemoved() and eventAdded() on every event.
    virtual void AllEventsChanged(const Segment *);

    /**
     * Called after a change in the segment that will change the way its displays,
     * like a label change for instance
     */
    virtual void appearanceChanged(const Segment *) { }

    /// Called after a change that affects the start time of the segment.
    virtual void startChanged(const Segment *, timeT) { }

    /// Called after the segment's end marker time has been changed.
    /**
     * @param shorten true if the marker change shortens the segment's duration
     */
    virtual void endMarkerTimeChanged(const Segment *, bool /*shorten*/) { }

    /// Called after a change of the segment transposition.
    virtual void transposeChanged(const Segment *, int /*transpose*/) { }

    /**
     * Called from the segment dtor
     * MUST BE IMPLEMENTED BY ALL OBSERVERS
     */
    virtual void segmentDeleted(const Segment *) = 0;
};


class SegmentHelper
{
protected:
    SegmentHelper(Segment &t) : m_segment(t) { }
    virtual ~SegmentHelper();

    typedef Segment::iterator iterator;

    Segment &segment() { return m_segment; }

    Segment::iterator begin() { return segment().begin(); }
    Segment::iterator end()   { return segment().end();   }

    bool isBeforeEndMarker(Segment::const_iterator i) {
        return segment().isBeforeEndMarker(i);
    }

    Segment::iterator insert(Event *e) { return segment().insert(e); }
    void erase(Segment::iterator i)    { segment().erase(i); }

private:
    Segment &m_segment;
};

}


#endif
