/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#ifndef _ROSEGARDENSEQUENCERIFACE_H_
#define _ROSEGARDENSEQUENCERIFACE_H_

#include <dcopobject.h>
#include "Event.h"

class RosegardenSequencerIface : virtual public DCOPObject
{
  K_DCOP
public:
k_dcop:

  virtual void quit() = 0;
  virtual int play(const Rosegarden::timeT &position,
                   const Rosegarden::timeT &latency) = 0;
  virtual void jumpTo(const Rosegarden::timeT &position) = 0;
  virtual int stop() = 0;
};

#endif // _ROSEGARDENSEQUENCERIFACE_H_
