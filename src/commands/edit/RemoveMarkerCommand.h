
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

#ifndef RG_REMOVEMARKERCOMMAND_H
#define RG_REMOVEMARKERCOMMAND_H

#include <string>
#include "document/Command.h"
#include <QString>
#include "base/Event.h"
#include <QCoreApplication>




namespace Rosegarden
{

class Marker;
class Composition;


class RemoveMarkerCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::RemoveMarkerCommand)

public:
    RemoveMarkerCommand(Composition *comp,
                        int id,
                        timeT time,
                        const std::string &name,
                        const std::string &description);
    ~RemoveMarkerCommand() override;

    static QString getGlobalName() { return tr("&Remove Marker"); }

    void execute() override;
    void unexecute() override;

protected:

    Composition     *m_composition;
    Marker          *m_marker;
    int              m_id;
    timeT            m_time;
    std::string                  m_name;
    std::string                  m_descr;
    bool                         m_detached;

};


}

#endif
