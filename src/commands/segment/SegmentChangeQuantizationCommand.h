/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SEGMENTCHANGEQUANTIZATIONCOMMAND_H
#define RG_SEGMENTCHANGEQUANTIZATIONCOMMAND_H

#include "document/Command.h"
#include <QString>
#include <QCoreApplication>
#include <vector>
#include "base/Event.h"
#include "gui/editors/notation/NotationStrings.h"

namespace Rosegarden
{

class Segment;


class SegmentChangeQuantizationCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS (Rosegarden::SegmentChangeQuantizationCommand)
public:
    /// Set quantization on segments.  If unit is zero, switch quantization off
    SegmentChangeQuantizationCommand(timeT);
    ~SegmentChangeQuantizationCommand() override;

    void addSegment(Segment *s);

    void execute() override;
    void unexecute() override;

    static QString getGlobalName(timeT unit) {
        if (!unit) {
            return tr("Unquantize");
        } else {
            timeT error = 0;
            QString label = NotationStrings::makeNoteMenuLabel(unit, true, error);
            return tr("Quantize to %1").arg(label);
        }
    }


private:
    struct SegmentRec {
        Segment *segment;
        timeT oldUnit;
        bool wasQuantized;
    };
    typedef std::vector<SegmentRec> SegmentRecSet;
    SegmentRecSet m_records;

    timeT m_unit;
};



}

#endif
