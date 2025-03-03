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

#ifndef RG_EVENTWIDGET_H
#define RG_EVENTWIDGET_H

#include "base/TimeT.h"

#include <QWidget>


namespace Rosegarden
{


class Event;


/// Baseclass for Event widgets.
class EventWidget : public QWidget
{
    Q_OBJECT

public:

    // Factory
    static EventWidget *create(QWidget *parent, const Event &event);

    EventWidget(QWidget *parent) :
        QWidget(parent)
    { }

    virtual timeT getDuration() const  { return 0; }

    virtual void updateEvent(Event & /*event*/) const  { }

};


}

#endif
