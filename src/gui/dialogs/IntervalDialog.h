
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_INTERVALDIALOG_H
#define RG_INTERVALDIALOG_H

#include <QDialog>
#include <vector>
#include "gui/widgets/DiatonicPitchChooser.h"

#include <rosegardenprivate_export.h>

class QWidget;
class QComboBox;
class QRadioButton;
class QCheckBox;


namespace Rosegarden
{

class RosegardenDocument;


class ROSEGARDENPRIVATE_EXPORT IntervalDialog : public QDialog
{
    Q_OBJECT
public:
    IntervalDialog(QWidget *parent, bool askChangeKey = false, bool askTransposeSegmentBack = false);
    
    // Distance in semitones
    int getChromaticDistance();

    // Distance in steps
    int getDiatonicDistance();

    // Transpose within key or change the key?
    bool getChangeKey();
    
    // Transpose the segment itself in the opposite direction?
    bool getTransposeSegmentBack();
    
    static QString getIntervalName(int intervalDiatonic, int intervalChromatic);
    
public slots:
    void slotSetReferenceNote(int,int,int);
    void slotSetTargetNote(int,int,int);
    
private:
    int getOctaveDistance();
    int getStepDistanceChromatic();
    int getAccidentalCorrectionChromatic();
    
    DiatonicPitchChooser *m_referencenote;
    DiatonicPitchChooser *m_targetnote;

    QRadioButton *m_transposeWithinKey;
    QRadioButton *m_transposeChangingKey;
    bool changeKey;

	QCheckBox *m_transposeSegmentBack;

    int intervalChromatic;
    int intervalDiatonic;
    QLabel *m_intervalLabel;

};


}

#endif
