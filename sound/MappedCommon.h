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

#ifndef _MAPPEDCOMMON_H_
#define _MAPPEDCOMMON_H_

// Some Mapped types that gui and sound libraries use to communicate
// plugin and Studio information.  Putting them here so we can change
// MappedStudio regularly without having to rebuild the gui.
//
#include <vector>

namespace Rosegarden
{

typedef int          MappedObjectId;
typedef QString      MappedObjectProperty;
typedef float        MappedObjectValue;

// typedef QValueVector<MappedObjectProperty> MappedObjectPropertyList;
// replaced with a std::vector<> for Qt2 compatibility

typedef std::vector<MappedObjectProperty> MappedObjectPropertyList;

}

QDataStream& operator>>(QDataStream& s, Rosegarden::MappedObjectPropertyList&);
QDataStream& operator<<(QDataStream&, const Rosegarden::MappedObjectPropertyList&);

#endif // _MAPPEDCOMMON_H_
