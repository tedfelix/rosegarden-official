
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

#ifndef _RG_MODIFYMARKERCOMMAND_H_
#define _RG_MODIFYMARKERCOMMAND_H_

#include <string>
#include <kcommand.h>
#include <qstring.h>
#include "base/Event.h"
#include <klocale.h>




namespace Rosegarden
{

class Composition;


class ModifyMarkerCommand : public KNamedCommand
{
public:
    ModifyMarkerCommand(Composition *comp,
                        int id,
                        timeT time,
                        timeT newTime,
                        const std::string &name,
                        const std::string &des);
    ~ModifyMarkerCommand();

    static QString getGlobalName() { return i18n("&Modify Marker"); }

    virtual void execute();
    virtual void unexecute();

protected:

    Composition     *m_composition;
    timeT            m_time;
    timeT            m_newTime;

    int                          m_id;
    std::string                  m_name;
    std::string                  m_description;
    std::string                  m_oldName;
    std::string                  m_oldDescription;

};



}

#endif
