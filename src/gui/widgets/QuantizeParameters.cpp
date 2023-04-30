/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2023 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[QuantizeParameters]"

#include "QuantizeParameters.h"

#include "misc/Debug.h"
#include "misc/Strings.h"  // qStrToBool() etc...
#include "misc/ConfigGroups.h"  // *ConfigGroup names
#include "base/Quantizer.h"
#include "base/BasicQuantizer.h"
#include "base/LegatoQuantizer.h"
#include "base/NotationQuantizer.h"
#include "gui/editors/notation/NotationStrings.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include "gui/widgets/LineEdit.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QPixmap>
#include <QString>
#include <QVBoxLayout>


namespace Rosegarden
{


/// Add quantizations to the comboBox.
static void
addQuantizations(QComboBox *comboBox)
{
    QPixmap noMap = NotePixmapFactory::makeToolbarPixmap("menu-no-note");

    // For each standard quantization
    for (size_t i = 0; i < Quantizer::getQuantizations().size(); ++i) {

        const timeT time = Quantizer::getQuantizations()[i];
        timeT error = 0;

        QPixmap pmap = NotePixmapFactory::makeNoteMenuPixmap(time, error);

        // ??? If we get an error, just add a placeholder item now and continue.
        //     We should never get an error.

        QString label;
        // Perfect?  Create the label.
        if (error == 0)
            label = NotationStrings::makeNoteMenuLabel(time, false, error);

        // Perfect?  Add the icon and label.
        if (error == 0) {
            comboBox->addItem(pmap, label);
        } else {
            // ??? We never end up in here since we are iterating through
            //     the standard quantizations.  We can probably remove this
            //     and noMap above.
            comboBox->addItem(noMap, QString("%1").arg(time));
        }
    }
}

QuantizeParameters::QuantizeParameters(QWidget *parent,
                                       QuantizerType defaultQuantizer,
                                       bool showNotationOption) :
        QFrame(parent)
{
    const bool inNotation = (defaultQuantizer == Notation);

    m_settings.beginGroup(
            inNotation ? NotationQuantizeConfigGroup :
                         GridQuantizeConfigGroup);

    m_mainLayout = new QVBoxLayout;
    m_mainLayout->setSpacing(5);
    setContentsMargins(5, 5, 5, 5);
    setLayout(m_mainLayout);

    // ========================================================
    // Quantizer box
    // ========================================================

    QGroupBox *quantizerBox = new QGroupBox(tr("Quantizer"));
    QGridLayout *qbLayout = new QGridLayout;
    quantizerBox->setLayout(qbLayout);
    m_mainLayout->addWidget(quantizerBox);

    qbLayout->addWidget(new QLabel(tr("Quantizer type:"), quantizerBox), 0, 0);
    m_quantizerType = new QComboBox(quantizerBox);
    m_quantizerType->addItem(tr("Grid quantizer"));
    m_quantizerType->addItem(tr("Legato quantizer"));
    m_quantizerType->addItem(tr("Heuristic notation quantizer"));
    QuantizerType quantizerType = static_cast<QuantizerType>(
            m_settings.value("quantizetype", defaultQuantizer).toInt());
    m_quantizerType->setCurrentIndex(quantizerType);
    connect(m_quantizerType,
                static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &QuantizeParameters::slotTypeChanged);
    qbLayout->addWidget(m_quantizerType, 0, 1);

    // ??? This is never visible.  However, it is communicating to
    //     EventQuantizeCommand whether we are in notation via the
    //     quantizenotationonly .conf setting.  Make this more direct.
    //     Get rid of the invisible control and send the "inNotation"
    //     value to EventQuantizeCommand via a struct.
    m_quantizeNotation = new QCheckBox(
            tr("Quantize for notation only (leave performance unchanged)"),
            quantizerBox);
    m_quantizeNotation->setChecked(qStrToBool(m_settings.value(
            "quantizenotationonly", inNotation)));
    qbLayout->addWidget(m_quantizeNotation, 1, 0, 1, 2);

    // ??? Always false.  Only caller always sets this to false.
    //     I suspect this is supposed to be visible only "inNotation".
    //     That would allow the user to select between notation
    //     quantizing and MIDI data (performance) quantizing.
    if (!showNotationOption)
        m_quantizeNotation->hide();


    // ========================================================
    // Notation parameters box
    // ========================================================

    m_notationBox = new QGroupBox( tr("Notation parameters"));
    QGridLayout *nbLayout = new QGridLayout;
    nbLayout->setSpacing(3);
    m_notationBox->setLayout(nbLayout);
    m_mainLayout->addWidget(m_notationBox);

    // Base grid unit
    nbLayout->addWidget(new QLabel(tr("Base grid unit:"), m_notationBox), 1, 0);
    m_notationBaseGridUnit = new QComboBox(m_notationBox);
    initBaseGridUnit("notationBaseGridUnit", m_notationBaseGridUnit);
    nbLayout->addWidget(m_notationBaseGridUnit, 1, 1);

    // Complexity
    nbLayout->addWidget(new QLabel(tr("Complexity:"), m_notationBox), 0, 0);
    m_complexity = new QComboBox(m_notationBox);
    m_complexity->addItem(tr("Very high"));
    m_complexity->addItem(tr("High"));
    m_complexity->addItem(tr("Normal"));
    m_complexity->addItem(tr("Low"));
    m_complexity->addItem(tr("Very low"));
    m_complexity->setCurrentIndex(m_settings.value(
            "quantizesimplicity", 13).toInt() - 11);
    nbLayout->addWidget(m_complexity, 0, 1);

    // Tuplet level
    nbLayout->addWidget(new QLabel(tr("Tuplet level:"), m_notationBox), 2, 0);
    m_tupletLevel = new QComboBox(m_notationBox);
    m_tupletLevel->addItem(tr("None"));
    m_tupletLevel->addItem(tr("2-in-the-time-of-3"));
    m_tupletLevel->addItem(tr("Triplet"));
    m_tupletLevel->addItem(tr("Any"));
    m_tupletLevel->setCurrentIndex(m_settings.value(
            "quantizemaxtuplet", 3).toInt() - 1);
    nbLayout->addWidget(m_tupletLevel, 2, 1);

    // Permit counterpoint
    m_permitCounterpoint = new QCheckBox(tr("Permit counterpoint"), m_notationBox);
    m_permitCounterpoint->setChecked(qStrToBool(m_settings.value(
            "quantizecounterpoint", "false" )));
    nbLayout->addWidget(m_permitCounterpoint, 3, 0, 1, 1);


    // ========================================================
    // Grid parameters box
    // ========================================================

    m_gridBox = new QGroupBox( tr("Grid parameters"));
    QGridLayout *gbLayout = new QGridLayout;
    gbLayout->setSpacing(3);
    m_gridBox->setLayout(gbLayout);
    m_mainLayout->addWidget(m_gridBox);

    // Base grid unit
    gbLayout->addWidget(new QLabel(tr("Base grid unit:"), m_gridBox), 0, 0);
    m_gridBaseGridUnit = new QComboBox(m_gridBox);
    initBaseGridUnit("gridBaseGridUnit", m_gridBaseGridUnit);
    connect(m_gridBaseGridUnit, static_cast<void(QComboBox::*)(int)>(
                &QComboBox::currentIndexChanged),
            this, &QuantizeParameters::slotGridUnitChanged);
    gbLayout->addWidget(m_gridBaseGridUnit, 0, 1);

    // Arbitrary grid unit
    m_arbitraryGridUnitLabel = new QLabel(tr("Arbitrary grid unit:"), m_gridBox);;
    gbLayout->addWidget(m_arbitraryGridUnitLabel, 1, 0);
    m_arbitraryGridUnit = new LineEdit(m_gridBox);
    int arbitraryGridUnit = m_settings.value("arbitraryGridUnit", 1).toInt();
    m_arbitraryGridUnit->setText(QString::number(arbitraryGridUnit));
    gbLayout->addWidget(m_arbitraryGridUnit, 1, 1);
    // Enable/Disable Arbitrary grid unit controls as appropriate.
    slotGridUnitChanged(m_gridBaseGridUnit->currentIndex());

    // Swing
    m_swingLabel = new QLabel(tr("Swing:"), m_gridBox);
    gbLayout->addWidget(m_swingLabel, 2, 0);
    m_swing = new QComboBox(m_gridBox);

    int swing = m_settings.value("quantizeswing", 0).toInt();

    for (int i = -100; i <= 200; i += 10) {
        m_swing->addItem(i == 0 ? tr("None") : QString("%1%").arg(i));

        // Found it?  Select it.
        if (i == swing)
            m_swing->setCurrentIndex(m_swing->count() - 1);
    }

    gbLayout->addWidget(m_swing, 2, 1);

    // Iterative amount
    m_iterativeAmountLabel = new QLabel(tr("Iterative amount:"), m_gridBox);
    gbLayout->addWidget(m_iterativeAmountLabel, 3, 0);
    m_iterativeAmount = new QComboBox(m_gridBox);

    int iterativeAmount = m_settings.value("quantizeiterate", 100).toInt();

    for (int i = 10; i <= 100; i += 10) {
        m_iterativeAmount->addItem(
                i == 100 ? tr("Full quantize") : QString("%1%").arg(i));

        // Found it?  Select it.
        if (i == iterativeAmount)
            m_iterativeAmount->setCurrentIndex(m_iterativeAmount->count() - 1);
    }

    gbLayout->addWidget(m_iterativeAmount, 3, 1);

    // Quantize durations
    m_quantizeDurations = new QCheckBox(
            tr("Quantize durations as well as start times"), m_gridBox);
    m_quantizeDurations->setChecked(qStrToBool(m_settings.value(
            "quantizedurations", "false")));
    gbLayout->addWidget(m_quantizeDurations, 4, 0, 1, 1);

    // Remove notes smaller than
    m_removeNotesCheckBox = new QCheckBox(
            tr("Remove notes smaller than:"), m_gridBox);
    connect(m_removeNotesCheckBox, &QCheckBox::clicked,
            this, &QuantizeParameters::slotRemoveNotesClicked);
    const bool removeNotesEnabled = qStrToBool(m_settings.value(
            "quantizeremovenotes", "false"));
    m_removeNotesCheckBox->setChecked(removeNotesEnabled);
    gbLayout->addWidget(m_removeNotesCheckBox, 5, 0);
    m_removeNotesSmallerThan = new QComboBox(m_gridBox);
    addQuantizations(m_removeNotesSmallerThan);
    m_removeNotesSmallerThan->setEnabled(removeNotesEnabled);
    m_removeNotesSmallerThan->setCurrentIndex(m_settings.value(
            "quantizeremovenotessmallerthan",
            static_cast<int>(
                    BasicQuantizer::getQuantizations().size()) - 1).toInt());
    gbLayout->addWidget(m_removeNotesSmallerThan, 5, 1);

    // Remove articulations
    m_removeArticulations = new QCheckBox(
            tr("Remove articulations (staccato and tenuto)"), m_gridBox);
    m_removeArticulations->setChecked(qStrToBool(m_settings.value(
            "quantizeremovearticulations", "false")));
    gbLayout->addWidget(m_removeArticulations, 6, 0);


    // ========================================================
    // After quantization box
    // ========================================================

    QGroupBox *afterQuantizationBox =
            new QGroupBox(tr("After quantization"), this);
    QGridLayout *pbLayout = new QGridLayout;
    pbLayout->setSpacing(3);
    afterQuantizationBox->setLayout(pbLayout);
    m_mainLayout->addWidget(afterQuantizationBox);

    // Re-beam
    // ??? This is used via the "quantizerebeam" .conf setting by
    //     EventQuantizeCommand.  Make this more direct.
    m_rebeam = new QCheckBox(tr("Re-beam"), afterQuantizationBox);
    m_rebeam->setChecked(qStrToBool(m_settings.value(
            "quantizerebeam", "true")));

    // Add articulations
    // ??? This is used via the "quantizearticulate" .conf setting by
    //     EventQuantizeCommand.  Make this more direct.
    m_addArticulations = new QCheckBox(
            tr("Add articulations (staccato, tenuto, slurs)"),
            afterQuantizationBox);
    m_addArticulations->setChecked(qStrToBool(m_settings.value(
            "quantizearticulate", "true")));

    // Tie notes at barlines
    // ??? This is used via the "quantizemakeviable" .conf setting by
    //     EventQuantizeCommand.  Make this more direct.
    m_tieNotesAtBarlines = new QCheckBox(
            tr("Tie notes at barlines etc"), afterQuantizationBox);
    m_tieNotesAtBarlines->setChecked(qStrToBool(m_settings.value(
            "quantizemakeviable", "false")));

    // Split-and-tie overlapping chords
    // ??? This is used via the "quantizedecounterpoint" .conf setting by
    //     EventQuantizeCommand.  Make this more direct.
    m_splitAndTie = new QCheckBox(
            tr("Split-and-tie overlapping chords"), afterQuantizationBox);
    m_splitAndTie->setChecked(qStrToBool(m_settings.value(
            "quantizedecounterpoint", "false")));

    pbLayout->addWidget(m_rebeam, 0, 0);
    pbLayout->addWidget(m_addArticulations, 1, 0);
    pbLayout->addWidget(m_tieNotesAtBarlines, 2, 0);
    pbLayout->addWidget(m_splitAndTie, 3, 0);

    // Show/Hide widgets as appropriate for the quantizer type.
    slotTypeChanged(quantizerType);

}

void
QuantizeParameters::initBaseGridUnit(QString settingsKey, QComboBox *comboBox)
{
    QPixmap noMap = NotePixmapFactory::makeToolbarPixmap("menu-no-note");

    timeT baseGridUnit = m_settings.value(
            settingsKey,
            static_cast<int>(
                Note(Note::Demisemiquaver).getDuration())).toInt();

    bool found = false;

    // ??? We should use addQuantizations() to prime this instead
    //     of rolling another for-loop here.

    // For each standard quantization
    for (unsigned int i = 0; i < Quantizer::getQuantizations().size(); ++i) {

        timeT time = Quantizer::getQuantizations()[i];
        timeT error = 0;

        QPixmap pmap = NotePixmapFactory::makeNoteMenuPixmap(time, error);
        QString label;
        if (error == 0)
            label = NotationStrings::makeNoteMenuLabel(time, false, error);

        if (error == 0) {
            comboBox->addItem(pmap, label);
        } else {
            // ??? We never end up in here since we are iterating through
            //     the standard quantizations.  We can probably remove this.
            comboBox->addItem(noMap, QString("%1").arg(time));
        }

        // Found it?  Select it.
        if (Quantizer::getQuantizations()[i] == baseGridUnit) {
            comboBox->setCurrentIndex(comboBox->count() - 1);
            found = true;
        }
    }

    comboBox->addItem(noMap, tr("Arbitrary grid unit"));
    // Save the index for future reference.
    m_arbitraryGridUnitIndex = comboBox->count() - 1;

    // Nothing was found up to this point, go with arbitrary.
    if (!found)
        comboBox->setCurrentIndex(m_arbitraryGridUnitIndex);
}

void
QuantizeParameters::saveSettings()
{
    m_settings.setValue("quantizetype", m_quantizerType->currentIndex());
    m_settings.setValue("gridBaseGridUnit", static_cast<unsigned long long>(
            Quantizer::getQuantizations()[m_gridBaseGridUnit->currentIndex()]));
    m_settings.setValue("arbitraryGridUnit", m_arbitraryGridUnit->text());
    m_settings.setValue("notationBaseGridUnit", static_cast<unsigned long long>(
            Quantizer::getQuantizations()[m_notationBaseGridUnit->currentIndex()]));
    m_settings.setValue("quantizeswing", m_swing->currentIndex() * 10 - 100);
    m_settings.setValue("quantizeiterate",
                        m_iterativeAmount->currentIndex() * 10 + 10);
    m_settings.setValue("quantizenotationonly",
                        m_quantizeNotation->isChecked());
    m_settings.setValue("quantizedurations",
                        m_quantizeDurations->isChecked());

    m_settings.setValue("quantizeremovenotes",
                        m_removeNotesCheckBox->isChecked());
    m_settings.setValue("quantizeremovenotessmallerthan",
                        m_removeNotesSmallerThan->currentIndex());
    m_settings.setValue("quantizeremovearticulations",
                        m_removeArticulations->isChecked());

    m_settings.setValue("quantizesimplicity",
                        m_complexity->currentIndex() + 11);
    m_settings.setValue("quantizemaxtuplet",
                        m_tupletLevel->currentIndex() + 1);
    m_settings.setValue("quantizecounterpoint",
                        m_permitCounterpoint->isChecked());
    m_settings.setValue("quantizerebeam", m_rebeam->isChecked());
    m_settings.setValue("quantizearticulate", m_addArticulations->isChecked());
    m_settings.setValue("quantizemakeviable", m_tieNotesAtBarlines->isChecked());
    m_settings.setValue("quantizedecounterpoint", m_splitAndTie->isChecked());
}

timeT
QuantizeParameters::getGridUnit() const
{
    timeT unit = 1;

    // Arbitrary grid unit selected?
    if (m_gridBaseGridUnit->currentIndex() == m_arbitraryGridUnitIndex) {
        // Use the arbitrary grid unit field.
        unit = m_arbitraryGridUnit->text().toInt();
        if (unit < 1)
            unit = 1;
    } else {
        unit = Quantizer::getQuantizations()[
                m_gridBaseGridUnit->currentIndex()];
    }

    return unit;
}

std::shared_ptr<Quantizer>
QuantizeParameters::getQuantizer()
{
    // ??? Similar to EventQuantizeCommand::makeQuantizer().
    //     Can we pull out a common routine?  Maybe a factory function
    //     in Quantizer.  It would probably require a ton of parameters,
    //     though.

    QuantizerType type =
            static_cast<QuantizerType>(m_quantizerType->currentIndex());

    std::shared_ptr<Quantizer> quantizer;

    switch (type) {
    case Grid:
        {
            const std::string target =
                    (m_quantizeNotation->isChecked() ?
                            Quantizer::NotationPrefix : Quantizer::RawEventData);
            const timeT unit = getGridUnit();
            const int swingPercent = m_swing->currentIndex() * 10 - 100;
            const int iteratePercent =
                    m_iterativeAmount->currentIndex() * 10 + 10;

            std::shared_ptr<BasicQuantizer> basicQuantizer(new BasicQuantizer(
                    Quantizer::RawEventData,  // source
                    target,
                    unit,
                    m_quantizeDurations->isChecked(),  // doDurations
                    swingPercent,
                    iteratePercent));

            if (m_removeNotesCheckBox->isChecked()) {
                basicQuantizer->setRemoveSmaller(
                        Quantizer::getQuantizations()[
                                m_removeNotesSmallerThan->currentIndex()]);
            }

            basicQuantizer->setRemoveArticulations(
                    m_removeArticulations->isChecked());

            quantizer = basicQuantizer;

            break;
        }
    case Legato:
        {
            const timeT unit = getGridUnit();

            if (m_quantizeNotation->isChecked()) {
                quantizer = std::shared_ptr<Quantizer>(new LegatoQuantizer(
                        Quantizer::RawEventData,  // source
                        Quantizer::NotationPrefix,  // target
                        unit));
            } else {  // Quantize the events.
                quantizer = std::shared_ptr<Quantizer>(new LegatoQuantizer(
                        Quantizer::RawEventData,  // source
                        Quantizer::RawEventData,  // target
                        unit));
            }

            break;
        }
    case Notation:
        {

            std::shared_ptr<NotationQuantizer> notationQuantizer;

            if (m_quantizeNotation->isChecked()) {
                notationQuantizer = std::shared_ptr<NotationQuantizer>(
                        new NotationQuantizer());
            } else {
                notationQuantizer = std::shared_ptr<NotationQuantizer>(
                        new NotationQuantizer(
                                Quantizer::RawEventData,  // source
                                Quantizer::RawEventData));  // target
            }

            notationQuantizer->setUnit(Quantizer::getQuantizations()[
                    m_notationBaseGridUnit->currentIndex()]);
            notationQuantizer->setSimplicityFactor(
                    m_complexity->currentIndex() + 11);
            notationQuantizer->setMaxTuplet(m_tupletLevel->currentIndex() + 1);
            notationQuantizer->setContrapuntal(
                    m_permitCounterpoint->isChecked());
            notationQuantizer->setArticulate(m_addArticulations->isChecked());

            quantizer = notationQuantizer;

            break;
        }
    }

    return quantizer;
}

void
QuantizeParameters::slotTypeChanged(int index)
{
    setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum));
    parentWidget()->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));

    QuantizerType quantizerType = static_cast<QuantizerType>(index);

    switch (quantizerType) {
    case Grid:
        m_notationBox->hide();

        m_gridBox->show();
        m_swingLabel->show();
        m_swing->show();
        m_iterativeAmountLabel->show();
        m_iterativeAmount->show();
        m_quantizeDurations->show();
        m_removeNotesCheckBox->show();
        m_removeNotesSmallerThan->show();
        m_removeArticulations->show();

        break;

    case Legato:
        m_notationBox->hide();

        m_gridBox->show();
        m_swingLabel->hide();
        m_swing->hide();
        m_iterativeAmountLabel->hide();
        m_iterativeAmount->hide();
        m_quantizeDurations->hide();
        m_removeNotesCheckBox->hide();
        m_removeNotesSmallerThan->hide();
        m_removeArticulations->hide();

        break;

    case Notation:
        m_gridBox->hide();
        m_notationBox->show();

        break;
    }

    adjustSize();
    parentWidget()->adjustSize();
}

void
QuantizeParameters::slotGridUnitChanged(int index)
{
    // Enable/Disable Arbitrary grid unit widgets
    bool arbitraryEnabled = (index == m_arbitraryGridUnitIndex);
    m_arbitraryGridUnitLabel->setEnabled(arbitraryEnabled);
    m_arbitraryGridUnit->setEnabled(arbitraryEnabled);
    m_arbitraryGridUnit->setText(QString::number(getGridUnit()));
}

void
QuantizeParameters::slotRemoveNotesClicked(bool checked)
{
    m_removeNotesSmallerThan->setEnabled(checked);
}


}
