/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
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
#include <QSizePolicy>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>


namespace Rosegarden
{

IntervalDialog::IntervalDialog(QWidget *parent, bool askChangeKey, bool askTransposeSegmentBack) :
        QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Specify Interval"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);

    QWidget *vBox = new QWidget;
    QVBoxLayout *vBoxLayout = new QVBoxLayout;
    metagrid->addWidget(vBox, 0, 0);

    QWidget *hBox = new QWidget;
    vBoxLayout->addWidget(hBox);
    QHBoxLayout *hBoxLayout = new QHBoxLayout;

    m_referencenote = new DiatonicPitchChooser(tr("Reference note:"), hBox );
    hBoxLayout->addWidget(m_referencenote);
    m_targetnote = new DiatonicPitchChooser(tr("Target note:"), hBox );
    hBoxLayout->addWidget(m_targetnote);
    hBox->setLayout(hBoxLayout);

    intervalChromatic = 0;
    intervalDiatonic = 0;

    m_intervalLabel = new QLabel(tr("a perfect unison"), vBox );
    vBoxLayout->addWidget(m_intervalLabel);
    m_intervalLabel->setAlignment(Qt::AlignCenter);
    QFont font(m_intervalLabel->font());
    font.setItalic(true);
    m_intervalLabel->setFont(font);

    if (askChangeKey) {
        QGroupBox *affectKeyGroup = new QGroupBox( tr("Effect on Key"), vBox );
        QVBoxLayout *affectKeyGroupLayout = new QVBoxLayout;
        vBoxLayout->addWidget(affectKeyGroup);
        m_transposeWithinKey = new QRadioButton(tr("Transpose within key"));
        affectKeyGroupLayout->addWidget(m_transposeWithinKey);
        m_transposeWithinKey->setChecked(true);
        m_transposeChangingKey = new QRadioButton(tr("Change key for selection"));
        affectKeyGroupLayout->addWidget(m_transposeChangingKey);
        affectKeyGroup->setLayout(affectKeyGroupLayout);
    } else {
        m_transposeChangingKey = nullptr;
        m_transposeWithinKey = nullptr;
    }
    
    if (askTransposeSegmentBack) {
        m_transposeSegmentBack = new QCheckBox(tr("Adjust segment transposition in opposite direction (maintain audible pitch)"), vBox );
        vBoxLayout->addWidget(m_transposeSegmentBack);
        m_transposeSegmentBack->setTristate(false);
        m_transposeSegmentBack->setChecked(false);
    } else {
        m_transposeSegmentBack = nullptr;
    }

    vBox->setLayout(vBoxLayout);

    connect(m_referencenote, &DiatonicPitchChooser::noteChanged,
            this, &IntervalDialog::slotSetReferenceNote);

    connect(m_targetnote, &DiatonicPitchChooser::noteChanged,
            this, &IntervalDialog::slotSetTargetNote);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
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
    if (down) {
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
    if (showStep) {
        switch (displayIntervalDiatonic % 7) {
        // First the diminished/perfect/augmented:
        case 0: // unison or octaves
        case 3: // fourth
        case 4: // fifth
           if (deviation == -1)
               textIntervalDeviated += tr("a diminished");
           else if (deviation == 1)
               textIntervalDeviated += tr("an augmented");
           else if (deviation == -2)
               textIntervalDeviated += tr("a doubly diminished");
           else if (deviation == 2)
               textIntervalDeviated += tr("a doubly augmented");
           else if (deviation == -3)
               textIntervalDeviated += tr("a triply diminished");
           else if (deviation == 3)
               textIntervalDeviated += tr("a triply augmented");
           else if (deviation == -4)
               textIntervalDeviated += tr("a quadruply diminished");
           else if (deviation == 4)
               textIntervalDeviated += tr("a quadruply augmented");
           else if (deviation == 0)
               textIntervalDeviated += tr("a perfect");
           else
               textIntervalDeviated += tr("an (unknown, %1)").arg(deviation);
           break;
        // Then the major/minor:
        case 1: // second
        case 2: // third
        case 5: // sixth
        case 6: // seventh
           if (deviation == -1)
               textIntervalDeviated += tr("a minor");
           else if (deviation == 0)
               textIntervalDeviated += tr("a major");
           else if (deviation == -2)
               textIntervalDeviated += tr("a diminished");
           else if (deviation == 1)
               textIntervalDeviated += tr("an augmented");
           else if (deviation == -3)
               textIntervalDeviated += tr("a doubly diminished");
           else if (deviation == 2)
               textIntervalDeviated += tr("a doubly augmented");
           else if (deviation == -4)
               textIntervalDeviated += tr("a triply diminished");
           else if (deviation == 3)
               textIntervalDeviated += tr("a triply augmented");
           else if (deviation == 4)
               textIntervalDeviated += tr("a quadruply augmented");
           else
               textIntervalDeviated += tr("an (unknown, %1)").arg(deviation);
           break;
        default:
           textIntervalDeviated += tr("an (unknown)");
        }
        switch (displayIntervalDiatonic % 7) {
	case 0:
	    // Note (hjj):
	    // "1 octave and a diminished octave" is better than
	    // "2 octaves and a diminished unison"
	    if (octaves > 0) {
	      textInterval += tr("%1 octave").arg(textIntervalDeviated);
	      octaves--;
	    } else if (octaves < 0) {
	      textInterval += tr("%1 octave").arg(textIntervalDeviated);
	      octaves++;
	    } else {
	      textInterval += tr("%1 unison").arg(textIntervalDeviated);
	    }
	    break;
	case 1:
	    textInterval += tr("%1 second").arg(textIntervalDeviated);
	    break;
	case 2:
	    textInterval += tr("%1 third").arg(textIntervalDeviated);
	    break;
	case 3:
	    textInterval += tr("%1 fourth").arg(textIntervalDeviated);
	    break;
	case 4:
	    textInterval += tr("%1 fifth").arg(textIntervalDeviated);
	    break;
	case 5:
	    textInterval += tr("%1 sixth").arg(textIntervalDeviated);
	    break;
	case 6:
	    textInterval += tr("%1 seventh").arg(textIntervalDeviated);
	    break;
        default:
	    textInterval += tr("%1").arg(textIntervalDeviated);
        }
    }
    
    if (displayIntervalChromatic != 0 || displayIntervalDiatonic != 0) {
        if (!down) {
	    if (octaves != 0) {
		if (showStep) {
		    return tr("up %n octave(s) and %1", "", octaves)
			   .arg(textInterval);
		} else {
		    return tr("up %n octave(s)", "", octaves);
		}
	    } else {
		return tr("up %1").arg(textInterval);
	    }
        } else {
	    if (octaves != 0) {
		if (showStep) {
		    return tr("down %n octave(s) and %1", "", octaves)
			   .arg(textInterval);
		} else {
		    return tr("down %n octave(s)", "", octaves);
		}
	    } else {
		return tr("down %1").arg(textInterval);
	    }
        }
    } else {
	return tr("a perfect unison");
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
    if (m_transposeChangingKey == nullptr) {
        return false;
    } else {
        return m_transposeChangingKey->isChecked();
    }
}

bool
IntervalDialog::getTransposeSegmentBack()
{
    if (m_transposeSegmentBack == nullptr) {
        return false;
    } else {
        return m_transposeSegmentBack->isChecked();	
    }
}

}
