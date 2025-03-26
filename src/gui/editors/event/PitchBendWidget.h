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

#ifndef RG_PITCHBENDWIDGET_H
#define RG_PITCHBENDWIDGET_H

#include "EventWidget.h"

class QSpinBox;


namespace Rosegarden
{


class EditEvent;
class Event;


class PitchBendWidget : public EventWidget
{
public:

    PitchBendWidget(EditEvent *parent, const Event &event);

    PropertyNameSet getPropertyFilter() const override;

    /// Copy widget values to the Event.
    void updateEvent(Event &event) const override;

private:

    // Widgets

    QSpinBox *m_pitchBendSpinBox;

};


}

#endif
