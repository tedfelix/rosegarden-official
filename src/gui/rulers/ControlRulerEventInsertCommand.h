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

#ifndef RG_CONTROLRULEREVENTINSERTCOMMAND_H
#define RG_CONTROLRULEREVENTINSERTCOMMAND_H

#include "document/BasicCommand.h"
#include <string>

#include <QCoreApplication>

namespace Rosegarden
{

class ControlRulerEventInsertCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::ControlRulerEventInsertCommand)

public:
    ControlRulerEventInsertCommand(const std::string &type, 
                                   timeT insertTime, 
                                   long number, 
                                   long initialValue,
                                   Segment &segment,
				    timeT duration=0);

    virtual ~ControlRulerEventInsertCommand() {;}

protected:

    virtual void modifySegment();

    std::string m_type;
    long m_number;
    long m_initialValue;
};

}

#endif /*CONTROLRULEREVENTINSERTCOMMAND_H_*/
