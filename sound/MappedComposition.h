// -*- c-basic-offset: 4 -*-

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


// MappedComposition is used with MappedEvent to create a sequence
// of MIDI ready events ready for playing.  The QDataStream operators
// are a necessary part of the DCOP transmission process allowing 
// the whole class to be serialized.  The core application is sent
// a request specifying a time slice between given start and end
// points which it fills with MappedEvents which are cut down
// (sequencer suitable) versions of the core Events.
//

#include <Composition.h>
#include "MappedEvent.h"
#include <multiset.h>
#include <qdatastream.h>

namespace Rosegarden
{

class MappedComposition : public std::multiset<MappedEvent *,
                          MappedEvent::MappedEventCmp>
{
public:
    MappedComposition():m_startTime(0), m_endTime(0) {;}

    MappedComposition(Rosegarden::Composition &comp,
                      const Rosegarden::timeT &sT,
                      const Rosegarden::timeT &eT);

    MappedComposition(const Rosegarden::timeT &sT, const Rosegarden::timeT &eT):
        m_startTime(sT), m_endTime(eT) {;}
    ~MappedComposition() {;}

    const Rosegarden::timeT getStartTime() const { return m_startTime; }
    const Rosegarden::timeT getEndTime() const { return m_endTime; }
    void setStartTime(const Rosegarden::timeT &sT) { m_startTime = sT; }
    void setEndTime(const Rosegarden::timeT &eT) { m_endTime = eT; }


    // This section is used for serialising this class over DCOP
    //
    //
    friend QDataStream& operator<<(QDataStream &dS, const MappedComposition &mC);
    friend QDataStream& operator>>(QDataStream &dS, MappedComposition &mC);

private:
    Rosegarden::timeT m_startTime;
    Rosegarden::timeT m_endTime;

};

typedef std::multiset<MappedEvent *, MappedEvent::MappedEventCmp>::iterator MappedCompositionIterator;


}

#endif // _MAPPEDCOMPOSITION_H_
