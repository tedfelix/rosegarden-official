// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _MATRIX_COMMANDS_H_
#define _MATRIX_COMMANDS_H_

#include "basiccommand.h"
#include "Segment.h"

class MatrixInsertionCommand : public BasicCommand
{
public:
    MatrixInsertionCommand(Rosegarden::Segment &segment,
                           Rosegarden::timeT time,
                           Rosegarden::timeT endTime,
                           Rosegarden::Event *event);

    virtual ~MatrixInsertionCommand();
    
protected:
    virtual void modifySegment();

    Rosegarden::Event* m_event;
};

//------------------------------

class MatrixEraseCommand : public BasicCommand
{
public:
    MatrixEraseCommand(Rosegarden::Segment &segment,
                       Rosegarden::Event *event);

    virtual Rosegarden::timeT getRelayoutEndTime();

protected:
    virtual void modifySegment();

    Rosegarden::Event *m_event; // only used on 1st execute (cf bruteForceRedo)
    Rosegarden::timeT m_relayoutEndTime;
};

//------------------------------

class MatrixModifyCommand : public BasicCommand
{
public:
    MatrixModifyCommand(Rosegarden::Segment &segment,
                        Rosegarden::Event *oldEvent,
                        Rosegarden::Event *newEvent,
                        bool isMove);

protected:
    virtual void modifySegment();

    int m_newPitch;

    Rosegarden::Event *m_oldEvent;
    Rosegarden::Event *m_newEvent;
};


#endif

