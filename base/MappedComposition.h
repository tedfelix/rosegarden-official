
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

#ifndef _MAPPEDCOMPOSITION_H_
#define _MAPPEDCOMPOSITION_H_

// Used as a transformation stage between Composition Events and MIDI
// events this eliminates the notion of Track.  The Composition is
// converted into a MappedComposition containing ordered Events with
// MIDI friendly information.
//

#include "MappedEvent.h"
#include <multiset.h>
#include <qdatastream.h>

namespace Rosegarden
{

class MappedComposition : public std::multiset<MappedEvent *,
                                      MappedEvent::MappedEventCmp>
{
public:
  MappedComposition():_startTime(0), _endTime(0) {;}
  MappedComposition(const unsigned int &sT, const unsigned int &eT):
             _startTime(sT), _endTime(eT) {;}
  ~MappedComposition() {;}

  const unsigned int beginTime() const { return _startTime; }
  const unsigned int endTime() const { return _endTime; }

  // This section is used for serialising this class over DCOP
  //
  //
  friend QDataStream& operator<<(QDataStream &dS, const MappedComposition &mC);
  friend QDataStream& operator>>(QDataStream &dS, MappedComposition &mC);

private:
  unsigned int _startTime;
  unsigned int _endTime;

  QDataStream _conversion;


};

typedef std::multiset<MappedEvent *, MappedEvent::MappedEventCmp>::iterator MappedCompositionIterator;


}

#endif // _MAPPEDCOMPOSITION_H_
