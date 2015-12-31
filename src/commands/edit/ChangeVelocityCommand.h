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

#ifndef RG_CHANGEVELOCITYCOMMAND_H
#define RG_CHANGEVELOCITYCOMMAND_H

#include "document/BasicSelectionCommand.h"
#include <QString>
#include <QCoreApplication>


namespace Rosegarden
{

class EventSelection;


/** Add or subtract a constant from all event velocities.
    Use SelectionPropertyCommand if you want to do something more
    creative. */
class ChangeVelocityCommand : public BasicSelectionCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::ChangeVelocityCommand)

public:
    ChangeVelocityCommand(int delta, EventSelection &selection,bool rounddelta=true) :
        BasicSelectionCommand(getGlobalName(delta), selection, true),
        m_selection(&selection), m_delta(delta),m_rounddelta(rounddelta) { }

    static QString getGlobalName(int delta = 0) {
        if (delta > 0) return tr("&Increase Velocity");
        else return tr("&Reduce Velocity");
    }

protected:
    virtual void modifySegment();

private:
    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    int m_delta;
    bool m_rounddelta;
};


}

#endif
