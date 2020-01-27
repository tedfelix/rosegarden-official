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

#include "QuarterSinePattern.h"

#include "gui/dialogs/EventParameterDialog.h"
#include "misc/Constants.h"

#include <cmath>

namespace Rosegarden
{


QuarterSinePattern
QuarterSinePattern::
crescendo(EventParameterDialog::
          tr("Quarter-wave crescendo - set %1 rising from min to max in a quarter sine wave contour"),
          false);

QuarterSinePattern
QuarterSinePattern::
diminuendo(EventParameterDialog::
           tr("Quarter-wave diminuendo - set %1 falling from max to min in a quarter sine wave contour"),
           true);

double
QuarterSinePattern::
getValueDelta(double valueChange, double timeRatio) const
{
    /**
       For a quarter-sine, range is 0 to pi/2, giving 0 to 1

       value delta = sin(pi * ratio/2) * valueChange
    **/
    const double sinArg = pi/2 * timeRatio;
    return sin(sinArg) * valueChange;
}

}
