/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.

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
    struct RelativeEvent;
    typedef std::vector<Event *> eventVector;
    typedef std::vector<RelativeEvent *> RelativeEventVec;


class figurationT
{
 public:
    RelativeEventVec m_events;
    timeT            m_duration;
    // Punt: Keysignature it applies to.
    // Punt: Parameter count
};

// @class ExpandFigurationCommand
// @remarks Implements the command "Expand block chords to figurations".  
// @author Tom Breton (Tehom)
class ExpandFigurationCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::ExpandFigurationCommand)

public:
    typedef Composition::segmentcontainer segmentcontainer;
    
    typedef std::pair<RelativeEvent *, bool> RelationSeen;
    typedef std::map <RelativeEvent *, bool> RelationSeenMap;
    typedef std::set <RelativeEvent *>       UnsolvedNote;
    typedef std::set<UnsolvedNote> UnsolvedFiguration;

    ExpandFigurationCommand(SegmentSelection selection);

    virtual ~ExpandFigurationCommand();

    static QString getGlobalName() 
        { return tr("Expand block chords to figurations"); }

    virtual void execute();
    virtual void unexecute();

private:
    
    void initialise(SegmentSelection selection);
    static UnsolvedNote getPossibleRelations(Event *e,
                                              const eventVector parameterChord,
                                              const Key key,
                                              timeT startTime);
    static RelationSeenMap getFreshSeenMap(UnsolvedFiguration &figuration);
    static timeT       getFirstFigTime(Segment *s);
    static timeT       getNextFigTime(Segment *s, timeT time);
    static eventVector getBlockChord(Segment *s, timeT time);
    static figurationT getFiguration(Segment *s);

    Composition                   *m_composition;
    // The new segments we make.
    segmentcontainer               m_newSegments;
    bool                           m_executed;
};

}

#endif
