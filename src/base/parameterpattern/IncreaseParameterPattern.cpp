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

#include "IncreaseParameterPattern.h"
#include "gui/dialogs/EventParameterDialog.h"

namespace Rosegarden
{

IncreaseParameterPattern IncreaseParameterPattern::increase(true);

IncreaseParameterPattern IncreaseParameterPattern::decrease(false);

QString
IncreaseParameterPattern::getText(QString propertyName) const
{
    QString text;
    if (m_isIncrease) {
        text = QObject::tr("Increase - raise each %1 by value");
    } else {
        text = QObject::tr("Decrease - lower each %1 by value");
    }
    return text.arg(propertyName);
}

ParameterPattern::SliderSpecVector
IncreaseParameterPattern::getSliderSpec(const SelectionSituation * situation) const
{
    QString valueText;
    if (m_isIncrease) {
        valueText = QObject::tr("Increase by");
    } else {
        valueText = QObject::tr("Decrease by");
    }

    SliderSpecVector result;
    result.push_back(SliderSpec(
            valueText,  // label
            10,  // defaultValue
            situation));

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
