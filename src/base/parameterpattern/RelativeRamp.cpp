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

#include "RelativeRamp.h"
#include "gui/dialogs/EventParameterDialog.h"

namespace Rosegarden
{

RelativeRamp RelativeRamp::single;

QString
RelativeRamp::getText(QString propertyName) const
{
    return
        QObject::tr("Relative Ramp - modify existing %1 values linearly").arg(propertyName);
}

ParameterPattern::SliderSpecVector
RelativeRamp::getSliderSpec(const SelectionSituation *situation) const
{
    SliderSpecVector result;

    SliderSpec firstArg =
      SliderSpec(QObject::tr("Increase first value this much: "),
		 0, situation, -situation->maxValue());
    SliderSpec lastArg =
      SliderSpec(QObject::tr("Increase last value this much: "), 
		 0, situation, -situation->maxValue());

    result.push_back(firstArg);
    result.push_back(lastArg);

    return result;
}
    
// Set the properties of events from begin to end.
void
RelativeRamp::
setEventProperties(iterator begin, iterator end,
                   Result *result) const
{
    const int          startDelta = result->m_parameters[0];
    const int          endDelta   = result->m_parameters[1];

    const StartAndDuration times = getTimes (begin, end);
    const timeT startTime = times.first;
    const timeT duration  = times.second;

    const double secondOrderDelta = endDelta - startDelta;

    for (iterator i = begin; i != end; ++i) {
        const timeT relativeTime = (*i)->getAbsoluteTime() - startTime;
	const double timeRatio = double(relativeTime)/double(duration);
	const double valueDelta =
	  startDelta + secondOrderDelta * timeRatio;
        result->m_situation->addToValue(*i, valueDelta);
    }
}
    
} // End namespace Rosegarden 
