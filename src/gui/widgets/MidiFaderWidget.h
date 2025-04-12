/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MIDIFADERWIDGET_H
#define RG_MIDIFADERWIDGET_H

#include <QFrame>
#include <QString>


class QWidget;
class QPushButton;
class QComboBox;


namespace Rosegarden
{

class Rotary;
class Fader;
class AudioVUMeter;


class MidiFaderWidget : public QFrame
{
    Q_OBJECT

public:
    MidiFaderWidget(QWidget *parent,
                    QString id = "");
    
    AudioVUMeter              *m_vuMeter;

    Fader           *m_fader;

    QPushButton               *m_muteButton;
    QPushButton               *m_soloButton;
    QPushButton               *m_recordButton;
    Rotary          *m_pan;

    QComboBox                 *m_output; 

    QString                    m_id;
};



}

#endif
