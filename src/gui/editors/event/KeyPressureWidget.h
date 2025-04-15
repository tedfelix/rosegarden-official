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

#ifndef RG_KEYPRESSUREWIDGET_H
#define RG_KEYPRESSUREWIDGET_H

#include "EventWidget.h"

class QSpinBox;


namespace Rosegarden
{


class EditEvent;
class Event;


class KeyPressureWidget : public EventWidget
{
public:

    KeyPressureWidget(EditEvent *parent, const Event &event);
    ~KeyPressureWidget();

    PropertyNameSet getPropertyFilter() const override;

    /// Copy widget values to the Event.
    void updateEvent(Event &event) const override;

private slots:

    void slotEditPitch(bool checked);

private:

    // Widgets

    QSpinBox *m_pitchSpinBox;
    QSpinBox *m_pressureSpinBox;

};


}

#endif
