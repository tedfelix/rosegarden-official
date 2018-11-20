/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SELECTADDEVENNOTESSCOMMAND_H
#define RG_SELECTADDEVENNOTESSCOMMAND_H

#include "base/Event.h"
#include "document/BasicCommand.h"
#include "document/Command.h"

#include <QCoreApplication>
#include <QString>

namespace Rosegarden
{
class Segment;
 class EventSelection;

// Select evenly spaced note events, adding new ones where needed.
class SelectAddEvenNotesCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SelectAddEvenNotesCommand)

// How to geometrically interpolate beat times.
class BeatInterpolator
{
public:
    BeatInterpolator(timeT duration,
                     timeT prevBeatDuration,
                     int numSkippedBeats);
    BeatInterpolator(void) :
        m_baseBeatDuration(480), // No good default for this.
        m_logScalingPerBeat(1.0)
        {}
    timeT getBeatRelativeTime(int beatNumber);
    static timeT
        getLastBeatRelativeTime(timeT duration,
                                timeT prevBeatDuration,
                                int numSkippedBeats);
private:
    static float
    calculateLogScalingPerBeat(timeT duration,
                               timeT prevBeatDuration,
                               int numSkippedBeats);
    timeT m_baseBeatDuration;
    float m_logScalingPerBeat;
};

// An event that defines a beat, including the number of skipped beats
// leading up to it and how to interpolate them.
struct BeatEvent
{
BeatEvent(Event *event, int numSkippedBeats,
          BeatInterpolator beatInterpolator = BeatInterpolator()) :
        m_event(event),
        m_numSkippedBeats(numSkippedBeats),
        m_beatInterpolator(beatInterpolator)
        {}
    Event *m_event;
    int    m_numSkippedBeats;
    BeatInterpolator m_beatInterpolator;
};

    typedef std::vector<BeatEvent> BeatEventVector;
    typedef std::vector<Event*> EventVector;

 public:
    SelectAddEvenNotesCommand(BeatEventVector beatEventVector, Segment *segment);
    
    static BeatEventVector findBeatEvents(Segment &s,
                                          Event *firstBeat,
                                          Event *secondBeat);
    static BeatEventVector findBeatEvents(EventSelection *eventSelection);
    EventSelection *getSubsequentSelection() override;

 private:
    static QString getGlobalName() { return tr("Select Beats"); }
    static timeT getStartTime(BeatEventVector &beatEventVector);
    static timeT getEndTime(BeatEventVector &beatEventVector);
    void modifySegment() override;
        
    BeatEventVector m_beatEventVector;
    EventVector     m_eventsAdded;
};
}

#endif /* ifndef RG_SELECTADDEVENNOTESSCOMMAND_H */
