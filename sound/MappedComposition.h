// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
  Rosegarden-4 v0.1
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
    MappedComposition():m_startTime(0, 0), m_endTime(0, 0) {;}

    MappedComposition(const Rosegarden::RealTime &sT,
                      const Rosegarden::RealTime &eT):
        m_startTime(sT), m_endTime(eT) {;}
    ~MappedComposition();

    const Rosegarden::RealTime getStartTime() const { return m_startTime; }
    const Rosegarden::RealTime getEndTime() const { return m_endTime; }
    void setStartTime(const Rosegarden::RealTime &sT) { m_startTime = sT; }
    void setEndTime(const Rosegarden::RealTime &eT) { m_endTime = eT; }

    // When we're looping we want to be able to move the start
    // time of MappedEvents around in this container
    //
    void moveStartTime(const Rosegarden::RealTime &mT);

    MappedComposition& operator+(const MappedComposition &mC);
    MappedComposition& operator=(const MappedComposition &mC);

    // This section is used for serialising this class over DCOP
    //
    //
    friend QDataStream& operator>>(QDataStream &dS, MappedComposition &mC);
    friend QDataStream& operator<<(QDataStream &dS,
                                   const MappedComposition &mC);

private:
    Rosegarden::RealTime m_startTime;
    Rosegarden::RealTime m_endTime;

};

typedef std::multiset<MappedEvent *, MappedEvent::MappedEventCmp>::iterator MappedCompositionIterator;


}

#endif // _MAPPEDCOMPOSITION_H_
