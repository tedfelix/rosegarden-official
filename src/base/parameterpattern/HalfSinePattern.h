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

#ifndef RG_HALFSINEPATTERN_H
#define RG_HALFSINEPATTERN_H

#include "ParameterPattern.h"
#include "LinearParameterPattern.h"

namespace Rosegarden
{

// @class HalfSinePattern
// Implement variant Crescendo and Diminuendo parameter patterns.
// @author Tom Breton (Tehom)
class HalfSinePattern : public LinearParameterPattern
{
    double getValueDelta(double valueChange, double timeRatio) const override;

public:
    HalfSinePattern(QString patternText,
		       bool isDiminuendo) :
    LinearParameterPattern(patternText,isDiminuendo)
    {};
    static HalfSinePattern crescendo;
    static HalfSinePattern diminuendo;
};

}

#endif /* ifndef RG_HALFSINEPATTERN_H */
