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

#ifndef RG_RELATIVEEVENT_H
#define RG_RELATIVEEVENT_H

namespace Rosegarden
{
  class Event;
  class FigChord;
  
/// @class RelativeEvent
/// Class to describe a relative event
/// @author Tom Breton (Tehom)
class RelativeEvent
{
public:
  RelativeEvent(Event *e, timeT startTime)
    : m_score(-1000000),
      m_bareEvent(e),
      m_relativeTime(e->getAbsoluteTime() - startTime)
  {};
  virtual ~RelativeEvent() {}

  virtual Event   *getAsEvent(timeT baseTime, const Key key,
			      const FigChord *notes)=0;
  void setScore(int score) { m_score = score; };
  int getScore() { return m_score; };
  timeT            getAbsoluteTime(timeT baseTime)
  { return m_relativeTime + baseTime; };
protected:
  int              m_score;
  Event            *m_bareEvent;
private:
  timeT            m_relativeTime;
};
}

#endif /* ifndef RG_RELATIVEEVENT_H */
