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

#ifndef RG_NOTEWIDGET_H
#define RG_NOTEWIDGET_H

#include "EventWidget.h"

class QGroupBox;
class QLabel;
class QPushButton;
class QSpinBox;


namespace Rosegarden
{


class Event;


class NoteWidget : public EventWidget
{
    //Q_OBJECT

public:

    NoteWidget(QWidget *parent, const Event &event);

    timeT getDuration() const override;
    int getPitch() const override;

private slots:

    void slotEditDuration(bool checked);
    void slotEditPitch(bool checked);

private:

    // Widgets

    // ??? Some of these don't need to be member variables.  Only the
    //     ones that contain data do.

    QSpinBox *m_durationSpinBox;
    QSpinBox *m_pitchSpinBox;
    QPushButton *m_pitchEditButton;

};


}

#endif
