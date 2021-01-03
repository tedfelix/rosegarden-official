
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_EVENTEDITCOMMAND_H
#define RG_EVENTEDITCOMMAND_H

#include "base/Event.h"
#include "document/BasicCommand.h"
#include <QString>
#include <QCoreApplication>




namespace Rosegarden
{

class Segment;


/**
 * Replace an event with another one (likely to be used in
 * conjunction with EventEditDialog)
 */

class EventEditCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::EventEditCommand)

public:
    EventEditCommand(Segment &segment,
                     Event *eventToModify,
                     const Event &newEvent);

    static QString getGlobalName() { return tr("Edit E&vent"); }

protected:
    void modifySegment() override;

private:
    Event *m_oldEvent; // only used on 1st execute
    Event m_newEvent; // only used on 1st execute
};



}

#endif
