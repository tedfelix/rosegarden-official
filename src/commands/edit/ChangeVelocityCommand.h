
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_CHANGEVELOCITYCOMMAND_H_
#define _RG_CHANGEVELOCITYCOMMAND_H_

#include "document/BasicSelectionCommand.h"
#include <QString>
#include <klocale.h>




namespace Rosegarden
{

class EventSelection;


/** Add or subtract a constant from all event velocities.
    Use SelectionPropertyCommand if you want to do something more
    creative. */
class ChangeVelocityCommand : public BasicSelectionCommand
{
public:
    ChangeVelocityCommand(int delta, EventSelection &selection) :
        BasicSelectionCommand(getGlobalName(delta), selection, true),
        m_selection(&selection), m_delta(delta) { }

    static QString getGlobalName(int delta = 0) {
        if (delta > 0) return i18n("&Increase Velocity");
        else return i18n("&Reduce Velocity");
    }

protected:
    virtual void modifySegment();

private:
    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    int m_delta;
};


}

#endif
