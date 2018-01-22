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

#include "LinearParameterPattern.h"
#include "gui/dialogs/EventParameterDialog.h"

namespace Rosegarden
{


LinearParameterPattern
LinearParameterPattern::
crescendo(EventParameterDialog::
          tr("Crescendo - set %1 rising from min to max"),
          false);

LinearParameterPattern
LinearParameterPattern::
diminuendo(EventParameterDialog::
           tr("Diminuendo - set %1 falling from max to min"),
           true);

QString
LinearParameterPattern::getText(QString propertyName) const
{
    return m_patternText.arg(propertyName);
}

ParameterPattern::SliderSpecVector
LinearParameterPattern::getSliderSpec(const SelectionSituation *situation) const
{
    SliderSpecVector result;
    std::pair<int,int> minMax =
        situation->getMinMax();

    SliderSpec lowArg = SliderSpec(EventParameterDialog::tr("Low Value"), 
                                    minMax.first, situation);
    SliderSpec highArg = SliderSpec(EventParameterDialog::tr("High Value"), 
                                    minMax.second, situation);

    if(m_isDiminuendo) {
        result.push_back(highArg);
        result.push_back(lowArg);
    } else {
        result.push_back(lowArg);
        result.push_back(highArg);
    }
    return result;
}
    
// Set the properties of events from begin to end.
void
LinearParameterPattern::
setEventProperties(iterator begin, iterator end,
                   Result *result) const
{
    const int          value1   = result->m_parameters[0];
    const int          value2   = result->m_parameters[1];

    const StartAndDuration times = getTimes (begin, end);
    const timeT startTime = times.first;
    const timeT duration  = times.second;

    const double valueChange = value2 - value1;

    for (iterator i = begin; i != end; ++i) {
        const timeT relativeTime = (*i)->getAbsoluteTime() - startTime;
	const double timeRatio = double(relativeTime)/double(duration);
        const int value = value1 + int(getValueDelta(valueChange, timeRatio));
        result->m_situation->setValue(*i, value);
    }
}

double
LinearParameterPattern::
getValueDelta(double valueChange, double timeRatio) const
{
    return valueChange * timeRatio;
}
    

} // End namespace Rosegarden 
