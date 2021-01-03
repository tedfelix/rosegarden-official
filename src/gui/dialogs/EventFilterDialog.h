/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    This file is Copyright 2003-2006
        D. Michael McIntyre <dmmcintyr@users.sourceforge.net>

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_EVENTFILTERDIALOG_H
#define RG_EVENTFILTERDIALOG_H

#include <QDialog>
#include <utility>
#include <vector>
#include "base/Event.h"
#include <QCheckBox>
#include <QComboBox>

class QWidget;
class QSpinBox;
class QPushButton;
class QGridLayout;

namespace Rosegarden
{

class Event;


/**
 * Creates a dialog box to allow the user to dial up various selection
 * criteria used for removing events from a selection.  It is up to the caller
 * to actually manipulate the selection.  After the dialog has been accepted,
 * its filterEvent() method can be used to decide whether a particular event
 * should continue to be selected.  See matrixview.cpp slotFilterSelection()
 * for an example of how to use this.
 */
class EventFilterDialog : public QDialog
{
    Q_OBJECT

public:

    EventFilterDialog(QWidget* parent);
    ~EventFilterDialog() override;

    //-------[ accessor functions ]------------------------

    // NOTE: the filterRange type is used to return an A B pair with A and B set
    // according to the state of the related include/exclude combo.  If A > B
    // then this is an inclusive range.  If A < B then it's an exclusive
    // range.  This saves passing around a third variable.
    typedef std::pair<long, long> filterRange;

    filterRange getPitch();
    filterRange getVelocity();
    filterRange getDuration();

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
    bool keepEvent(Event* const &e);

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
    bool pitchIsInclusive()    { return (m_notePitchIncludeComboBox->currentIndex()    == 0); }
    bool velocityIsInclusive() { return (m_noteVelocityIncludeComboBox->currentIndex() == 0); }
    bool durationIsInclusive() { return (m_noteDurationIncludeComboBox->currentIndex() == 0); }

protected slots:

    // set widget values to include everything
    void slotToggleAll();

    // set widget values to include nothing
    void slotToggleNone();

    // write out settings to QSettings data for next time
    void accept() override;

    // update note name text display and ensure From <= To
    void slotPitchFromChanged(int pitch);
    void slotPitchToChanged(int pitch);
    
    // ensure From <= To to guarantee a logical range for these sets
    void slotVelocityFromChanged(int velocity);
    void slotVelocityToChanged(int velocity);
    void slotDurationFromChanged(int index);
    void slotDurationToChanged(int index);

    // create a pitch chooser widget sub-dialog to show pitch on staff
    void slotPitchFromChooser();
    void slotPitchToChooser();

    
private:
    //---------[ data members ]-----------------------------

    QGridLayout* layout;

    QComboBox*   m_noteDurationFromComboBox;
    QComboBox*   m_noteDurationIncludeComboBox;
    QComboBox*   m_noteDurationToComboBox;
    QComboBox*   m_notePitchIncludeComboBox;
    QComboBox*   m_noteVelocityIncludeComboBox;
    
    QPushButton* m_pitchFromChooserButton;
    QPushButton* m_pitchToChooserButton;
    QPushButton* m_buttonAll;
    QPushButton* m_buttonNone;
    
    QSpinBox*    m_pitchFromSpinBox;
    QSpinBox*    m_pitchToSpinBox;
    QSpinBox*    m_velocityFromSpinBox;
    QSpinBox*    m_velocityToSpinBox;

    QCheckBox*   m_useNotationDuration;
    QCheckBox*   m_selectRests;

    std::vector<timeT> m_standardQuantizations;

};


}

#endif
