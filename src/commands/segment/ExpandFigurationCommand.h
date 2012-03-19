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

#ifndef _EXPANDFIGURATIONCOMMAND_H_
#define _EXPANDFIGURATIONCOMMAND_H_

#include "document/Command.h"
#include "base/Event.h"
#include "base/Composition.h"
#include <QCoreApplication>
#include <QString>
#include <map>


namespace Rosegarden
{


    class SegmentSelection;
    class NotationQuantizer;
    class ChordFromCounterpoint;
    class RelativeEvent;
    typedef std::vector<RelativeEvent *> RelativeEventVec;


// @class Figuration
// @remarks A figuration ready for expansion
// @author Tom Breton (Tehom)
class Figuration
{
 public:
    static timeT getOKFigTime(Composition *composition, timeT lookAtTime = 0);
    static timeT getNextFigTime(Composition *composition, timeT lookAtTime);
    timeT        getEndTime(timeT startTime)
    { return startTime + m_duration; }
    RelativeEventVec m_events;
    timeT            m_duration;
    // Parameter count
    unsigned int     m_NbParameters;
    // Punt: Keysignature it applies to.
};

// @class ExpandFigurationCommand
// @remarks Implements the command "Expand block chords to figurations".  
// @author Tom Breton (Tehom)
class ExpandFigurationCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::ExpandFigurationCommand)

public:
    typedef std::pair<RelativeEvent *, bool> RelationSeen;
    typedef std::map <RelativeEvent *, bool> RelationSeenMap;
    typedef std::set <RelativeEvent *>       UnsolvedNote;
    typedef std::set<UnsolvedNote> UnsolvedFiguration;

    ExpandFigurationCommand(SegmentSelection selection);

    virtual ~ExpandFigurationCommand();

    static QString getGlobalName() 
        { return tr("Expand Block Chords to Figurations"); }

    virtual void execute();
    virtual void unexecute();

private:
    
    void initialise(SegmentSelection selection);
    static UnsolvedNote getPossibleRelations(Event *e,
                                             const ChordFromCounterpoint *parameterChord,
                                             const Key key,
                                             timeT startTime);
    static RelationSeenMap getFreshSeenMap(UnsolvedFiguration &figuration);
    static Figuration getFiguration(Segment *s);

    Composition                   *m_composition;
    // The new segments we make.
    segmentcontainer               m_newSegments;
    bool                           m_executed;

    static const timeT m_preDuration;
    static NotationQuantizer * m_nq;
};

}

#endif
