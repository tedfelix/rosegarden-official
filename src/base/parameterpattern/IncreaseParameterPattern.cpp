/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "IncreaseParameterPattern.h"
#include "gui/dialogs/EventParameterDialog.h"

namespace Rosegarden
{

IncreaseParameterPattern
IncreaseParameterPattern::
increase(
	 EventParameterDialog::tr("Increase - raise each %1 by value"),
	 EventParameterDialog::tr("Increase by"),
	 true);

IncreaseParameterPattern
IncreaseParameterPattern::
decrease(
	 EventParameterDialog::tr("Decrease - lower each %1 by value"),
	 EventParameterDialog::tr("Decrease by"),
	 false);

QString
IncreaseParameterPattern::getText(QString propertyName) const
{
    return m_patternText.arg(propertyName);
}

ParameterPattern::SliderSpecVector
IncreaseParameterPattern::getSliderSpec(const SelectionSituation * situation) const
{
    SliderSpecVector result;
    int defaultValue = 10;
    result.push_back(SliderSpec(m_valueText, defaultValue, situation));
    return result;
}
    
void
IncreaseParameterPattern::setEventProperties(iterator begin, iterator end,
                                             Result *result) const
{
    const int          delta    = result->m_parameters[0];
    const int          increase = m_isIncrease ? delta : -delta;
    for (iterator i = begin; i != end; ++i) {
        result->m_situation->addToValue(*i, increase);
    }
}

} // End namespace Rosegarden 
