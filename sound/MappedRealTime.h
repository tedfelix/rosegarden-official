// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
  Rosegarden-4
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

#include <qdatastream.h>
#include "RealTime.h"

// Just a DCOP wrapper to RealTime
//

#ifndef _MAPPEDREALTIME_H_
#define _MAPPEDREALTIME_H_

namespace Rosegarden
{

class MappedRealTime : public RealTime
{
public:
    MappedRealTime():RealTime(0, 0) {;}
    MappedRealTime(const RealTime &t):RealTime(t.sec, t.usec) {;}

    // Return as RealTime
    RealTime getRealTime() { return RealTime(sec, usec); }

    // DCOP datastream
    //
    friend QDataStream& operator>>(QDataStream &dS, MappedRealTime *mRT);
    friend QDataStream& operator<<(QDataStream &dS, MappedRealTime *mRT);
    friend QDataStream& operator>>(QDataStream &dS, MappedRealTime &mRT);
    friend QDataStream& operator<<(QDataStream &dS, const MappedRealTime &mRT);


};

}

#endif // _MAPPEDREALTIME_H_

