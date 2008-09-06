/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "IntervalDialog.h"
#include <QLayout>

#include <iostream>
#include <klocale.h>
#include "misc/Strings.h"
#include "base/MidiDevice.h"
#include "base/NotationRules.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGroupBox>
#include <QCheckBox>
#include <QLabel>
#include <QRadioButton>
#include <QGroupBox>
#include <QSizePolicy>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>


namespace Rosegarden
{

IntervalDialog::IntervalDialog(QDialogButtonBox::QWidget *parent, bool askChangeKey, bool askTransposeSegmentBack) :
        QDialog(parent)
{
    setModal(true);
    setWindowTitle(i18n("Specify Interval"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vBox = new QWidget(this);
    QVBoxLayout *vBoxLayout = new QVBoxLayout;
    metagrid->addWidget(vBox, 0, 0);


    QWidget *hBox = new QWidget( vBox );
    vBoxLayout->addWidget(hBox);
    QHBoxLayout *hBoxLayout = new QHBoxLayout;

    m_referencenote = new DiatonicPitchChooser(i18n("Reference note:"), hBox );
    hBoxLayout->addWidget(m_referencenote);
    m_targetnote = new DiatonicPitchChooser(i18n("Target note:"), hBox );
    hBoxLayout->addWidget(m_targetnote);
    hBox->setLayout(hBoxLayout);

    intervalChromatic = 0;
    intervalDiatonic = 0;

    //m_intervalPitchLabel = new QLabel( i18n("Pitch: %1").arg(intervalChromatic), hBox);
    //m_intervalOctavesLabel = new QLabel( i18n("Octaves: %1").arg(intervalDiatonic / 7), hBox);
    //m_intervalStepsLabel = new QLabel( i18n("Steps: %1").arg(intervalDiatonic % 7), hBox);

    m_intervalLabel = new QLabel(i18n("a perfect unison"), vBox );
    vBoxLayout->addWidget(m_intervalLabel);
    m_intervalLabel->setAlignment(Qt::AlignCenter);
    QFont font(m_intervalLabel->font());
    font.setItalic(true);
    m_intervalLabel->setFont(font);

    if (askChangeKey)
    {
        QGroupBox *affectKeyGroup = new QGroupBox( i18n("Effect on Key"), vBox );
        vBoxLayout->addWidget(affectKeyGroup);
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
        m_transposeSegmentBack = new QCheckBox(i18n("Adjust segment transposition in opposite direction (maintain audible pitch)"), vBox );
        vBoxLayout->addWidget(m_transposeSegmentBack);
        m_transposeSegmentBack->setTristate(false);
        m_transposeSegmentBack->setChecked(false);
    }
    else
    {
        m_transposeSegmentBack = NULL;
    }

    vBox->setLayout(vBoxLayout);

    connect(m_referencenote, SIGNAL(noteChanged(int,int,int)),
            this, SLOT(slotSetReferenceNote(int,int,int)));

    connect(m_targetnote, SIGNAL(noteChanged(int,int,int)),
            this, SLOT(slotSetTargetNote(int,int,int)));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

// number of octaves the notes are apart
int
IntervalDialog::getOctaveDistance()
{
    return m_targetnote->getOctave() - m_referencenote->getOctave();
}

// chromatic distance between the steps, not taking account octaves or 
// accidentals
int
IntervalDialog::getStepDistanceChromatic()
{
    return scale_Cmajor[m_targetnote->getStep()] - scale_Cmajor[m_referencenote->getStep()];
    // - getChromaticStepValue(m_referencestep->currentIndex());
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

QString
IntervalDialog::getIntervalName(int intervalDiatonic, int intervalChromatic)
{
    // displayInterval: an intervalDiatonic of -3 will yield a displayInterval of 3 and
    // set the boolean 'down' to true.
    int displayIntervalDiatonic = intervalDiatonic;
    int displayIntervalChromatic = intervalChromatic;
    bool down = (intervalDiatonic < 0 || 
                 (intervalDiatonic == 0 &&
                  intervalChromatic < 0));
    if (down)
    {
        displayIntervalDiatonic = -displayIntervalDiatonic;
        displayIntervalChromatic = -displayIntervalChromatic;
    }
    
    int octaves = displayIntervalDiatonic / 7;
    int deviation = displayIntervalChromatic % 12 - scale_Cmajor[displayIntervalDiatonic % 7];
    // Note (hjj):
    // "1 octave and a diminished octave" is better than
    // "2 octaves and a diminished unison"
    if (displayIntervalDiatonic % 7 == 0) {
        if (octaves > 0) {
            deviation = (deviation < 5 ? deviation : deviation - 12);
        } else if (octaves < 0) {
            deviation = (deviation < 5 ? -deviation : 12 - deviation);
        }
    } else if (down) {
	// Note (hjj):
	// an augmented prime down, NOT a diminished prime down
	deviation = -deviation;
    }
    
    // show the step for an unison only if the octave doesn't change, any other interval 
    //  always, and augmented/dimnished unisons (modulo octaves) always.
    bool showStep = displayIntervalDiatonic == 0 || 
        displayIntervalDiatonic % 7 != 0 || deviation != 0;
    
    QString textInterval = "";
    QString textIntervalDeviated = "";
    if (showStep)
    {
        switch (displayIntervalDiatonic % 7)
        {
        // First the diminished/perfect/augmented:
        case 0: // unison or octaves
        case 3: // fourth
        case 4: // fifth
           if (deviation == -1)
               textIntervalDeviated += i18n("a diminished");
           else if (deviation == 1)
               textIntervalDeviated += i18n("an augmented");
           else if (deviation == -2)
               textIntervalDeviated += i18n("a doubly diminished");
           else if (deviation == 2)
               textIntervalDeviated += i18n("a doubly augmented");
           else if (deviation == -3)
               textIntervalDeviated += i18n("a triply diminished");
           else if (deviation == 3)
               textIntervalDeviated += i18n("a triply augmented");
           else if (deviation == -4)
               textIntervalDeviated += i18n("a quadruply diminished");
           else if (deviation == 4)
               textIntervalDeviated += i18n("a quadruply augmented");
           else if (deviation == 0)
               textIntervalDeviated += i18n("a perfect");
           else
               textIntervalDeviated += i18n("an (unknown, %1)").arg(deviation);
           break;
        // Then the major/minor:
        case 1: // second
        case 2: // third
        case 5: // sixth
        case 6: // seventh
           if (deviation == -1)
               textIntervalDeviated += i18n("a minor");
           else if (deviation == 0)
               textIntervalDeviated += i18n("a major");
           else if (deviation == -2)
               textIntervalDeviated += i18n("a diminished");
           else if (deviation == 1)
               textIntervalDeviated += i18n("an augmented");
           else if (deviation == -3)
               textIntervalDeviated += i18n("a doubly diminished");
           else if (deviation == 2)
               textIntervalDeviated += i18n("a doubly augmented");
           else if (deviation == -4)
               textIntervalDeviated += i18n("a triply diminished");
           else if (deviation == 3)
               textIntervalDeviated += i18n("a triply augmented");
           else if (deviation == 4)
               textIntervalDeviated += i18n("a quadruply augmented");
           else if (deviation == 0)
               textIntervalDeviated += i18n("a perfect");
           else
               textIntervalDeviated += i18n("an (unknown, %1)").arg(deviation);
           break;
        default:
           textIntervalDeviated += i18n("an (unknown)");
        }
        switch (displayIntervalDiatonic % 7)
	{
	case 0:
	    // Note (hjj):
	    // "1 octave and a diminished octave" is better than
	    // "2 octaves and a diminished unison"
	    if (octaves > 0) {
	      textInterval += i18n("%1 octave").arg(textIntervalDeviated);
	      octaves--;
	    } else if (octaves < 0) {
	      textInterval += i18n("%1 octave").arg(textIntervalDeviated);
	      octaves++;
	    } else {
	      textInterval += i18n("%1 unison").arg(textIntervalDeviated);
	    }
	    break;
	case 1:
	    textInterval += i18n("%1 second").arg(textIntervalDeviated);
	    break;
	case 2:
	    textInterval += i18n("%1 third").arg(textIntervalDeviated);
	    break;
	case 3:
	    textInterval += i18n("%1 fourth").arg(textIntervalDeviated);
	    break;
	case 4:
	    textInterval += i18n("%1 fifth").arg(textIntervalDeviated);
	    break;
	case 5:
	    textInterval += i18n("%1 sixth").arg(textIntervalDeviated);
	    break;
	case 6:
	    textInterval += i18n("%1 seventh").arg(textIntervalDeviated);
	    break;
        default:
	    textInterval += i18n("%1").arg(textIntervalDeviated);
        }
    }
    
    if (displayIntervalChromatic != 0 || displayIntervalDiatonic != 0)
    {
        if (!down)
        {
	    if (octaves != 0) {
		if (showStep) {
		    return i18n("up 1 octave and %1",
		           "up %n octaves and %1",
			   octaves).arg(textInterval);
		} else {
		    return i18n("up 1 octave",
		           "up %n octaves",
			   octaves);
		}
	    } else {
		return i18n("up %1").arg(textInterval);
	    }
        }
        else
        {
	    if (octaves != 0) {
		if (showStep) {
		    return i18n("down 1 octave and %1",
		           "down %n octaves and %1",
			   octaves).arg(textInterval);
		} else {
		    return i18n("down 1 octave",
		           "down %n octaves",
			   octaves);
		}
	    } else {
		return i18n("down %1").arg(textInterval);
	    }
        }
    } else {
	return i18n("a perfect unison");
    }
}

void
IntervalDialog::slotSetTargetNote(int pitch, int octave, int step)
{
    intervalChromatic = pitch - m_referencenote->getPitch();
    intervalDiatonic = (octave * 7 + step) - (m_referencenote->getOctave() * 7 + m_referencenote->getStep());

    m_intervalLabel->setText( getIntervalName( intervalDiatonic, intervalChromatic ) );
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
