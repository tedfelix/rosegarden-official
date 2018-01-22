
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

#ifndef RG_INVERTCOMMAND_H
#define RG_INVERTCOMMAND_H

#include "document/BasicSelectionCommand.h"
#include <QString>
#include <QCoreApplication>




namespace Rosegarden
{

class EventSelection;


class InvertCommand : public BasicSelectionCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::InvertCommand)

public:
    InvertCommand(int semitones, EventSelection &selection) :
        BasicSelectionCommand(getGlobalName(semitones), selection, true),
        m_selection(&selection), m_semitones(semitones) { }

    static QString getGlobalName(int semitones = 0) {
        switch (semitones) {
        default:  return tr("&Invert");
        }
    }

protected:
    virtual void modifySegment();

private:
    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    int m_semitones;
};



}

#endif
