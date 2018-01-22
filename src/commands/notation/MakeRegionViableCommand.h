
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

#ifndef RG_MAKEREGIONVIABLECOMMAND_H
#define RG_MAKEREGIONVIABLECOMMAND_H

#include "document/BasicCommand.h"
#include <QString>
#include "base/Event.h"
#include <QCoreApplication>




namespace Rosegarden
{

class Segment;


/// MakeRegionViable works on part of a segment
class MakeRegionViableCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::MakeRegionViableCommand)

public:
    MakeRegionViableCommand(Segment &segment,
                            timeT startTime,
                            timeT endTime) :
        BasicCommand(getGlobalName(), segment, startTime, endTime) { }

    static QString getGlobalName() { return tr("Tie Notes at &Barlines"); }

protected:
    virtual void modifySegment();
};
    


}

#endif
