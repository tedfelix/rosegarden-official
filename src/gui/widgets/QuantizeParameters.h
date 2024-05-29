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

#ifndef RG_QUANTIZEPARAMETERS_H
#define RG_QUANTIZEPARAMETERS_H

#include "base/Event.h"  // For timeT

#include <QFrame>
#include <QSettings>
#include <memory>

class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QVBoxLayout;
class QWidget;

#include <vector>


namespace Rosegarden
{


class LineEdit;
class Quantizer;


/// The parameter widgets within the Quantize dialog.
/**
 * Gets quantization parameters from the user and returns a properly configured
 * Quantizer via getQuantizer().
 */
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

    /// Returns a Quantizer configured to the user's specifications.
    std::shared_ptr<Quantizer> getQuantizer();

private slots:
    /// m_quantizerType
    void slotTypeChanged(int index);
    /// m_gridBaseGridUnit
    void slotGridUnitChanged(int index);
    /// m_removeNotesCheckBox
    void slotRemoveNotesClicked(bool checked);

private:
    QSettings m_settings;

    QVBoxLayout *m_mainLayout{};

    QComboBox *m_quantizerType{};
    // ??? Hidden widget that is never shown.
    QCheckBox *m_quantizeNotation{};

    // Grid Parameters
    QGroupBox *m_gridBox{};
    QComboBox *m_gridBaseGridUnit{};
    QLabel *m_arbitraryGridUnitLabel{};
    LineEdit *m_arbitraryGridUnit{};
    /// Get the selected grid unit from m_gridBaseGridUnit and m_arbitraryGridUnit.
    timeT getGridUnit() const;
    QLabel *m_swingLabel{};
    QComboBox *m_swing{};
    QLabel *m_iterativeAmountLabel{};
    QComboBox *m_iterativeAmount{};
    QCheckBox *m_quantizeDurations{};
    QCheckBox *m_removeNotesCheckBox{};
    QComboBox *m_removeNotesSmallerThan{};
    QCheckBox *m_removeArticulations{};

    // Notation Parameters
    QGroupBox *m_notationBox{};
    QComboBox *m_complexity{};
    QComboBox *m_notationBaseGridUnit{};
    QComboBox *m_tupletLevel{};
    QCheckBox *m_permitCounterpoint{};

    // After quantization
    QCheckBox *m_rebeam{};
    QCheckBox *m_addArticulations{};
    QCheckBox *m_tieNotesAtBarlines{};
    QCheckBox *m_splitAndTie{};

};


}

#endif
