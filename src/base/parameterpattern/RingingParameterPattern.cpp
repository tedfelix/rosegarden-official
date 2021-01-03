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

#include "RingingParameterPattern.h"
#include "gui/dialogs/EventParameterDialog.h"

namespace Rosegarden
{

RingingParameterPattern RingingParameterPattern::single;

QString
RingingParameterPattern::getText(QString propertyName) const
{
    return
        QObject::tr("Ringing - set %1 alternating from max to min with both dying to zero").arg(propertyName);
}

ParameterPattern::SliderSpecVector
RingingParameterPattern::getSliderSpec(const SelectionSituation *situation) const
{
    SliderSpecVector result;
    std::pair<int,int> minMax = situation->getMinMax();

    result.push_back(SliderSpec(QObject::tr("First Value"),
                                minMax.second, situation));
    result.push_back(SliderSpec(QObject::tr("Second Value"),
                                minMax.first, situation));
    return result;
}
    
// Set the properties of events from begin to end.
void
RingingParameterPattern::
setEventProperties(iterator begin, iterator end,
                   Result *result) const
{
    const int          value1   = result->m_parameters[0];
    const int          value2   = result->m_parameters[1];

    int count = 0;
    StartAndDuration times = getTimes (begin, end);
    timeT startTime = times.first;
    timeT duration  = times.second;

    double step = double(value1 - value2) / double(duration);
    double lowStep = double(value2) / double(duration);

    for (iterator i = begin; i != end; ++i) {
        // Only change count on suitable events, in case non-notes
        // like key sigs are selected
        if  (result->m_situation->isSuitable(*i)) {
            timeT  relativeTime = (*i)->getAbsoluteTime() - startTime;
            double realStep  = (count % 2 == 0) ? step   : lowStep;
            int    baseValue = (count % 2 == 0) ? value1 : value2;
            int value = baseValue - int(realStep * relativeTime);
            if (value < 0) { value = 0; }
            result->m_situation->setValue(*i, value);
            ++count;
        }
    }
}

} // End namespace Rosegarden 
