/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "HalfSinePattern.h"
#include "gui/dialogs/EventParameterDialog.h"
#include <cmath>

namespace Rosegarden
{

HalfSinePattern
HalfSinePattern::
crescendo(EventParameterDialog::
          tr("Half-wave crescendo - set %1 rising from min to max in a half sine wave contour"),
          false);

HalfSinePattern
HalfSinePattern::
diminuendo(EventParameterDialog::
           tr("Half-wave diminuendo - set %1 falling from max to min in a half sine wave contour"),
           true);

double
HalfSinePattern::
getValueDelta(double valueChange, double timeRatio) const
{
  /**
     For a half-sine, range is -pi/2 to pi/2, giving -1 to 1.

     value delta = ([-1..1]/2 + 0.5) * valueChange

     value delta = (sin(pi * ratio - pi/2)/2 + 0.5) * valueChange

     Using sin(x-pi/2) = -cos(x)

     value delta = (-cos(pi * ratio)/2 + 0.5) * valueChange

     To calculate pi, acos(0.0) = pi/2
  **/
  
    const double cosArg = 2 * acos(0.0) * timeRatio;
    return (-cos(cosArg)/2 + 0.5) * valueChange;
}

}
