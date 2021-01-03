/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "AlternatingParameterPattern.h"
#include "gui/dialogs/EventParameterDialog.h"

namespace Rosegarden
{

AlternatingParameterPattern AlternatingParameterPattern::single;

QString
AlternatingParameterPattern::getText(QString propertyName) const
{
    return
        QObject::tr("Alternating - set %1 to max and min on alternate events").arg(propertyName);
}

ParameterPattern::SliderSpecVector
AlternatingParameterPattern::getSliderSpec(const SelectionSituation *situation) const
{
    SliderSpecVector result;
    std::pair<int,int> minMax =
        situation->getMinMax();

    // First is high value, second is low.
    result.push_back(SliderSpec(QObject::tr("First Value"),
                                minMax.second, situation));
    result.push_back(SliderSpec(QObject::tr("Second Value"),
                                minMax.first, situation));
    return result;
}
    
// Set the properties of events from begin to end.
void
AlternatingParameterPattern::
setEventProperties(iterator begin, iterator end,
                   Result *result) const
{
    const int          value1   = result->m_parameters[0];
    const int          value2   = result->m_parameters[1];
    int count = 0;
    for (iterator i = begin; i != end; ++i) {
        // Only change count on suitable events, in case non-notes
        // like key sigs are selected
        if (result->m_situation->isSuitable(*i)) {
            if (count % 2 == 0)
                { result->m_situation->setValue(*i, value1);}
            else
                { result->m_situation->setValue(*i, value2);}
            ++count;
        }
    }
}

} // End namespace Rosegarden 
