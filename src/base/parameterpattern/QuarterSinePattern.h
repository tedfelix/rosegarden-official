/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_QUARTERSINEPATTERN_H
#define RG_QUARTERSINEPATTERN_H

#include "ParameterPattern.h"
#include "LinearParameterPattern.h"

namespace Rosegarden
{


// @class QuarterSinePattern
// Implement variant Crescendo and Diminuendo parameter patterns.
// @author Tom Breton (Tehom)
class QuarterSinePattern : public LinearParameterPattern
{
protected:

    QString getText(QString propertyName) const override;

    double getValueDelta(double valueChange, double timeRatio) const override;

public:
    explicit QuarterSinePattern(bool isDiminuendo) :
        LinearParameterPattern(isDiminuendo)
    {}

};


}

#endif /* ifndef RG_QUARTERSINEPATTERN_H */
