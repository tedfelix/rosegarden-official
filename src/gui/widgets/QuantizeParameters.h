
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

#ifndef RG_QUANTIZEPARAMETERS_H
#define RG_QUANTIZEPARAMETERS_H

#include "base/Event.h"  // For timeT

#include <QFrame>
#include <QGroupBox>

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QVBoxLayout;
class QWidget;

#include <vector>


namespace Rosegarden
{


class Quantizer;


class QuantizeParameters : public QFrame
{
    Q_OBJECT

public:
    enum QuantizerType { Grid, Legato, Notation };

    QuantizeParameters(QWidget *parent,
                       QuantizerType defaultQuantizer,
                       bool showNotationOption);
    
    /// Returned quantizer object is on heap -- caller must delete.
    Quantizer *getQuantizer() const;

    QWidget *getAdvancedWidget()  { return m_postProcessingBox; }

private slots:
    void slotTypeChanged(int);

private:
    std::vector<timeT> m_standardQuantizations;

    /// Are we in the notation editor?
    /**
     * true if the default initial quantizer was the notation quantizer.
     * We assume this means the notation editor launched us.
     *
     * This is used to select which QSettings group we persist this
     * dialog to.
     *
     * ??? Make this a QString and set it to the proper group in the ctor.
     *     Or keep the QSettings instance as a member and set the group
     *     in the ctor.
     */
    bool m_inNotation;

    QVBoxLayout *m_mainLayout;

    QComboBox *m_quantizerType;
    // ??? Hidden widget that is never shown.
    QCheckBox *m_notationTarget;

    // Grid Parameters
    QGroupBox *m_gridBox;
    QComboBox *m_baseGridUnit;
    QLabel *m_swingLabel;
    QComboBox *m_swingCombo;
    QLabel *m_iterativeLabel;
    QComboBox *m_iterativeCombo;
    QCheckBox *m_durationCheckBox;

    // Notation Parameters
    QGroupBox *m_notationBox;
    QComboBox *m_simplicityCombo;
    QComboBox *m_notationUnitCombo;
    QComboBox *m_maxTuplet;
    QCheckBox *m_counterpoint;

    // After quantization
    QGroupBox *m_postProcessingBox;
    QCheckBox *m_rebeam;
    QCheckBox *m_articulate;
    QCheckBox *m_makeViable;
    QCheckBox *m_deCounterpoint;

};


}

#endif
