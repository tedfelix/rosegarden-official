
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

#ifndef RG_MATRIXMODIFYCOMMAND_H
#define RG_MATRIXMODIFYCOMMAND_H

#include "document/BasicCommand.h"

#include <QCoreApplication>


namespace Rosegarden
{

class Segment;
class Event;


class MatrixModifyCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::MatrixModifyCommand)

public:
    MatrixModifyCommand(Segment &segment,
                        Event *oldEvent,
                        Event *newEvent,
                        bool isMove,
                        bool normalize);

protected:
    void modifySegment() override;

    bool m_normalize;

    Event *m_oldEvent;
    Event *m_newEvent;
};



}

#endif
