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

#ifndef RG_INCREASEPARAMETERPATTERN_H
#define RG_INCREASEPARAMETERPATTERN_H

#include "ParameterPattern.h"

namespace Rosegarden
{

// @class IncreaseParameterPattern Implement the Flat parameter pattern
// @author Tom Breton (Tehom)
class IncreaseParameterPattern : public ParameterPattern
{
    QString getText(QString propertyName) const override;

    // Make as many sliders as we need.  EventParameterDialog will
    // truncate or pad as needed.
    SliderSpecVector
        getSliderSpec(const SelectionSituation *situation) const override;

    // Set the properties of events from begin to end.
    void
        setEventProperties(iterator begin, iterator end,
                           Result *result) const override;

    bool    m_isIncrease;

public:
    IncreaseParameterPattern(bool isIncrease) :
        m_isIncrease(isIncrease)
    {}

    static IncreaseParameterPattern increase;
    static IncreaseParameterPattern decrease;
};

}

#endif
