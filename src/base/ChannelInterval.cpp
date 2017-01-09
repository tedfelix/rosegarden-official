/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ChannelInterval.h"

#include "misc/Debug.h"
#include <limits>

namespace Rosegarden
{

// Time constants used with ChannelInterval
// @author Tom Breton (Tehom)
// @author Ted Felix
// NB: We cannot use the RealTime constants here lest we fall victim to
//     C++'s unpredictable static init order.
// We can't assume all events are > ZeroTime, there are rare
// exceptions.  These used to correspond to constants in RealTime.cpp.
const RealTime ChannelInterval::m_beforeEarliestTime(std::numeric_limits<int>::min(),0);
const RealTime ChannelInterval::m_earliestTime(std::numeric_limits<int>::min()+1,0);
const RealTime ChannelInterval::m_latestTime(std::numeric_limits<int>::max(),0);
const RealTime ChannelInterval::m_afterLatestTime(std::numeric_limits<int>::max(),999999999);

#if !defined NDEBUG
void
ChannelInterval::
assertSane() const
{
    Q_ASSERT(m_end > m_start);
    Q_ASSERT(m_marginBefore >= RealTime::zeroTime);
    Q_ASSERT(m_marginAfter >= RealTime::zeroTime);
}
#endif

QDebug &operator<<(QDebug &dbg, const ChannelInterval &channelInterval) {
    dbg
        << "interval" << channelInterval.m_start.toString()
        << "to" << channelInterval.m_end.toString()
        << "on channel" << channelInterval.getChannelId();
    dbg.nospace() << ".";
    dbg.space();
    return dbg;
}


}

