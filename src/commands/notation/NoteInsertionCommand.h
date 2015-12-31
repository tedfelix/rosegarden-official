
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

#ifndef RG_NOTEINSERTIONCOMMAND_H
#define RG_NOTEINSERTIONCOMMAND_H

#include "base/NotationTypes.h"
#include "document/BasicCommand.h"
#include "base/Event.h"
#include "gui/editors/notation/NoteStyle.h"

#include <QCoreApplication>

namespace Rosegarden
{

class Segment;
class Event;


class NoteInsertionCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::NoteInsertionCommand)

public:
    enum AutoBeamMode {
        AutoBeamOff,
        AutoBeamOn
    };

    enum AutoTieBarlinesMode {
        AutoTieBarlinesOff,
        AutoTieBarlinesOn
    };

    enum MatrixMode {
        MatrixModeOff,
        MatrixModeOn
    };

    enum GraceMode {
        GraceModeOff,
        GraceModeOn,
        GraceAndTripletModesOn
    };

    NoteInsertionCommand(Segment &segment,
                         timeT time,
                         timeT endTime,
                         Note note,
                         int pitch,
                         Accidental accidental,
                         AutoBeamMode autoBeam,
                         MatrixMode matrixType,
                         GraceMode grace,
                         float targetSubordering,
                         NoteStyleName noteStyle);
    NoteInsertionCommand(Segment &segment,
                             timeT time,
                             timeT endTime,
                             Note note,
                             int pitch,
                             Accidental accidental,
                             AutoBeamMode autoBeam,
                             AutoTieBarlinesMode autoTieBarlines,
                             MatrixMode matrixType,
                             GraceMode grace,
                             float targetSubordering,
                             NoteStyleName noteStyle,
                             int velocity = 0); // Zero for rest inserter
    virtual ~NoteInsertionCommand();

    virtual EventSelection *getSubsequentSelection();
    Event *getLastInsertedEvent() { return m_lastInsertedEvent; }

protected:
    virtual void modifySegment();

    timeT getModificationStartTime(Segment &, timeT);

    timeT m_insertionTime;
    Note m_note;
    int m_pitch;
    Accidental m_accidental;
    bool m_autoBeam;
    bool m_autoTieBarlines;
    bool m_matrixType;
    GraceMode m_grace;
    float m_targetSubordering;
    NoteStyleName m_noteStyle;
    int m_velocity;

    Event *m_lastInsertedEvent;
};


}

#endif
