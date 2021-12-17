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

#ifndef RG_PLACECONTROLLERSCOMMAND_H
#define RG_PLACECONTROLLERSCOMMAND_H

#include "document/BasicCommand.h"

#include <QCoreApplication>  // For Q_DECLARE_TR_FUNCTIONS()


namespace Rosegarden
{


class ControlParameter;
class EventSelection;
class Instrument;


/// Place a default CC where each note begins.
class PlaceControllersCommand : public BasicCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::PlaceControllersCommand)

public:
    PlaceControllersCommand(EventSelection &selection,
                            const Instrument *instrument,
                            const ControlParameter *cp);

protected:
    void modifySegment() override;

private:
    EventSelection *m_selection;

    // Event type (pitchbend or controller)
    const std::string m_eventType;
    // Controller number (Ignored for pitchbend)
    const int m_controllerId;
    // Value to place.
    const int m_controllerValue;
};


}

#endif /* ifndef RG_PLACECONTROLLERSCOMMAND_H */
