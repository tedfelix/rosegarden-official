
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SUSTAININSERTIONCOMMAND_H
#define RG_SUSTAININSERTIONCOMMAND_H

#include "document/BasicCommand.h"
#include <QString>
#include "base/Event.h"
#include <QCoreApplication>


class Pedal;


namespace Rosegarden
{

class Segment;
class Event;


class SustainInsertionCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SustainInsertionCommand)

public:
    SustainInsertionCommand(Segment &segment,
                            timeT time,
                            bool down,
                            int controllerNumber);
    virtual ~SustainInsertionCommand();

    static QString getGlobalName(bool down) {
        if (down) {
            return tr("Add Pedal &Press");
        } else {
            return tr("Add Pedal &Release");
        }
    }

    virtual EventSelection *getSubsequentSelection();
    Event *getLastInsertedEvent() { return m_lastInsertedEvent; }

protected:
    virtual void modifySegment();

    bool m_down;
    int m_controllerNumber;
    Event *m_lastInsertedEvent;
};



}

#endif
