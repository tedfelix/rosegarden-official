/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "PitchTrackerConfigurationPage.h"
#include "sound/Tuning.h"
#include "sound/PitchDetector.h"
#include "misc/ConfigGroups.h"
// Might need default analysis sizes
#include "gui/configuration/PitchTrackerConfigurationPage.h"
#include "sound/PitchDetector.h"
// Tunings are returned a stl vector. See sound/Tuning.h
#include <vector>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QSettings>
#include <QWidget>
#include <QHBoxLayout>
#include <QToolTip>
#include <QLayout>

namespace Rosegarden
{

const int PitchTrackerConfigurationPage::defaultGraphWidth = 4000;
const int PitchTrackerConfigurationPage::defaultGraphHeight = 100;
const bool PitchTrackerConfigurationPage::defaultIgnore8ve = true;

PitchTrackerConfigurationPage::PitchTrackerConfigurationPage(QWidget *parent) :
        TabbedConfigurationPage(parent),
        m_noTuningsAlert(parent)
{
    QSettings settings;
    settings.beginGroup(PitchTrackerConfigGroup);

    QFrame *frame;
    QGridLayout *layout;

    frame = new QFrame(m_tabWidget);
    frame->setContentsMargins(10, 10, 10, 10);
    layout = new QGridLayout(frame);
    layout->setSpacing(5);

    int row = 0;

    layout->setRowMinimumHeight(row, 15);
    ++row;

    layout->addWidget(new QLabel(tr("Tuning"), frame), row, 0);

    m_tuningMode = new QComboBox(frame);
    connect(m_tuningMode, SIGNAL(activated(int)),
            this, SLOT(slotModified()));
    m_tuningMode->setEditable(false);

    slotPopulateTuningCombo(true);

    int defaultTuningMode = settings.value("tuning", 0).toInt() ;
    m_tuningMode->setCurrentIndex(defaultTuningMode);
    layout->addWidget(m_tuningMode, row, 1, 1, 2);
    ++row;

    // TODO: write Edit Tuning dialogue to allow these to be edited
    // Currently, the settings make A440 in MIDI 12ET be the same note
    // in all tunings (a regression since rosegarden v1.2.4gpt1)
    m_rootPitch = 69;
    m_refPitch  = 69;
    m_refFreq   = 440.0;
    layout->addWidget(new QLabel(tr("Root Pitch"), frame), row, 0);
    m_rootPitchLabel = new QLabel("A3");
    layout->addWidget(m_rootPitchLabel, row, 1, 1, 2);
    row++;
    layout->addWidget(new QLabel(tr("Reference Pitch"), frame), row, 0);
    m_refPitchLabel = new QLabel("A3");
    layout->addWidget(m_refPitchLabel, row, 1, 1, 2);
    row++;
    layout->addWidget(new QLabel(tr("Reference Frequency"), frame), row, 0);
    m_refFreqLabel = new QLabel("440.0");
    layout->addWidget(m_refFreqLabel, row, 1, 1, 2);
    row++;

    layout->setRowStretch(row, 2);
    frame->setLayout(layout);
    addTab(frame, tr("General"));

    frame = new QFrame(m_tabWidget);
    frame->setContentsMargins(10, 10, 10, 10);
    layout = new QGridLayout(frame);
    layout->setSpacing(5);
    row = 0;

    // TODO: Add input source combo
    layout->addWidget(new QLabel(tr("Method"), frame), row, 0);
    m_method = new QComboBox(frame);
    connect(m_method, SIGNAL(activated(int)),
            this, SLOT(slotModified()));
    m_method->setEditable(false);
    const QVector<PitchDetector::Method> *pt_methods =
        PitchDetector::getMethods();
    if (pt_methods) {
        QVector<PitchDetector::Method>::const_iterator i;
        for (i = pt_methods->begin(); i != pt_methods->end(); ++i) {
            m_method->addItem(*i);
        }
        int defaultMethod = settings.value("method", 0).toInt() ;
        if (defaultMethod >= 0 && defaultMethod < pt_methods->size()) {
            m_method->setCurrentIndex(defaultMethod);
        }
    }
    layout->addWidget(m_method, row, 1, 1, 2);
    ++row;

    layout->addWidget(new QLabel(tr("Frame Size"), frame), row, 0);
    m_frameSize = new QSpinBox(frame);
    connect(m_frameSize, SIGNAL(valueChanged(int)),
            this, SLOT(slotModified()));
    m_frameSize->setMinimum(64);
    m_frameSize->setMaximum(32768);
    int frameSize = settings.value("framesize",
                                   PitchDetector::defaultFrameSize).toInt();
    if (frameSize >= 64 && frameSize <= 32768) {
        m_frameSize->setValue(frameSize);
    }
    layout->addWidget(m_frameSize, row, 1, 1, 2);
    ++row;

    layout->addWidget(new QLabel(tr("Step Size"), frame), row, 0);
    m_stepSize = new QSpinBox(frame);
    connect(m_stepSize, SIGNAL(valueChanged(int)),
            this, SLOT(slotModified()));
    m_stepSize->setMinimum(64);
    m_stepSize->setMaximum(8192);
    int stepSize = settings.value("stepsize",
                                  PitchDetector::defaultStepSize).toInt();
    if (stepSize >= 64 && stepSize <= 8192) {
        m_stepSize->setValue(stepSize);
    }
    layout->addWidget(m_stepSize, row, 1, 1, 2);
    ++row;

    layout->addWidget(new QLabel(tr("Ignore Octave Errors"), frame), row, 0);
    m_ignore8ve = new QCheckBox(frame);
    connect(m_ignore8ve, SIGNAL(stateChanged(int)),
            this, SLOT(slotModified()));
    bool defaultIgnore8ve =
           settings.value("ignoreoctave",
                          PitchTrackerConfigurationPage::defaultIgnore8ve
                         ).toBool();
    m_ignore8ve->setChecked(defaultIgnore8ve);
    layout->addWidget(m_ignore8ve, row, 1, 1, 2);
    ++row;

    layout->setRowStretch(row, 2);
    frame->setLayout(layout);
    addTab(frame, tr("Algorithm"));

    frame = new QFrame(m_tabWidget);
    frame->setContentsMargins(10, 10, 10, 10);
    layout = new QGridLayout(frame);
    layout->setSpacing(5);
    row = 0;
    layout->addWidget(new QLabel(tr("Graph Width (ms)"), frame), row, 0);
    m_graphWidth = new QSpinBox(frame);
    connect(m_graphWidth, SIGNAL(valueChanged(int)),
            this, SLOT(slotModified()));
    m_graphWidth->setMinimum(200);
    m_graphWidth->setMaximum(20000);
    const int graphWidth =
        settings.value("graphwidth", defaultGraphWidth).toInt();
    if (graphWidth >= 200 && graphWidth <= 20000) {
        m_graphWidth->setValue(graphWidth);
    }
    layout->addWidget(m_graphWidth, row, 1, 1, 2);
    ++row;

    layout->addWidget(new QLabel(tr("Graph Height (cents)"), frame), row, 0);
    m_graphHeight = new QSpinBox(frame);
    connect(m_graphHeight, SIGNAL(valueChanged(int)),
            this, SLOT(slotModified()));
    m_graphHeight->setMinimum(20);
    m_graphHeight->setMaximum(600);
    const int graphHeight =
        settings.value("graphheight", defaultGraphHeight).toInt();
    if (graphHeight >= 20 && graphHeight <= 600) {
        m_graphHeight->setValue(graphHeight);
    }
    layout->addWidget(m_graphHeight, row, 1, 1, 2);
    ++row;

    layout->setRowStretch(row, 2);
    frame->setLayout(layout);
    addTab(frame, tr("Appearance"));
    settings.endGroup();
}

void
PitchTrackerConfigurationPage::slotPopulateTuningCombo(bool rescan)
{
    // Read the tunings file to determine those available, and populate the
    // combo box.
    if (rescan || !m_tunings) m_tunings = Accidentals::Tuning::getTunings();

    if (m_tunings) {
        // Empty the tuning mode combo box and repopulate.
        while (m_tuningMode->count()) {
            m_tuningMode->removeItem(0);
        }

        std::vector<Accidentals::Tuning *>::iterator t;
        for (t = m_tunings->begin(); t != m_tunings->end(); ++t) {
            m_tuningMode->addItem(QString((*t)->getName().c_str()));
        }
    } else { // the tunings file couldn't be found, so display a message
        std::cout << "Pitch Tracker: Error: No tunings!!\n"
                     "Probably a missing tuning.xml file.\n"
                     "The user will have been warned." << std::endl;

        m_noTuningsAlert.showMessage(tr(
            "The tunings file could not be found! "
            "The file named \"tunings.xml\" containing tuning definitions "
            "has not been found in any of the standard "
            "directories. On Linux platforms, these include "
            "/usr/share/rosegarden/pitches, "
            "/usr/local/share/rosegarden/pitches and "
            "$HOME/.local/share/rosegarden/pitches. "
            "This file should be part of the standard installation."));
    }
}

void
PitchTrackerConfigurationPage::apply()
{
    QSettings settings;
    settings.beginGroup(PitchTrackerConfigGroup);

    settings.setValue("tuning", m_tuningMode->currentIndex());
    settings.setValue("rootpitch", "A3");
    settings.setValue("refpitch", "A3");
    settings.setValue("reffrequency", 440.0);
    settings.setValue("method", m_method->currentIndex());
    settings.setValue("framesize", m_frameSize->value());
    settings.setValue("stepsize", m_stepSize->value());
    settings.setValue("ignoreoctave", m_ignore8ve->isChecked());
    settings.setValue("graphwidth", m_graphWidth->value());
    settings.setValue("graphheight", m_graphHeight->value());

    settings.endGroup();
}

}
