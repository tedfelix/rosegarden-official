// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "NotationTypes.h"
#include "quantizevalues.h"

QuantizeValues::QuantizeValues()
{
    Rosegarden::timeT note64 = Rosegarden::Note(Rosegarden::Note::SixtyFourthNote).getDuration();

    // smaller duration
    Rosegarden::timeT note96 = (Rosegarden::timeT)((double)note64 * 1.5);


    push_back(std::pair<Rosegarden::timeT, std::string>(Rosegarden::Note(Rosegarden::Note::WholeNote).getDuration(), std::string("1/1")));

    push_back(std::pair<Rosegarden::timeT, std::string>(Rosegarden::Note(Rosegarden::Note::HalfNote).getDuration(), std::string("1/2")));

    push_back(std::pair<Rosegarden::timeT, std::string>(Rosegarden::Note(Rosegarden::Note::QuarterNote).getDuration(), std::string("1/4")));

    push_back(std::pair<Rosegarden::timeT, std::string>(Rosegarden::Note(Rosegarden::Note::EighthNote).getDuration(), std::string("1/8")));

    push_back(std::pair<Rosegarden::timeT, std::string>(Rosegarden::Note(Rosegarden::Note::SixteenthNote).getDuration(), std::string("1/16")));

    push_back(std::pair<Rosegarden::timeT, std::string>(note96 * 4, std::string("1/24")));

    push_back(std::pair<Rosegarden::timeT, std::string>(Rosegarden::Note(Rosegarden::Note::ThirtySecondNote).getDuration(), std::string("1/32")));

    push_back(std::pair<Rosegarden::timeT, std::string>(note64, std::string("1/64")));

    push_back(std::pair<Rosegarden::timeT, std::string>(note96, std::string("1/96")));

    // no quantization
    //
    push_back(std::pair<Rosegarden::timeT, std::string>
        (0, string("Off")));

}


