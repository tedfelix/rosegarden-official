
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_PITCHCHOOSER_H
#define RG_PITCHCHOOSER_H

#include <QGroupBox>


class QString;
class QWidget;
class QSpinBox;
class QLabel;
class QVBoxLayout;


namespace Rosegarden
{

class PitchDragLabel;


class PitchChooser : public QGroupBox
{
    Q_OBJECT
public:
    PitchChooser(QString title,
                 QWidget *parent,
                 int defaultPitch = 60);
    
    int getPitch() const;

signals:
    void pitchChanged(int);
    void preview(int);

public slots:
    void slotSetPitch(int);
    void slotResetToDefault();
    void addWidgetToLayout(QWidget *widget);

protected:
    int m_defaultPitch;
    PitchDragLabel *m_pitchDragLabel;
    QSpinBox *m_pitch;
    QLabel *m_pitchLabel;
    QVBoxLayout *m_layout;
};

    

}

#endif
