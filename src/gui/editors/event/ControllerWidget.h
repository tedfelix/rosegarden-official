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

#ifndef RG_CONTROLLERWIDGET_H
#define RG_CONTROLLERWIDGET_H

#include "EventWidget.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QSpinBox;


namespace Rosegarden
{


class EditEvent;
class Event;


class ControllerWidget : public EventWidget
{
public:

    ControllerWidget(EditEvent *parent, const Event &event);

    PropertyNameSet getPropertyFilter() const override;

    /// Copy widget values to the Event.
    void updateEvent(Event &event) const override;

private:

    EditEvent *m_parent;

    // Widgets

    QComboBox *m_numberComboBox;
    QSpinBox *m_valueSpinBox;

};


}

#endif
