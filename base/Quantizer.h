// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#ifndef QUANTIZER_H
#define QUANTIZER_H

#include "Segment.h"
#include "Event.h"
#include "NotationTypes.h"
#include "FastVector.h"
#include <string>

namespace Rosegarden {

class EventSelection;

/**
   The Quantizer class rounds the starting times and durations of note
   and rest events according to one of a set of possible criteria.
*/

class Quantizer
{
    // define the Quantizer API

public:
    virtual ~Quantizer();

    /**
     * Quantize a Segment.
     */
    void quantize(Segment *) const;

    /**
     * Quantize a section of a Segment.
     */
    void quantize(Segment *,
		  Segment::iterator from,
		  Segment::iterator to) const;

    /**
     * Quantize an EventSelection.
     */
    void quantize(EventSelection *);

    /**
     * Quantize a section of a Segment, and force the quantized
     * results into the formal absolute time and duration of
     * the events.  This is a destructive operation that should
     * not be carried out except on a user's explicit request.
     * (If target is RawEventData, this will do nothing besides
     * quantize.  In this case, but no other, unquantize will
     * still work afterwards.)
     */
    void fixQuantizedValues(Segment *,
			    Segment::iterator from,
			    Segment::iterator to) const;

    /**
     * Return the quantized duration of the event if it has been
     * quantized -- otherwise just return the unquantized duration.
     * Do not modify the event.
     */
    timeT getQuantizedDuration(const Event *e) const;

    /**
     * Return the quantized absolute time of the event if it has been
     * quantized -- otherwise just return the unquantized time.  Do
     * not modify the event.
     */
    timeT getQuantizedAbsoluteTime(const Event *e) const;

    /**
     * Return the unquantized absolute time of the event --
     * the absolute time that would be restored by a call to
     * unquantize.
     */
    timeT getUnquantizedAbsoluteTime(Event *e) const;

    /**
     * Return the unquantized absolute time of the event --
     * the absolute time that would be restored by a call to
     * unquantize.
     */
    timeT getUnquantizedDuration(Event *e) const;

    /**
     * Unquantize all events in the given range, for this
     * quantizer.  Properties set by other quantizers with
     * different propertyNamePrefix values will remain.
     */
    void unquantize(Segment *,
		    Segment::iterator from, Segment::iterator to) const;

    /**
     * Unquantize a selection of Events
     */
    void unquantize(EventSelection *) const;

    static const std::string RawEventData;
    static const std::string DefaultTarget;
    static const std::string GlobalSource;
    static const std::string NotationPrefix;

protected:
    /**
     * \arg source, target : Description of where to find the
     * times to be quantized, and where to put the quantized results.
     * 
     * These may be strings, specifying a prefix for the names
     * of properties to contain the timings, or may be the special
     * value RawEventData in which case the event's absolute time
     * and duration will be used instead of properties.
     * 
     * If source specifies a property prefix for properties that are
     * found not to exist, they will be pre-filled from the original
     * timings in the target values before being quantized and then
     * set back into the target.  (This permits a quantizer to write
     * directly into the Event's absolute time and duration without
     * losing the original values, because they are backed up
     * automatically into the source properties.)
     * 
     * Note that because it's impossible to modify the duration or
     * absolute time of an event after construction, if target is
     * RawEventData the quantizer must re-construct each event in
     * order to adjust its timings.  This operation (deliberately)
     * loses any non-persistent properties in the events.  This
     * does not happen if target is a property prefix.
     *
     *   Examples:
     *
     *   -- if source == RawEventData and target == "MyPrefix",
     *   values will be read from the event's absolute time and
     *   duration, quantized, and written into MyPrefixAbsoluteTime
     *   and MyPrefixDuration properties on the event.  A call to
     *   unquantize will simply delete these properties.
     *
     *   -- if source == "MyPrefix" and target == RawEventData,
     *   the MyPrefixAbsoluteTime and MyPrefixDuration will be
     *   populated if necessary from the event's absolute time and
     *   duration, and then quantized and written back into the
     *   event's values.  A call to unquantize will write the
     *   MyPrefix-property timings back into the event's values,
     *   and delete the MyPrefix properties.
     *
     *   -- if source == "YourPrefix" and target == "MyPrefix",
     *   values will be read from YourPrefixAbsoluteTime and
     *   YourPrefixDuration, quantized, and written into the
     *   MyPrefix-properties.  This may be useful for piggybacking
     *   onto another quantizer's output.
     *
     *   -- if source == RawEventData and target == RawEventData,
     *   values will be read from the event's absolute time and
     *   duration, quantized, and written back to these values.
     */
    Quantizer(std::string source, std::string target);

    /**
     * If only target is supplied, source is deduced appropriately
     * as GlobalSource if target == RawEventData and RawEventData
     * otherwise.
     */
    Quantizer(std::string target);

    /**
     * To implement a subclass of Quantizer, you should
     * override either quantizeSingle (if your quantizer is simple
     * enough only to have to look at a single event at a time) or
     * quantizeRange.  The default implementation of quantizeRange
     * simply calls quantizeSingle on each non-rest event in turn.
     * The default implementation of quantizeSingle, as you see,
     * does nothing.
     * 
     * Note that implementations of these methods should call
     * getFromSource and setToTarget to get and set the unquantized
     * and quantized data; they should not query the event properties
     * or timings directly.
     *
     * NOTE: It is vital that ordering is maintained after
     * quantization.  That is, an event whose absolute time quantizes
     * to a time t must appear in the original segment before all
     * events whose times quantize to greater than t.  This means you
     * must quantize the absolute times of non-note events as well as
     * notes.  You don't need to worry about quantizing rests,
     * however; they're only used for notation and will be
     * automatically recalculated if the notation quantization values
     * are seen to change.
     */
    virtual void quantizeSingle(Segment *,
				Segment::iterator) const { }

    /**
     * See note for quantizeSingle.
     */
    virtual void quantizeRange(Segment *,
			       Segment::iterator,
			       Segment::iterator) const;

    std::string m_source;
    std::string m_target;

    enum ValueType { AbsoluteTimeValue = 0, DurationValue = 1 };

    PropertyName m_sourceProperties[2];
    PropertyName m_targetProperties[2];

public: // should be protected, but gcc-2.95 doesn't like allowing NotationQuantizer::m_impl to access them
    timeT getFromSource(Event *, ValueType) const;
    timeT getFromTarget(Event *, ValueType) const;
    void setToTarget(Segment *, Segment::iterator, timeT t, timeT d) const;
    mutable FastVector<Event *> m_toInsert;

protected:
    void removeProperties(Event *) const;
    void removeTargetProperties(Event *) const;
    void makePropertyNames();

    void insertNewEvents(Segment *) const;

private: // not provided
    Quantizer(const Quantizer &);
    Quantizer &operator=(const Quantizer &);
    bool operator==(const Quantizer &) const;
    bool operator!=(const Quantizer & c) const;
};


class BasicQuantizer : public Quantizer
{
public:
    BasicQuantizer(timeT unit = -1, bool doDurations = false);
    BasicQuantizer(std::string target,
		   timeT unit = -1, bool doDurations = false);
    BasicQuantizer(const BasicQuantizer &);
    virtual ~BasicQuantizer();

    void setUnit(timeT unit) { m_unit = unit; }
    timeT getUnit() const { return m_unit; }

    void setDoDurations(bool doDurations) { m_durations = doDurations; }
    bool getDoDurations() const { return m_durations; }

    /**
     * Return the standard quantization units in descending order of
     * unit duration
     */
    static std::vector<timeT> getStandardQuantizations();

    /**
     * Study the given segment; if all the events in it have times
     * that match one or more of the standard quantizations, return
     * the longest standard quantization unit to match.  Otherwise
     * return 0.
     */
    static timeT getStandardQuantization(Segment *);

    /**
     * Study the given selection; if all the events in it have times
     * that match one or more of the standard quantizations, return
     * the longest standard quantization unit to match.  Otherwise
     * return 0.
     */
    static timeT getStandardQuantization(EventSelection *);

protected:
    virtual void quantizeSingle(Segment *,
				Segment::iterator) const;

private:
    BasicQuantizer &operator=(const BasicQuantizer &); // not provided

    timeT m_unit;
    bool m_durations;

    static std::vector<timeT> m_standardQuantizations;
    static void checkStandardQuantizations();
    static timeT getUnitFor(Event *);
};


class NotationQuantizer : public Quantizer
{
public:
    NotationQuantizer();
    NotationQuantizer(std::string target);
    NotationQuantizer(const NotationQuantizer &);
    ~NotationQuantizer();

    /**
     * Set the absolute time minimum unit.  Default is demisemiquaver.
     */
    void  setUnit(timeT);
    timeT getUnit() const;

    /**
     * Set the simplicity factor.  This controls the relative "pull"
     * towards larger units and more obvious beats in placing notes.
     * The value 10 means no pull to larger units, lower values mean
     * an active pull away from them.  Default is 13.
     */
    void setSimplicityFactor(int);
    int  getSimplicityFactor() const;

    /**
     * Set the maximum size of tuplet group.  2 = two-in-the-time-of-three
     * groupings, 3 = triplets, etc.  Default is 3.  Set <2 to switch off
     * tuplets altogether.
     */
    void setMaxTuplet(int);
    int  getMaxTuplet() const;

    /**
     * Set whether to add articulations (staccato, tenuto, slurs).
     * Default is true.  Doesn't affect quantization, only the marks
     * that are added to quantized notes.
     */
    void setArticulate(bool);
    bool getArticulate() const;

protected:
    virtual void quantizeRange(Segment *,
			       Segment::iterator,
			       Segment::iterator) const;

protected:
    // avoid having to rebuild absolutely everything each time we
    // tweak the implementation
    class Impl;
    Impl *m_impl;

private:
    NotationQuantizer &operator=(const NotationQuantizer &); // not provided
};

}

#endif
