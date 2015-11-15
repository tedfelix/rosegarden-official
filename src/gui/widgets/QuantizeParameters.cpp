/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "QuantizeParameters.h"

#include "misc/Strings.h"
#include "misc/ConfigGroups.h"
#include "base/NotationTypes.h"
#include "base/Quantizer.h"
#include "base/BasicQuantizer.h"
#include "base/LegatoQuantizer.h"
#include "base/NotationQuantizer.h"
#include "gui/editors/notation/NotationStrings.h"
#include "gui/editors/notation/NotePixmapFactory.h"

#include <QLayout>
#include <QApplication>
#include <QComboBox>
#include <QSettings>
#include <QCheckBox>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QObject>
#include <QPixmap>
#include <QPushButton>
#include <QString>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>


namespace Rosegarden
{


QuantizeParameters::QuantizeParameters(QWidget *parent,
                                       QuantizerType defaultQuantizer,
                                       bool showNotationOption) :
        QFrame(parent),
        m_standardQuantizations(BasicQuantizer::getStandardQuantizations())
{
    m_inNotation = (defaultQuantizer == Notation);

    m_mainLayout = new QVBoxLayout;
    m_mainLayout->setSpacing(5);
    setContentsMargins(5, 5, 5, 5);
    setLayout(m_mainLayout);

    // Quantizer box
    QGroupBox *quantizerBox = new QGroupBox(tr("Quantizer"));
    QGridLayout *qbLayout = new QGridLayout;
    quantizerBox->setLayout(qbLayout);
    m_mainLayout->addWidget(quantizerBox);

    qbLayout->addWidget(new QLabel(tr("Quantizer type:"), quantizerBox), 0, 0);
    m_typeCombo = new QComboBox(quantizerBox);
    m_typeCombo->addItem(tr("Grid quantizer"));
    m_typeCombo->addItem(tr("Legato quantizer"));
    m_typeCombo->addItem(tr("Heuristic notation quantizer"));
    qbLayout->addWidget(m_typeCombo, 0, 1);

    m_notationTarget = new QCheckBox
                       (tr("Quantize for notation only (leave performance unchanged)"),
                        quantizerBox);
    qbLayout->addWidget(m_notationTarget, 1, 0, 1, 2);

    if (!showNotationOption) m_notationTarget->hide();


    // Notation box
    m_notationBox = new QGroupBox( tr("Notation parameters"));
    QGridLayout *nbLayout = new QGridLayout;
    nbLayout->setSpacing(3);
    m_notationBox->setLayout(nbLayout);
    m_mainLayout->addWidget(m_notationBox);

    nbLayout->addWidget(new QLabel(tr("Base grid unit:"), m_notationBox), 1, 0);
    m_notationUnitCombo = new QComboBox(m_notationBox);
    nbLayout->addWidget(m_notationUnitCombo, 1, 1);

    nbLayout->addWidget(new QLabel(tr("Complexity:"), m_notationBox), 0, 0);

    m_simplicityCombo = new QComboBox(m_notationBox);
    m_simplicityCombo->addItem(tr("Very high"));
    m_simplicityCombo->addItem(tr("High"));
    m_simplicityCombo->addItem(tr("Normal"));
    m_simplicityCombo->addItem(tr("Low"));
    m_simplicityCombo->addItem(tr("Very low"));
    nbLayout->addWidget(m_simplicityCombo, 0, 1);

    nbLayout->addWidget(new QLabel(tr("Tuplet level:"), m_notationBox), 2, 0);
    m_maxTuplet = new QComboBox(m_notationBox);
    m_maxTuplet->addItem(tr("None"));
    m_maxTuplet->addItem(tr("2-in-the-time-of-3"));
    m_maxTuplet->addItem(tr("Triplet"));
    m_maxTuplet->addItem(tr("Any"));
    nbLayout->addWidget(m_maxTuplet, 2, 1);

    m_counterpoint = new QCheckBox(tr("Permit counterpoint"), m_notationBox);
    nbLayout->addWidget(m_counterpoint, 3, 0, 1, 1);


    // Grid box
    m_gridBox = new QGroupBox( tr("Grid parameters"));
    QGridLayout *gbLayout = new QGridLayout;
    gbLayout->setSpacing(3);
    m_gridBox->setLayout(gbLayout);
    m_mainLayout->addWidget(m_gridBox);

    gbLayout->addWidget(new QLabel(tr("Base grid unit:"), m_gridBox), 0, 0);
    m_gridUnitCombo = new QComboBox(m_gridBox);
    gbLayout->addWidget(m_gridUnitCombo, 0, 1);

    m_swingLabel = new QLabel(tr("Swing:"), m_gridBox);
    gbLayout->addWidget(m_swingLabel, 1, 0);
    m_swingCombo = new QComboBox(m_gridBox);
    gbLayout->addWidget(m_swingCombo, 1, 1);

    m_iterativeLabel = new QLabel(tr("Iterative amount:"), m_gridBox);
    gbLayout->addWidget(m_iterativeLabel, 2, 0);
    m_iterativeCombo = new QComboBox(m_gridBox);
    gbLayout->addWidget(m_iterativeCombo, 2, 1);

    m_durationCheckBox = new QCheckBox
                         (tr("Quantize durations as well as start times"), m_gridBox);
    gbLayout->addWidget(m_durationCheckBox, 3, 0, 1, 1);

    // Post-processing box
    m_postProcessingBox = new QGroupBox
                          (tr("After quantization"), this);
    QGridLayout *pbLayout = new QGridLayout;
    pbLayout->setSpacing(3);
    m_postProcessingBox->setLayout(pbLayout);
    m_mainLayout->addWidget(m_postProcessingBox);

    m_rebeam = new QCheckBox(tr("Re-beam"), m_postProcessingBox);
    m_articulate = new QCheckBox
                   (tr("Add articulations (staccato, tenuto, slurs)"), m_postProcessingBox);
    m_makeViable = new QCheckBox(tr("Tie notes at barlines etc"), m_postProcessingBox);
    m_deCounterpoint = new QCheckBox(tr("Split-and-tie overlapping chords"), m_postProcessingBox);

    pbLayout->addWidget(m_rebeam, 0, 0);
    pbLayout->addWidget(m_articulate, 1, 0);
    pbLayout->addWidget(m_makeViable, 2, 0);
    pbLayout->addWidget(m_deCounterpoint, 3, 0);

    QPixmap noMap = NotePixmapFactory::makeToolbarPixmap("menu-no-note");

    int defaultType = 0;
    timeT defaultUnit =
        Note(Note::Demisemiquaver).getDuration();

    int defaultSwing = 0;
    int defaultIterate = 100;

    QSettings settings;
    settings.beginGroup(m_inNotation ? NotationQuantizeConfigGroup : GridQuantizeConfigGroup);

    defaultType = settings.value("quantizetype", (defaultQuantizer == Notation)
            ? 2 : (defaultQuantizer == Legato) ? 1 : 0).toInt();
    defaultUnit = settings.value("quantizeunit", static_cast<unsigned long long>(defaultUnit)).toInt();
    defaultSwing = settings.value("quantizeswing", defaultSwing).toInt();
    defaultIterate = settings.value("quantizeiterate", defaultIterate).toInt();
    m_notationTarget->setChecked(qStrToBool(settings.value
            ("quantizenotationonly", (defaultQuantizer == Notation))));
    m_durationCheckBox->setChecked(qStrToBool(settings.value
            ("quantizedurations", "false")));
    m_simplicityCombo->setCurrentIndex(settings.value
            ("quantizesimplicity", 13).toInt()  - 11);
    m_maxTuplet->setCurrentIndex(settings.value
            ("quantizemaxtuplet", 3).toInt()  - 1);
    m_counterpoint->setChecked(qStrToBool(settings.value
            ("quantizecounterpoint", "false" )));
    m_rebeam->setChecked(qStrToBool(settings.value
            ("quantizerebeam", "true")));
    m_makeViable->setChecked(qStrToBool(settings.value
            ("quantizemakeviable", "false")));
    m_deCounterpoint->setChecked(qStrToBool(settings.value
            ("quantizedecounterpoint", "false")));
    m_articulate->setChecked(qStrToBool(settings.value
            ("quantizearticulate", "true")));

    settings.endGroup();

    m_postProcessingBox->show();

    for (unsigned int i = 0; i < m_standardQuantizations.size(); ++i) {

        timeT time = m_standardQuantizations[i];
        timeT error = 0;

        QPixmap pmap = NotePixmapFactory::makeNoteMenuPixmap(time, error);
        QString label = NotationStrings::makeNoteMenuLabel(time, false, error);

        if (error == 0) {
            m_gridUnitCombo->addItem(pmap, label);
            m_notationUnitCombo->addItem(pmap, label);
        } else {
            m_gridUnitCombo->addItem(noMap, QString("%1").arg(time));
            m_notationUnitCombo->addItem(noMap, QString("%1").arg(time));
        }

        if (m_standardQuantizations[i] == defaultUnit) {
            m_gridUnitCombo->setCurrentIndex(m_gridUnitCombo->count() - 1);
            m_notationUnitCombo->setCurrentIndex
            (m_notationUnitCombo->count() - 1);
        }
    }

    for (int i = -100; i <= 200; i += 10) {
        m_swingCombo->addItem(i == 0 ? tr("None") : QString("%1%").arg(i));
        if (i == defaultSwing)
            m_swingCombo->setCurrentIndex(m_swingCombo->count() - 1);
    }

    for (int i = 10; i <= 100; i += 10) {
        m_iterativeCombo->addItem(i == 100 ? tr("Full quantize") :
                                     QString("%1%").arg(i));
        if (i == defaultIterate)
            m_iterativeCombo->setCurrentIndex(m_iterativeCombo->count() - 1);
    }

    switch (defaultType) {
    case 0:  // grid
        m_gridBox->show();
        m_swingLabel->show();
        m_swingCombo->show();
        m_iterativeLabel->show();
        m_iterativeCombo->show();
        m_notationBox->hide();
        m_durationCheckBox->show();
        m_typeCombo->setCurrentIndex(0);
        break;
    case 1:  // legato
        m_gridBox->show();
        m_swingLabel->hide();
        m_swingCombo->hide();
        m_iterativeLabel->hide();
        m_iterativeCombo->hide();
        m_notationBox->hide();
        m_durationCheckBox->hide();
        m_typeCombo->setCurrentIndex(1);
    case 2:  // notation
        m_gridBox->hide();
        m_notationBox->show();
        m_typeCombo->setCurrentIndex(2);
        break;
    }

    connect(m_typeCombo, SIGNAL(activated(int)), SLOT(slotTypeChanged(int)));
}

Quantizer *
QuantizeParameters::getQuantizer() const
{
    //!!! Excessive duplication with
    // EventQuantizeCommand::makeQuantizer in editcommands.cpp

    int type = m_typeCombo->currentIndex();
    timeT unit = 0;

    if (type == 0 || type == 1) {
        unit = m_standardQuantizations[m_gridUnitCombo->currentIndex()];
    } else {
        unit = m_standardQuantizations[m_notationUnitCombo->currentIndex()];
    }

    Quantizer *quantizer = 0;

    int swing = m_swingCombo->currentIndex();
    swing *= 10;
    swing -= 100;

    int iterate = m_iterativeCombo->currentIndex();
    iterate *= 10;
    iterate += 10;

    if (type == 0) {

        if (m_notationTarget->isChecked()) {
            quantizer = new BasicQuantizer
                        (Quantizer::RawEventData,
                         Quantizer::NotationPrefix,
                         unit, m_durationCheckBox->isChecked(),
                         swing, iterate);
        } else {
            quantizer = new BasicQuantizer
                        (Quantizer::RawEventData,
                         Quantizer::RawEventData,
                         unit, m_durationCheckBox->isChecked(),
                         swing, iterate);
        }
    } else if (type == 1) {
        if (m_notationTarget->isChecked()) {
            quantizer = new LegatoQuantizer
                        (Quantizer::RawEventData,
                         Quantizer::NotationPrefix, unit);
        } else {
            quantizer = new LegatoQuantizer
                        (Quantizer::RawEventData,
                         Quantizer::RawEventData,
                         unit);
        }
    } else {

        NotationQuantizer *nq;

        if (m_notationTarget->isChecked()) {
            nq = new NotationQuantizer();
        } else {
            nq = new NotationQuantizer
                 (Quantizer::RawEventData,
                  Quantizer::RawEventData);
        }

        nq->setUnit(unit);
        nq->setSimplicityFactor(m_simplicityCombo->currentIndex() + 11);
        nq->setMaxTuplet(m_maxTuplet->currentIndex() + 1);
        nq->setContrapuntal(m_counterpoint->isChecked());
        nq->setArticulate(m_articulate->isChecked());

        quantizer = nq;
    }

    QSettings settings;
    settings.beginGroup(m_inNotation ? NotationQuantizeConfigGroup : GridQuantizeConfigGroup);

    settings.setValue("quantizetype", type);
    settings.setValue("quantizeunit", static_cast<unsigned long long>(unit));
    settings.setValue("quantizeswing", swing);
    settings.setValue("quantizeiterate", iterate);
    settings.setValue("quantizenotationonly",
                       m_notationTarget->isChecked());
    if (type == 0) {
        settings.setValue("quantizedurations",
                           m_durationCheckBox->isChecked());
    } else {
        settings.setValue("quantizesimplicity",
                           m_simplicityCombo->currentIndex() + 11);
        settings.setValue("quantizemaxtuplet",
                           m_maxTuplet->currentIndex() + 1);
        settings.setValue("quantizecounterpoint",
                           m_counterpoint->isChecked());
        settings.setValue("quantizearticulate",
                           m_articulate->isChecked());
    }
    settings.setValue("quantizerebeam", m_rebeam->isChecked());
    settings.setValue("quantizemakeviable", m_makeViable->isChecked());
    settings.setValue("quantizedecounterpoint", m_deCounterpoint->isChecked());

    settings.endGroup();

    return quantizer;
}

void
QuantizeParameters::slotTypeChanged(int index)
{
    setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum));
    parentWidget()->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));

    if (index == 0) {
        m_gridBox->show();
        m_swingLabel->show();
        m_swingCombo->show();
        m_iterativeLabel->show();
        m_iterativeCombo->show();
        m_durationCheckBox->show();
        m_notationBox->hide();
    } else if (index == 1) {
        m_gridBox->show();
        m_swingLabel->hide();
        m_swingCombo->hide();
        m_iterativeLabel->hide();
        m_iterativeCombo->hide();
        m_durationCheckBox->hide();
        m_notationBox->hide();
    } else {
        m_gridBox->hide();
        m_notationBox->show();
    }

    adjustSize();
    parentWidget()->adjustSize();
}

}
