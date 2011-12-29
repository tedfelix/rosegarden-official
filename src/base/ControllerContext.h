/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _CONTROLLERCONTEXT_H_
#define _CONTROLLERCONTEXT_H_

#include <base/Event.h>
#include <map>

namespace Rosegarden
{
  class Composition;
  class ControllerContextMap;
  class ControllerSearch;
  class ControlParameter;
  class Instrument;
  class RosegardenDocument;
  class Segment;

// @class ControllerSearchValue A (possibly intermediate) value in a
// parameter search, including what time it was found at.
// @author Tom Breton (Tehom)
class ControllerSearchValue
{
    friend class ControllerSearch;
 public:
 ControllerSearchValue(long value, timeT when) :
    m_value(value),
        m_when(when)
        {};
 ControllerSearchValue(void) :
    m_value(0),
        m_when(0)
            {};    
    int value(void) { return m_value; }
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
    typedef std::pair<bool,ControllerSearchValue> Maybe;
 ControllerSearch(const std::string eventType,
                  int controllerId,
                  Composition &comp,
                  const Instrument  *instrument,
                  const ControlParameter *controlParameter) :
    m_eventType(eventType),
        m_controllerId(controllerId),
        m_comp(comp),
        m_instrument(instrument),
        m_controlParameter(controlParameter)
        {};
    Maybe
        search(Segment *s, timeT noEarlierThan,
               timeT noLaterThan, bool forceAbsolute) const;
    bool matches(Event *e) const;
    int getStaticValue(void) const;
 private:
    const std::string  m_eventType;
    const int          m_controllerId;
    Composition       &m_comp;
    const Instrument  *m_instrument;
    const ControlParameter *m_controlParameter;
};


// @class ControllerContext  Context information for one controller of
// one Controllable instrument at one time.
// @author Tom Breton (Tehom)
class ControllerContext
{
    friend class ControllerContextMap;
 public:
 ControllerContext(int diffValue, int min, int max)
     : m_diffValue(diffValue),
        m_min(min),
        m_max(max)
        {};
    unsigned int getAbsoluteValue(unsigned int relativeValue) const;
    void adjustControllerValue(Event *e) const;
    static const ControllerContext
        getControllerContext(RosegardenDocument *doc, Segment *s, timeT at,
                             const std::string eventType, int controllerId);
 private:
    int m_diffValue;
    int m_min;
    int m_max;

    static const ControllerContext dummyContext127;
};

// @class ControllerContextMap A cache of ControllerContexts, one per
// controller and one for pitchbend.
// @author Tom Breton (Tehom)
struct ControllerContextMap : public std::map<int,ControllerContext>
{
 public:
 ControllerContextMap(void) :
    m_havePitchBendContext(false),
        m_pitchBendContext(0,0,0) // Really uninitted
            {};

    const ControllerContext * 
        findControllerContext(RosegardenDocument *doc, Segment *s, timeT at,
                              const std::string eventType, int controllerId);

 private:
    bool              m_havePitchBendContext;
    ControllerContext m_pitchBendContext;
 };

}

#endif /* ifndef _CONTROLLERCONTEXT_H_ */
