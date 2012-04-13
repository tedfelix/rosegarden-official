/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ChannelInterval.h"

#include <limits>

namespace Rosegarden
{

// Time constants used with ChannelInterval
// @author Tom Breton (Tehom)
// NB: We cannot use the RealTime constants here lest we fall victim to
//     C++'s unpredictable static init order.
// See RealTime::beforeZeroTime
const RealTime ChannelInterval::m_beforeEarliestTime(-1,0);
// See RealTime::zeroTime
const RealTime ChannelInterval::m_earliestTime(0,0);
// See RealTime::beforeMaxTime
const RealTime ChannelInterval::m_latestTime(std::numeric_limits<int>::max(),0);
// See RealTime::maxTime
const RealTime ChannelInterval::m_afterLatestTime(std::numeric_limits<int>::max(),999999999);

}


