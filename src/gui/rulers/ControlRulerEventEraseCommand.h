/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_CONTROLLEREVENTERASECOMMAND_H
#define RG_CONTROLLEREVENTERASECOMMAND_H

#include "base/Event.h"
#include "document/BasicCommand.h"
#include "ControlItem.h"

#include <QCoreApplication>

namespace Rosegarden
{

class ControlRulerEventEraseCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::ControlRulerEventEraseCommand)

public:

    ControlRulerEventEraseCommand(ControlItemList selectedItems,
                                Segment &segment,
                                Rosegarden::timeT start, Rosegarden::timeT end);
    virtual ~ControlRulerEventEraseCommand() {;}


protected:

    virtual void modifySegment();

    ControlItemList m_selectedItems;
};

}

#endif /*CONTROLLEREVENTERASECOMMAND_H_*/
