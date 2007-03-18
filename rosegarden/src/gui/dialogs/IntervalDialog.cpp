/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "IntervalDialog.h"
#include <qlayout.h>

#include <iostream>
#include <klocale.h>
#include "misc/Strings.h"
#include "base/MidiDevice.h"
#include <kcombobox.h>
#include <kdialogbase.h>
#include <qframe.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qsizepolicy.h>
#include <qstring.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

IntervalDialog::IntervalDialog(QWidget *parent, bool askChangeKey, bool askTransposeSegmentBack) :
        KDialogBase(parent, 0, true, i18n("Specify Interval"), Ok | Cancel )
{
    QVBox *vBox = makeVBoxMainWidget();

    QHBox *hBox = new QHBox( vBox );

    m_referencenote = new DiatonicPitchChooser( i18n("Reference note:"), hBox );
    m_targetnote = new DiatonicPitchChooser( i18n("Target note:"), hBox );

    intervalChromatic = 0;
    intervalDiatonic = 0;

    //m_intervalPitchLabel = new QLabel( i18n("Pitch: %1").arg(intervalChromatic), hBox);
    //m_intervalOctavesLabel = new QLabel( i18n("Octaves: %1").arg(intervalDiatonic / 7), hBox);
    //m_intervalStepsLabel = new QLabel( i18n("Steps: %1").arg(intervalDiatonic % 7), hBox);
    m_intervalLabel = new QLabel( i18n("prime"), vBox);

    if (askChangeKey)
    {
               QButtonGroup *affectKeyGroup = new QButtonGroup(1, Horizontal, i18n("Affect key?"), vBox);
               m_transposeWithinKey = new QRadioButton(i18n("Transpose within key"), affectKeyGroup);
               m_transposeWithinKey->setChecked(true);
               m_transposeChangingKey = new QRadioButton(i18n("Change key for selection"), affectKeyGroup);
    }
    else
    {
       m_transposeChangingKey = NULL;
       m_transposeWithinKey = NULL;
    }
    
    if (askTransposeSegmentBack)
    {
    	m_transposeSegmentBack = new QCheckBox( "Adjust segment transposition in opposite direction (maintain audible pitch)", vBox );
    	m_transposeSegmentBack->setTristate(false);
    	m_transposeSegmentBack->setChecked(false);
    }
    else
    {
    	m_transposeSegmentBack = NULL;
    }

    connect(m_referencenote, SIGNAL(noteChanged(int,int,int)),
            this, SLOT(slotSetReferenceNote(int,int,int)));

    connect(m_targetnote, SIGNAL(noteChanged(int,int,int)),
            this, SLOT(slotSetTargetNote(int,int,int)));
}

// number of octaves the notes are apart
int
IntervalDialog::getOctaveDistance()
{
    return m_targetnote->getOctave() - m_referencenote->getOctave();
}

// input: C = 0, D = 1, E = 2, F = 3, etc
// output: C = 0, D = 2, E = 4, F = 5, etc
int
getChromaticStepValue(int stepnr)
{
    switch (stepnr)
    {
    case 0: //C
       return 0;
    case 1: //D
       return 2;
    case 2: //E
       return 4;
    case 3: //F
       return 5;
    case 4: //G
       return 7;
    case 5: //A
       return 9;
    case 6: //B
       return 11;
    }
}

// chromatic distance between the steps, not taking account octaves or 
// accidentals
int
IntervalDialog::getStepDistanceChromatic()
{
    return getChromaticStepValue(m_targetnote->getStep()) - getChromaticStepValue(m_referencenote->getStep());
    // - getChromaticStepValue(m_referencestep->currentItem());
    //return m_targetnote->getPitch() - m_referencenote->getPitch();
}

// correction due to accidentals
int
IntervalDialog::getAccidentalCorrectionChromatic()
{
    return m_targetnote->getAccidental() - m_referencenote->getAccidental();
}

int
IntervalDialog::getDiatonicDistance()
{
    return getOctaveDistance() * 7 + m_targetnote->getStep() - m_referencenote->getStep();
}

int
IntervalDialog::getChromaticDistance()
{
    return getOctaveDistance() * 12 + getStepDistanceChromatic() + getAccidentalCorrectionChromatic();
}

void
IntervalDialog::slotSetTargetNote(int pitch, int octave, int step)
{
    intervalChromatic = pitch - m_referencenote->getPitch();
    intervalDiatonic = (octave * 7 + step) - (m_referencenote->getOctave() * 7 + m_referencenote->getStep());

    //m_intervalPitchLabel->setText( i18n("Pitch: %1").arg(intervalChromatic) );
    //m_intervalOctavesLabel->setText( i18n("Octaves: %1").arg(intervalDiatonic / 7) );
    //m_intervalStepsLabel->setText( i18n("Step: %1").arg(intervalDiatonic % 7) );
    
    static QString stepName[] = {
       i18n("unison"), i18n("second"), i18n("third"), i18n("fourth"),
       i18n("fifth"),  i18n("sixth"),  i18n("seventh")
    };

    int displayInterval = intervalDiatonic;
    int displayIntervalChromatic = intervalChromatic;
    bool down = intervalDiatonic < 0;
    if (down)
    {
       displayInterval = -displayInterval;
       displayIntervalChromatic = -displayIntervalChromatic;
    }
    QString text = "";
    // diminished / small / / large / augmented / unknown
    static int stepIntervals[] = { 0,2,4,5,7,9,11 };
    int deviation = displayIntervalChromatic % 12 - stepIntervals[displayInterval % 7];
    switch (displayInterval % 7)
    {
    case 0:
    case 1:
    case 4:
    case 5:
       if (deviation == -1)
           text += "diminished ";
       else if (deviation == 1)
           text += "augmented ";
       else if (deviation == 0)
           text += "perfect ";
       else
           text += "(unknown) ";
       break;
    case 2:
    case 3:
    case 6:
       if (deviation == -1)
           text += "small ";
       else if (deviation == 0)
           text += "large ";
       else
           text += "(unknown) ";
       break;
    default:
       text += "(unknown) ";
    }
    
    text += stepName[displayInterval % 7];
    int octaves = displayInterval / 7;
    if (octaves == 1)
    {
       text += i18n(" and 1 octave");
    }
    else if (octaves > 1)
    {
       text += i18n(" and %1 octaves").arg(octaves);
    }

    if (displayInterval != 0)
    {
        if (!down)
        {
            text += i18n(" up");
        }
        else
        {
            text += i18n(" down");
       }
    }

    m_intervalLabel->setText( text );
}

void
IntervalDialog::slotSetReferenceNote(int pitch, int octave, int step)
{
    // recalculate target note based on reference note and current interval
    int pitch_new = pitch + intervalChromatic;
    int diatonic_new = (octave * 7 + step) + intervalDiatonic;
    int octave_new = diatonic_new / 7;
    int step_new = diatonic_new % 7;

    m_targetnote->slotSetNote( pitch_new, octave_new, step_new );
}

bool
IntervalDialog::getChangeKey()
{
    if (m_transposeChangingKey == NULL)
    {
        return false;
    }
    else
    {
        return m_transposeChangingKey->isChecked();
    }
}

bool
IntervalDialog::getTransposeSegmentBack()
{
	if (m_transposeSegmentBack == NULL)
	{
		return false;
	}
	else
	{
		return m_transposeSegmentBack->isChecked();	
	}
}

}
#include "IntervalDialog.moc"
