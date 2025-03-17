/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_PROGRAMCHANGEWIDGET_H
#define RG_PROGRAMCHANGEWIDGET_H

#include "EventWidget.h"

class QSpinBox;


namespace Rosegarden
{


class EditEvent;
class Event;


class ProgramChangeWidget : public EventWidget
{
public:

    ProgramChangeWidget(EditEvent *parent, const Event &event);

    PropertyNameSet getPropertyFilter() const override;

    /// Copy widget values to the Event.
    void updateEvent(Event &event) const override;

private:

    // Widgets

    QSpinBox *m_programSpinBox;

};


}

#endif
