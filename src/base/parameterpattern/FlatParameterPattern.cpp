/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "FlatParameterPattern.h"
#include "gui/dialogs/EventParameterDialog.h"

namespace Rosegarden
{

FlatParameterPattern FlatParameterPattern::single;

QString
FlatParameterPattern::getText(QString propertyName) const
{
    return
        EventParameterDialog::tr("Flat - set %1 to value").arg(propertyName);
}


ParameterPattern::SliderSpecVector
FlatParameterPattern::getSliderSpec(const Situation *situation) const
{
    SliderSpecVector result;
    int defaultVelocity = situation->m_currentFlatValue;
    result.push_back(SliderSpec(EventParameterDialog::tr("Value"),
                                defaultVelocity));
    return result;
}
    
// Set the properties of events from begin to end.
void
FlatParameterPattern::setEventProperties(iterator begin, iterator end,
                                         Result *result) const
{
    const PropertyName property = result->m_situation->m_property;
    const int          value    = result->m_parameters[0];
    for (iterator i = begin; i != end; ++i) {
        if ((*i)->isa(result->m_situation->m_eventType))
            { (*i)->set<Int>(property, value); }
    }
}

} // End namespace Rosegarden 
