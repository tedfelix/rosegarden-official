
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

#ifndef RG_CLEFINSERTIONCOMMAND_H
#define RG_CLEFINSERTIONCOMMAND_H

#include "base/NotationTypes.h"
#include "document/BasicCommand.h"
#include <QString>
#include "base/Event.h"

#include <QCoreApplication>


namespace Rosegarden
{

class Segment;
class LinkedSegment;
class Event;

class ClefInsertionCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::ClefInsertionCommand)

public:
    ClefInsertionCommand(Segment &segment,
                         timeT time,
                         Clef clef,
                         bool shouldChangeOctave = false,
                         bool shouldTranspose = false);
    ~ClefInsertionCommand() override;

    virtual QString getThisGlobalName(Clef *clef = nullptr);
    static QString getGlobalName(Clef *clef = nullptr);
    timeT getRelayoutEndTime() override;

    EventSelection *getSubsequentSelection() override;
    Event *getLastInsertedEvent() { return m_lastInsertedEvent; }

protected:
    void modifySegment() override;

    Clef m_clef;
    bool m_shouldChangeOctave;
    bool m_shouldTranspose;

    Event *m_lastInsertedEvent;
};

class ClefLinkInsertionCommand : public ClefInsertionCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::ClefLinkInsertionCommand)

public:
    ClefLinkInsertionCommand(Segment &segment,
                            timeT time,
                            Clef clef,
                            bool shouldChangeOctave = false,
                            bool shouldTranspose = false);
    ~ClefLinkInsertionCommand() override;

    QString getThisGlobalName(Clef *clef = nullptr) override;
    static QString getGlobalName(Clef *clef = nullptr);

protected:
    void modifySegment() override;
};

}

#endif
