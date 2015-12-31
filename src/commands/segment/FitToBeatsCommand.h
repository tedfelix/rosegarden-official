/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_FITTOBEATSCOMMAND_H
#define RG_FITTOBEATSCOMMAND_H

#include "document/Command.h"
#include "base/Event.h"
#include "base/Composition.h"
#include <QCoreApplication>
#include <QString>
#include <map>


namespace Rosegarden
{

// @class FitToBeatsCommand
// @remarks Implements the command "Fit Existing Beats to Beat Segment".  
// @author Tom Breton (Tehom)
class FitToBeatsCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::FitToBeatsCommand)

public:
    FitToBeatsCommand(Segment *grooveSegment);

    virtual ~FitToBeatsCommand();

    static QString getGlobalName() 
        { return tr("Fit Existing Beats to Beat Segment"); }

    virtual void execute();
    virtual void unexecute();

private:
    typedef std::map<timeT, tempoT> TempoMap;
    typedef std::pair<timeT, tempoT> TempoChange;
    typedef std::vector<RealTime> vecRealTime;
    
    void initialise(Segment *s);
    void changeAllTempi(TempoMap newTempi);
    void changeSegments(SegmentMultiSet oldSegments,
                        SegmentMultiSet newSegments);

    static int
        getBeatRealTimes(Segment *s, vecRealTime &beatRealTimes);
    static TempoChange
        getTempoChange(Composition &composition, int i);
    static void
        getCurrentTempi(Composition &composition, TempoMap &Tempos);

    Composition *m_composition;

    SegmentMultiSet m_oldSegments;
    SegmentMultiSet m_newSegments;
    // !!! These don't need to be maps but they do need to associate
    // with a timeT.  Could just use a TempoChange.
    TempoMap m_oldTempi;
    TempoMap m_newTempi;
    bool                              m_executed;
};

}

#endif
