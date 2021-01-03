
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_QUANTIZEPARAMETERS_H
#define RG_QUANTIZEPARAMETERS_H

#include "base/Event.h"  // For timeT

#include <QFrame>
#include <QGroupBox>
#include <QSettings>

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QVBoxLayout;
class QWidget;

#include <vector>


namespace Rosegarden
{


class LineEdit;
class Quantizer;


class QuantizeParameters : public QFrame
{
    Q_OBJECT

public:
    enum QuantizerType { Grid, Legato, Notation };

    QuantizeParameters(QWidget *parent,
                       QuantizerType defaultQuantizer,
                       bool showNotationOption);
    
    /// Call on "accept" to save the values for next time.
    void saveSettings();

    /// Returned quantizer object is on heap -- caller must delete.
    Quantizer *getQuantizer();

private slots:
    void slotTypeChanged(int);
    void gridUnitChanged(int index);

private:
    std::vector<timeT> m_standardQuantizations;
    /// Init a base grid unit combobox from settings.
    void initBaseGridUnit(QString settingsKey, QComboBox *comboBox);

    QSettings m_settings;

    QVBoxLayout *m_mainLayout;

    QComboBox *m_quantizerType;
    // ??? Hidden widget that is never shown.
    QCheckBox *m_quantizeNotation;

    // Grid Parameters
    QGroupBox *m_gridBox;
    QComboBox *m_gridBaseGridUnit;
    /// Index into m_gridBaseGridUnit for "Arbitrary gird unit".
    int m_arbitraryGridUnitIndex;
    QLabel *m_arbitraryGridUnitLabel;
    LineEdit *m_arbitraryGridUnit;
    /// Get the selected grid unit from m_gridBaseGridUnit and m_arbitraryGridUnit.
    timeT getGridUnit() const;
    QLabel *m_swingLabel;
    QComboBox *m_swing;
    QLabel *m_iterativeAmountLabel;
    QComboBox *m_iterativeAmount;
    QCheckBox *m_quantizeDurations;

    // Notation Parameters
    QGroupBox *m_notationBox;
    QComboBox *m_complexity;
    QComboBox *m_notationBaseGridUnit;
    QComboBox *m_tupletLevel;
    QCheckBox *m_permitCounterpoint;

    // After quantization
    QCheckBox *m_rebeam;
    QCheckBox *m_addArticulations;
    QCheckBox *m_tieNotesAtBarlines;
    QCheckBox *m_splitAndTie;

};


}

#endif
