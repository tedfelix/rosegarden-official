
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_DIATONICPITCHCHOOSER_H
#define RG_DIATONICPITCHCHOOSER_H

#include <QGroupBox>


class QString;
class QWidget;
class QSpinBox;
class QComboBox;
class QLabel;


namespace Rosegarden
{

class PitchDragLabel;


class DiatonicPitchChooser : public QGroupBox
{
    Q_OBJECT
public:
    DiatonicPitchChooser(QString title,
                           QWidget *parent,
                           int defaultNote = 0,
                           int defaultPitch = 60,
                           int defaultOctave = 5);
    
    // C0=0, D0=1, C1=12, etc.
    int getPitch() const;

    // C=0, D=1, E=2, F=3, etc.
    int getStep() const;

    // pitch 0 is the first C of octave 0.
    int getOctave() const;

    // 0  = none, 
    // -x = x flats
    // x  = x sharps
    int getAccidental();

signals:
    void pitchChanged(int);
    //pitch, octave, step
    void noteChanged(int,int,int);
    void preview(int);

public slots:
    void slotSetPitch(int);
    //pitch, octave, step
    void slotSetNote(int,int,int);
    void slotSetStep(int);
    void slotSetOctave(int);
    void slotSetAccidental(int);
    void slotResetToDefault();

protected:
    int m_defaultPitch;
    
    PitchDragLabel *m_pitchDragLabel;
    
    QComboBox *m_step;
    QComboBox *m_accidental;
    QComboBox *m_octave;
    
    QLabel *m_pitchLabel;

private:
    void setLabelsIfNeeded();
};

    

}

#endif
