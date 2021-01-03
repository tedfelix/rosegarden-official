
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

#ifndef RG_SEGMENTCHANGETRANSPOSECOMMAND_H
#define RG_SEGMENTCHANGETRANSPOSECOMMAND_H

#include "document/Command.h"
#include <QString>
#include <QCoreApplication>
#include <vector>
#include "base/Event.h"

namespace Rosegarden
{

class Segment;


class SegmentChangeTransposeCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SegmentChangeTransposeCommand)
public:
    /// Set transpose on segments.
    SegmentChangeTransposeCommand(int, Segment *segment);
    ~SegmentChangeTransposeCommand() override;

    void execute() override;
    void unexecute() override;

    static QString getGlobalName(int unit) {
        if (!unit) {
            return tr("Undo change transposition");
        } else {
            return tr("Change transposition to %1").arg(unit);
        }
    }

private:
    Segment *m_segment;
    int m_oldUnit;
    int m_unit;
};



}

#endif
