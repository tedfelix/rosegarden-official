// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    This file is Copyright 2003
        D. Michael McIntyre <dmmcintyr@users.sourceforge.net>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#ifndef _eventfilter_H_
#define _eventfilter_H_

#include <qvariant.h>
#include <kdialog.h>
#include <kdialogbase.h>
#include "Quantizer.h"   // named durations
#include "qspinbox.h"
#include "qcombobox.h"
#include "qcheckbox.h"
#include "kconfig.h"
#include "widgets.h"

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QSpinBox;

class RosegardenPitchChooser;

/*
 * Creates a dialog box to allow the user to dial up various selection
 * criteria used for removing events from a selection.  It is up to the caller
 * to actually manipulate the selection.  After the dialog has been accepted,
 * its filterEvent() method can be used to decide whether a particular event
 * should continue to be selected.  See matrixview.cpp slotFilterSelection()
 * for an example of how to use this.
 */
class EventFilterDialog : public KDialog
{
    Q_OBJECT

public:

    EventFilterDialog(QWidget* parent);
    ~EventFilterDialog();

    KConfig *cfg;

    //-------[ accessor functions ]------------------------

    // NOTE: the filterRange type is used to return an A B pair with A and B set
    // according to the state of the related include/exclude combo.  If A > B
    // then this is an inclusive range.  If A < B then it's an exclusive
    // range.  This saves passing around a third variable.
    typedef std::pair<long, long> filterRange;

    filterRange getPitch();
    filterRange getVelocity();
    filterRange getDuration();

    filterRange getController();
    filterRange getValue();

    filterRange getWheel();

    bool filterNote() 	    { return m_noteCheckBox->isChecked();       }
    bool filterController() { return m_controllerCheckBox->isChecked(); }
    bool filterWheel()      { return m_wheelCheckBox->isChecked();      }
    
    // returns TRUE if the property value falls with in the filterRange 
    bool eventInRange(filterRange foo, long property) {
	if (foo.first > foo.second)
	    return (property <= foo.second || property >= foo.first);
	else
	    return (property >= foo.first && property <= foo.second); }

    // Used to do the work of deciding whether to keep or reject an event
    // based on the state of the dialog's widgets.  Returns TRUE if an event
    // should continue to be selected.  This method is the heart of the
    // EventFilterDialog's public interface.
    bool keepEvent(Rosegarden::Event* const &e);

protected:

    //--------[ member functions ]-------------------------
    
    // initialize the dialog
    void initDialog();

    // populate the duration combos
    void populateDurationCombos();

    // convert duration from combobox index into actual RG duration
    // between 0 and LONG_MAX
    long getDurationFromIndex(int index);

    // simple A B swap used to flip inclusive/exclusive values
    void invert (filterRange &);

    // return inclusive/exclusive toggle states concisely for tidy code
    bool pitchIsInclusive()    { return (m_notePitchIncludeComboBox->currentItem()    == 0); }
    bool velocityIsInclusive() { return (m_noteVelocityIncludeComboBox->currentItem() == 0); }
    bool durationIsInclusive() { return (m_noteDurationIncludeComboBox->currentItem() == 0); }

    bool controllerNumberIsInclusive() {
	                         return (m_controllerNumberIncludeComboBox->currentItem()
	    			                                                      == 0); }

    bool controllerValueIsInclusive()  {
	                         return (m_controllerValueIncludeComboBox->currentItem()
	                                                                              == 0); }

    bool wheelIsInclusive()    { return (m_wheelAmountIncludeComboBox->currentItem()  == 0); }
    

    //---------[ data members ]-----------------------------

    static const char * const ConfigGroup;
    
    QGridLayout* layout;
    
    QCheckBox* 	 m_controllerCheckBox;
    QCheckBox*	 m_noteCheckBox;
    QCheckBox*	 m_wheelCheckBox;
    QComboBox*   m_controllerNumberIncludeComboBox;
    QComboBox* 	 m_controllerValueIncludeComboBox;
    QComboBox*	 m_noteDurationFromComboBox;
    QComboBox* 	 m_noteDurationIncludeComboBox;
    QComboBox* 	 m_noteDurationToComboBox;
    QComboBox*	 m_notePitchIncludeComboBox;
    QComboBox* 	 m_noteVelocityIncludeComboBox;
    QComboBox* 	 m_wheelAmountIncludeComboBox;
    
    QLabel* 	 m_controllerFromLabel;
    QLabel*	 m_controllerToLabel;
    QLabel* 	 m_durationLabel;
    QLabel*  	 m_numberLabel;
    QLabel* 	 m_pitchFromLabel;
    QLabel* 	 m_pitchFromValueLabel;
    QLabel* 	 m_pitchLabel;
    QLabel* 	 m_pitchToLabel;
    QLabel*	 m_pitchToValueLabel;
    QLabel* 	 m_valueLabel;
    QLabel* 	 m_velocityLabel;
    QLabel* 	 m_wheelAmountLabel;
    QLabel* 	 m_wheelFromLabel;
    QLabel* 	 m_wheelToLabel;
    
    QPushButton* m_pitchFromChooserButton;
    QPushButton* m_pitchToChooserButton;
    QPushButton* m_buttonAll;
    QPushButton* m_buttonCancel;
    QPushButton* m_buttonNone;
    QPushButton* m_buttonOK;
    
    QSpinBox*  	 m_controllerNumberFromSpinBox;
    QSpinBox* 	 m_controllerNumberToSpinBox;
    QSpinBox* 	 m_controllerValueFromSpinBox;
    QSpinBox* 	 m_controllerValueToSpinBox;
    QSpinBox* 	 m_pitchFromSpinBox;
    QSpinBox* 	 m_pitchToSpinBox;
    QSpinBox* 	 m_velocityFromSpinBox;
    QSpinBox* 	 m_velocityToSpinBox;
    QSpinBox* 	 m_wheelAmountFromSpinBox;
    QSpinBox* 	 m_wheelAmountToSpinBox;

    std::vector<Rosegarden::timeT> m_standardQuantizations;

protected slots:

    // set widget values to include everything
    void slotToggleAll();

    // set widget values to include nothing
    void slotToggleNone();

    // write out settings to kconfig data for next time and call accept()
    void slotButtonOK();

    // hooked up to disable their associated widgets
    void slotNoteCheckBoxToggle(int);
    void slotControllerCheckBoxToggle(int);
    void slotWheelCheckBoxToggle(int);
    
    // update note name text display and ensure From <= To
    void slotPitchFromChanged(int pitch);
    void slotPitchToChanged(int pitch);
    
    // ensure From <= To to guarantee a logical range for these sets
    void slotVelocityFromChanged(int velocity);
    void slotVelocityToChanged(int velocity);
    void slotDurationFromChanged(int index);
    void slotDurationToChanged(int index);
    void slotControllerFromChanged(int controller);
    void slotControllerToChanged(int controller);
    void slotValueFromChanged(int value);
    void slotValueToChanged(int value);
    void slotWheelFromChanged(int value);
    void slotWheelToChanged(int value);

    // create a pitch chooser widget sub-dialog to show pitch on staff
    void slotPitchFromChooser();
    void slotPitchToChooser();
};


/*
 * Creates a small dialog box containing a RosegardenPitchChooser widget
 */
class PitchPickerDialog : public KDialogBase
{
    Q_OBJECT

public:

    PitchPickerDialog(QWidget* parent, int initialPitch, bool isFrom);
    ~PitchPickerDialog();

    int getPitch() { return m_pitch->getPitch(); }
    
private:
    RosegardenPitchChooser* m_pitch;
};


#endif // _eventfilter_H_
