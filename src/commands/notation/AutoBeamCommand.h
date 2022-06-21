
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_AUTOBEAMCOMMAND_H
#define RG_AUTOBEAMCOMMAND_H

#include "document/BasicCommand.h"

#include <QCoreApplication>
#include <QString>


namespace Rosegarden
{


//class Segment;
class EventSelection;
class CommandRegistry;


class AutoBeamCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::AutoBeamCommand)

public:
    explicit AutoBeamCommand(EventSelection &selection) :
        BasicCommand(getGlobalName(), selection)
    { }

#if 0
    // Unused.
    AutoBeamCommand(Segment &segment) :
        BasicCommand(
                getGlobalName(),
                segment,
                segment.getStartTime(),
                segment.getEndMarkerTime())
    { }
#endif

    static void registerCommand(CommandRegistry *r);

protected:
    void modifySegment() override;

private:
    static QString getGlobalName() { return tr("&Auto-Beam"); }
};


}

#endif
