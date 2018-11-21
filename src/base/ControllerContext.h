/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_CONTROLLERCONTEXT_H
#define RG_CONTROLLERCONTEXT_H

#include <base/Event.h>
#include <map>

namespace Rosegarden
{
  class ControllerContext;
  class ControllerContextMap;
  class ControllerSearch;
  class ControlParameter;
  class Instrument;
  class Segment;

// @class ControllerSearchValue A (possibly intermediate) value in a
// parameter search, including what time it was found at.
// @author Tom Breton (Tehom)
class ControllerSearchValue
{
    friend class ControllerSearch;
 public:
    typedef std::pair<bool,ControllerSearchValue> Maybe;
 ControllerSearchValue(long value, timeT when) :
    m_value(value),
        m_when(when)
        {};
 ControllerSearchValue(void) :
    m_value(0),
        m_when(0)
            {};    
    int value(void) { return m_value; }
    int time(void)  { return m_when; }
 private:
    // Type is long so that ControllerEventAdapter can work.
    long              m_value; 
    timeT             m_when;
};

// @class ControllerSearch The unvarying parameters governing a
// search for a controller for a given instrument.
// @author Tom Breton (Tehom)
class ControllerSearch
{
 public:
    typedef ControllerSearchValue::Maybe Maybe;

    ControllerSearch(const std::string eventType,
                     int controllerId);
    
    // Search Segments A and B for the latest controller value.  B may
    // be nullptr but A must exist. 
    Maybe
        doubleSearch(Segment *a, Segment *b, timeT noLaterThan) const;

 private:
    Maybe
        searchSegment(const Segment *s, timeT noEarlierThan,
                      timeT noLaterThan) const;
    bool matches(Event *e) const;

    const std::string  m_eventType;
    const int          m_controllerId;
    const Instrument  *m_instrument;
};

// @class ControllerContextMap A cache of controller values, one per
// controller and one for pitchbend.
// @author Tom Breton (Tehom)
struct ControllerContextMap 
{
    typedef ControllerSearchValue::Maybe Maybe;
    typedef std::map< int, ControllerSearchValue>  Cache;
    typedef std::pair<int, ControllerSearchValue>  CacheEntry;
 public:
 ControllerContextMap(void) :
    m_PitchBendLatestValue(Maybe(false,ControllerSearchValue()))
    {};

    void makeControlValueAbsolute(Instrument *instrument, Segment *a,
                                  Segment *b, Event *e, timeT at);

    // Return the respective controller value at searchTime.  Segment
    // A is primary and governs timing and repitition.  Segment
    // B, if non-nullptr, will be searched too.  In any case the latest event
    // takes priority.  Defaults from Instrument will be used if
    // neccessary. 
    int getControllerValue(Instrument *instrument,
                           Segment *a, Segment *b, 
                           timeT searchTime, const std::string eventType,
                           int controllerId);

    void storeLatestValue(Event *e);
    void clear(void);

 private:
    int makeAbsolute(const ControlParameter * controlParameter,
                     int value) const;
    const ControlParameter
    *getControlParameter(Instrument *instrument,
                         const std::string eventType,
                         const int controllerId);
    static int
    getStaticValue(Instrument *instrument,
                   const std::string eventType, int controllerId);

    Cache             m_latestValues;
    Maybe             m_PitchBendLatestValue;
 };

class ControllerContextParams
{
public:
  ControllerContextParams(timeT                     triggerTime,
			  Instrument               *instrument,
			  Segment                  *a,
			  Segment                  *b,
			  ControllerContextMap     &controllerContext,
			  ControllerContextParams  *parentContext) :
    m_triggerTime(triggerTime),
    m_instrument(instrument),
    m_a(a),
    m_b(b),
    m_controllerContext(controllerContext),
    m_parentContext(parentContext)
  {}

  void makeControlValueAbsolute(Event *e) {
    m_controllerContext.
      makeControlValueAbsolute(m_instrument, m_a, m_b, e, m_triggerTime);
  };

private:
  timeT                     m_triggerTime;
  Instrument               *m_instrument;
  Segment                  *m_a;
  // May be nullptr
  Segment                  *m_b;
  ControllerContextMap     &m_controllerContext;
  // May be nullptr
  ControllerContextParams  *m_parentContext;
};
}

#endif /* ifndef RG_CONTROLLERCONTEXT_H */
