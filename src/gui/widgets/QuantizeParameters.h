
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

#ifndef _RG_ROSEGARDENQUANTIZEPARAMETERS_H_
#define _RG_ROSEGARDENQUANTIZEPARAMETERS_H_

#include <qframe.h>
#include <qstring.h>
#include <vector>
#include "base/Event.h"
#include <qgroupbox.h>


class QWidget;
class QPushButton;
class QLabel;
class QGridLayout;
class QCheckBox;
class KComboBox;


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
                       bool showNotationOption,
                       bool showAdvancedButton,
                       QString configCategory,
                       QString preamble = 0);
    
    /**
     * Returned quantizer object is on heap -- caller must delete.
     * Also writes values to KConfig if so requested in constructor.
     */
    Quantizer *getQuantizer() const;

    QWidget *getAdvancedWidget() { return m_postProcessingBox; }

    bool shouldRebeam() const { return m_rebeam; }
    bool shouldDeCounterpoint() const { return m_deCounterpoint; }
    bool shouldMakeViable() const { return m_makeViable; }

    void showAdvanced(bool show);

public slots:
    void slotTypeChanged(int);
    void slotAdvancedChanged();

protected:
    QString m_configCategory;

    std::vector<timeT> m_standardQuantizations;

    QGridLayout *m_mainLayout;

    KComboBox *m_typeCombo;

    QGroupBox *m_gridBox;
    QCheckBox *m_durationCheckBox;
    KComboBox *m_gridUnitCombo;
    QLabel    *m_swingLabel;
    KComboBox *m_swingCombo;
    QLabel    *m_iterativeLabel;
    KComboBox *m_iterativeCombo;

    QGroupBox *m_notationBox;
    QCheckBox *m_notationTarget;
    KComboBox *m_notationUnitCombo;
    KComboBox *m_simplicityCombo;
    KComboBox *m_maxTuplet;
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
