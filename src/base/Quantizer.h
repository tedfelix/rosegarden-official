/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_QUANTIZER_H
#define RG_QUANTIZER_H

#include "base/Segment.h"
#include "FastVector.h"

#include <string>


namespace Rosegarden {


class Event;
class EventSelection;

/**
   The Quantizer class rounds the starting times and durations of note
   and rest events according to one of a set of possible criteria.
*/
class Quantizer
{
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
    virtual timeT getQuantizedDuration(const Event *e) const;

    /**
     * Return the quantized absolute time of the event if it has been
     * quantized -- otherwise just return the unquantized time.  Do
     * not modify the event.
     */
    virtual timeT getQuantizedAbsoluteTime(const Event *e) const;

    /**
     * Return the unquantized absolute time of the event --
     * the absolute time that would be restored by a call to
     * unquantize.
     */
    virtual timeT getUnquantizedAbsoluteTime(Event *e) const;

    /**
     * Return the unquantized absolute time of the event --
     * the absolute time that would be restored by a call to
     * unquantize.
     */
    virtual timeT getUnquantizedDuration(Event *e) const;

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
    mutable std::pair<timeT, timeT> m_normalizeRegion;

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


}

#endif
