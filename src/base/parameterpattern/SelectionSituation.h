/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2020 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SELECTIONSITUATION_H
#define RG_SELECTIONSITUATION_H

#include "base/PropertyName.h"
#include "base/Selection.h"

#include <QString>

#include <string>


namespace Rosegarden
{


/// A selection with its situation (from a ruler, etc).
/**
 * Non-gui-related situation data for EventParameterDialog and
 * SelectionPropertyCommand.
 *
 * @author Tom Breton (Tehom)
 */
class SelectionSituation
{
public:
    SelectionSituation(std::string eventType,
                       EventSelection *selection,
                       int currentFlatValue = -1);

    QString getPropertyNameQString() const;

    bool isSuitable(Event *e) const  { return e->isa(m_eventType); }

    /// The min and max value for the selected events.
    std::pair<int,int> getMinMax() const;

    void setValue(Event *e, int value) const;
    void addToValue(Event *e, int increase) const;

    EventSelection *getEventSelection() const  { return m_selection; }

    int getFlatValue() const  { return m_currentFlatValue; }

    /// The max allowed value for this kind of event.
    int maxValue() const;

private:
    const std::string m_eventType;
    PropertyName m_property;

    EventSelection *m_selection;
    int calcMeanValue() const;

    // A reference value from outside.  Some patterns use it as
    // default.
    const int m_currentFlatValue;
};


}

#endif
