// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
  Rosegarden
  A sequencer and musical notation editor.
  Copyright 2000-2008 the Rosegarden development team.
 
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#include "MappedRealTime.h"

namespace Rosegarden
{

QDataStream&
operator>>(QDataStream &dS, MappedRealTime *mRT)
{
    dS >> mRT->sec;
    dS >> mRT->nsec;
    return dS;
}

QDataStream&
operator<<(QDataStream &dS, MappedRealTime *mRT)
{
    dS << mRT->sec;
    dS << mRT->nsec;
    return dS;
}

QDataStream&
operator>>(QDataStream &dS, MappedRealTime &mRT)
{
    dS >> mRT.sec;
    dS >> mRT.nsec;
    return dS;
}


QDataStream&
operator<<(QDataStream &dS, const MappedRealTime &mRT)
{
    dS << mRT.sec;
    dS << mRT.nsec;
    return dS;
}


}

