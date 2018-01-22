
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

#include <QFrame>
#include <QString>
#include <vector>
#include "base/Event.h"
#include <QGroupBox>


class QWidget;
class QPushButton;
class QLabel;
class QVBoxLayout;
class QCheckBox;
class QComboBox;


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
    
    /**
     * Returned quantizer object is on heap -- caller must delete.
     */
    Quantizer *getQuantizer() const;

    QWidget *getAdvancedWidget() { return m_postProcessingBox; }

    bool shouldRebeam() const { return m_rebeam; }
    bool shouldDeCounterpoint() const { return m_deCounterpoint; }
    bool shouldMakeViable() const { return m_makeViable; }


public slots:
    void slotTypeChanged(int);

protected:
    std::vector<timeT> m_standardQuantizations;
    bool m_inNotation;

    QVBoxLayout *m_mainLayout;

    QComboBox *m_typeCombo;

    QGroupBox *m_gridBox;
    QCheckBox *m_durationCheckBox;
    QComboBox *m_gridUnitCombo;
    QLabel    *m_swingLabel;
    QComboBox *m_swingCombo;
    QLabel    *m_iterativeLabel;
    QComboBox *m_iterativeCombo;

    QGroupBox *m_notationBox;
    QCheckBox *m_notationTarget;
    QComboBox *m_notationUnitCombo;
    QComboBox *m_simplicityCombo;
    QComboBox *m_maxTuplet;
    QCheckBox *m_counterpoint;

    QPushButton *m_advancedButton;
    QGroupBox *m_postProcessingBox;
    QCheckBox *m_articulate;
    QCheckBox *m_makeViable;
    QCheckBox *m_deCounterpoint;
    QCheckBox *m_rebeam;
};



}

#endif
