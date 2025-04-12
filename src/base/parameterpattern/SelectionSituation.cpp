/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "SelectionSituation.h"

#include "base/BaseProperties.h"
#include "base/MidiTypes.h"
#include "gui/rulers/ControllerEventAdapter.h"
#include "misc/Strings.h"

#include <limits>
#include <utility>  // for std::pair

namespace Rosegarden
{


SelectionSituation::
SelectionSituation(const std::string& eventType,
                   EventSelection *selection,
                   int currentFlatValue) :
    m_eventType(eventType),
    m_property(),
    m_selection(selection),
    m_currentFlatValue((currentFlatValue < 0) ?
                       calcMeanValue() :
                       currentFlatValue)
{
    // For PitchBend there is no single property name, so we leave it as
    // controller value.
    if (m_eventType == Note::EventType)
        m_property = BaseProperties::VELOCITY;
    else
        m_property = Controller::VALUE;
}

// Return the largest value that the relevant property may have
int
SelectionSituation::
maxValue()  const
{
  if (m_eventType ==       Note::EventType) { return 127; }
  if (m_eventType == Controller::EventType) { return 127; }
  if (m_eventType ==  PitchBend::EventType) { return 16383; }
  // We shouldn't reach here.
  return 0;
}
// Get the property name as a QString
// @author Tom Breton (Tehom)
QString
SelectionSituation::getPropertyNameQString() const
{
    return strtoqstr(m_property);
}

void
SelectionSituation::setValue(Event *e, int value) const
{
    if (!isSuitable(e)) { return; }
    ControllerEventAdapter(e).setValue(value);
}

void
SelectionSituation::addToValue(Event *e, int increase) const
{
    if (!isSuitable(e)) { return; }
    long oldValue;
    ControllerEventAdapter(e).getValue(oldValue);
    ControllerEventAdapter(e).setValue(oldValue + increase);
}

// Return the maximum and minimum values of an integer-valued property
// for events in the selection.
std::pair<int,int>
SelectionSituation::
getMinMax() const
{
    // Start with values such that anything we find will supersede
    // them.
    int min = std::numeric_limits<int>::max(),
        max = std::numeric_limits<int>::min();
    const EventContainer &events =
            m_selection->getSegmentEvents();
    for (EventContainer::const_iterator i = events.begin();
         i != events.end();
         ++i) {
        if (isSuitable(*i)) {
            long value;
            ControllerEventAdapter(*i).getValue(value);

            if (max < value) { max = value; }
            if (value < min) { min = value; }
        }
    }
    return std::pair<int,int>(min,max);
}

// Return the mean value of the selection
int
SelectionSituation::
calcMeanValue() const
{
    float total = 0;
    int count = 0;
    const EventContainer &events =
            m_selection->getSegmentEvents();
    for (EventContainer::const_iterator i = events.begin();
         i != events.end();
         ++i) {
        if (isSuitable(*i)) {
            long value;
            ControllerEventAdapter(*i).getValue(value);
            total += (int)value;
            ++count;
        }
    }

    if (count > 0) { return (total / count) + 0.5; }
    else { return 0; }
}

}
