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

#include "SelectDialog.h"

// local includes
#include "misc/Debug.h"
#include "misc/ConfigGroups.h"

#include "gui/widgets/CheckButton.h"

// Qt includes
#include <QDialog>
#include <QUrl>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QSettings>
#include <QRadioButton>


namespace Rosegarden
{


SelectDialog::SelectDialog(QWidget *parent) :
    QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Search and Select"));

    // master layout
    QVBoxLayout *layout = new QVBoxLayout();
    setLayout(layout);

    // create tab widget
    m_tabWidget = new QTabWidget();
    layout->addWidget(m_tabWidget);

    // create duration tab
    makeDurationTab();
    m_tabWidget->addTab(m_durationTab, tr("Duration"));

    // create pitch tab
    makePitchTab();
    m_tabWidget->addTab(m_pitchTab, tr("Pitch"));

    // create special tab
    makeSpecialTab();
    m_tabWidget->addTab(m_specialTab, tr("Special"));

    // create advanced tab
    makeAdvancedTab();
    m_tabWidget->addTab(m_advancedTab, tr("Advanced"));

    // if there WAS an existing selection
    //
    // need memory in QSettings
    //
    // need proper layout
    m_replaceExistingSelection = new QRadioButton(tr("Replace existing selection"));
    m_addToExistingSelection = new QRadioButton(tr("Extend existing selection"));
    layout->addWidget(m_replaceExistingSelection);
    layout->addWidget(m_addToExistingSelection);

    // primary buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    layout->addWidget(buttonBox);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::helpRequested, this, &SelectDialog::help);

}

SelectDialog::~SelectDialog()
{
}

void
SelectDialog::accept()
{
    // Memory for the widgets that don't manage their own
    QSettings settings;
    settings.beginGroup(SelectDialogConfigGroup);
    settings.setValue("include_short_performance_durations", m_include_shorter_performance_durations->isChecked());
    settings.setValue("include_long_performance_durations", m_include_longer_performance_durations->isChecked());
    settings.endGroup();
    QDialog::accept();
}

void
SelectDialog::help()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:manual-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:manual-search-and-select-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}

// Duration Tab /////////////////////////////////////////////////////////////////
void
SelectDialog::makeDurationTab()
{
    // button grid
    QGridLayout *grid = new QGridLayout();
    m_durationTab = new QWidget();
    // pad the grid a slight amount horizontally so the labels don't get cut off
    grid->setHorizontalSpacing(15);
    m_durationTab->setLayout(grid);

    // breve buttons
    m_use_duration_breve = new CheckButton("breve");
    grid->addWidget(m_use_duration_breve, 0, 0);

    m_use_duration_breve_dotted = new CheckButton("dotted-breve");
    grid->addWidget(m_use_duration_breve_dotted, 0, 1);

    m_use_duration_breve_double_dotted = new CheckButton("double-dotted-breve");
    grid->addWidget(m_use_duration_breve_double_dotted, 0, 2);

    m_use_duration_breve_tuplet = new CheckButton("tuplet-breve");
    grid->addWidget(m_use_duration_breve_tuplet, 0, 3);

    m_use_duration_breve_rest = new CheckButton("rest-breve");
    grid->addWidget(m_use_duration_breve_rest, 0, 4);

    m_use_duration_breve_dotted_rest = new CheckButton("dotted-rest-breve");
    grid->addWidget(m_use_duration_breve_dotted_rest, 0, 5);

    m_use_duration_breve_double_dotted_rest = new CheckButton("double-dotted-rest-breve");
    grid->addWidget(m_use_duration_breve_double_dotted_rest, 0, 6);

    m_use_duration_breve_rest_tuplet = new CheckButton("rest-tuplet-breve");
    grid->addWidget(m_use_duration_breve_rest_tuplet, 0, 7);

    m_use_all_breves = new CheckButton("golden-arrow-left", false);
    grid->addWidget(m_use_all_breves, 0, 8);


    // semibreve buttons
    m_use_duration_semibreve = new CheckButton("semibreve");
    grid->addWidget(m_use_duration_semibreve, 1, 0);

    m_use_duration_semibreve_dotted = new CheckButton("dotted-semibreve");
    grid->addWidget(m_use_duration_semibreve_dotted, 1, 1);

    m_use_duration_semibreve_double_dotted = new CheckButton("double-dotted-semibreve");
    grid->addWidget(m_use_duration_semibreve_double_dotted, 1, 2);

    m_use_duration_semibreve_tuplet = new CheckButton("tuplet-semibreve");
    grid->addWidget(m_use_duration_semibreve_tuplet, 1, 3);

    m_use_duration_semibreve_rest = new CheckButton("rest-semibreve");
    grid->addWidget(m_use_duration_semibreve_rest, 1, 4);

    m_use_duration_semibreve_dotted_rest = new CheckButton("dotted-rest-semibreve");
    grid->addWidget(m_use_duration_semibreve_dotted_rest, 1, 5);

    m_use_duration_semibreve_double_dotted_rest = new CheckButton("double-dotted-rest-semibreve");
    grid->addWidget(m_use_duration_semibreve_double_dotted_rest, 1, 6);

    m_use_duration_semibreve_rest_tuplet = new CheckButton("rest-tuplet-semibreve");
    grid->addWidget(m_use_duration_semibreve_rest_tuplet, 1, 7);

    m_use_all_semibreves = new CheckButton("golden-arrow-left", false);
    grid->addWidget(m_use_all_semibreves, 1, 8);


    // minim buttons
    m_use_duration_minim = new CheckButton("minim");
    grid->addWidget(m_use_duration_minim, 2, 0);

    m_use_duration_minim_dotted = new CheckButton("dotted-minim");
    grid->addWidget(m_use_duration_minim_dotted, 2, 1);

    m_use_duration_minim_double_dotted = new CheckButton("double-dotted-minim");
    grid->addWidget(m_use_duration_minim_double_dotted, 2, 2);

    m_use_duration_minim_tuplet = new CheckButton("tuplet-minim");
    grid->addWidget(m_use_duration_minim_tuplet, 2, 3);

    m_use_duration_minim_rest = new CheckButton("rest-minim");
    grid->addWidget(m_use_duration_minim_rest, 2, 4);

    m_use_duration_minim_dotted_rest = new CheckButton("dotted-rest-minim");
    grid->addWidget(m_use_duration_minim_dotted_rest, 2, 5);

    m_use_duration_minim_double_dotted_rest = new CheckButton("double-dotted-rest-minim");
    grid->addWidget(m_use_duration_minim_double_dotted_rest, 2, 6);

    m_use_duration_minim_rest_tuplet = new CheckButton("rest-tuplet-minim");
    grid->addWidget(m_use_duration_minim_rest_tuplet, 2, 7);

    m_use_all_minims = new CheckButton("golden-arrow-left", false);
    grid->addWidget(m_use_all_minims, 2, 8);


    // crotchet buttons
    m_use_duration_crotchet = new CheckButton("crotchet");
    grid->addWidget(m_use_duration_crotchet, 3, 0);

    m_use_duration_crotchet_dotted = new CheckButton("dotted-crotchet");
    grid->addWidget(m_use_duration_crotchet_dotted, 3, 1);

    m_use_duration_crotchet_double_dotted = new CheckButton("double-dotted-crotchet");
    grid->addWidget(m_use_duration_crotchet_double_dotted, 3, 2);

    m_use_duration_crotchet_tuplet = new CheckButton("tuplet-crotchet");
    grid->addWidget(m_use_duration_crotchet_tuplet, 3, 3);

    m_use_duration_crotchet_rest = new CheckButton("rest-crotchet");
    grid->addWidget(m_use_duration_crotchet_rest, 3, 4);

    m_use_duration_crotchet_dotted_rest = new CheckButton("dotted-rest-crotchet");
    grid->addWidget(m_use_duration_crotchet_dotted_rest, 3, 5);

    m_use_duration_crotchet_double_dotted_rest = new CheckButton("double-dotted-rest-crotchet");
    grid->addWidget(m_use_duration_crotchet_double_dotted_rest, 3, 6);

    m_use_duration_crotchet_rest_tuplet = new CheckButton("rest-tuplet-crotchet");
    grid->addWidget(m_use_duration_crotchet_rest_tuplet, 3, 7);

    m_use_all_crotchets = new CheckButton("golden-arrow-left", false);
    grid->addWidget(m_use_all_crotchets, 3, 8);


    // quaver buttons
    m_use_duration_quaver = new CheckButton("quaver");
    grid->addWidget(m_use_duration_quaver, 4, 0);

    m_use_duration_quaver_dotted = new CheckButton("dotted-quaver");
    grid->addWidget(m_use_duration_quaver_dotted, 4, 1);

    m_use_duration_quaver_double_dotted = new CheckButton("double-dotted-quaver");
    grid->addWidget(m_use_duration_quaver_double_dotted, 4, 2);

    m_use_duration_quaver_tuplet = new CheckButton("tuplet-quaver");
    grid->addWidget(m_use_duration_quaver_tuplet, 4, 3);

    m_use_duration_quaver_rest = new CheckButton("rest-quaver");
    grid->addWidget(m_use_duration_quaver_rest, 4, 4);

    m_use_duration_quaver_dotted_rest = new CheckButton("dotted-rest-quaver");
    grid->addWidget(m_use_duration_quaver_dotted_rest, 4, 5);

    m_use_duration_quaver_double_dotted_rest = new CheckButton("double-dotted-rest-quaver");
    grid->addWidget(m_use_duration_quaver_double_dotted_rest, 4, 6);

    m_use_duration_quaver_rest_tuplet = new CheckButton("rest-tuplet-quaver");
    grid->addWidget(m_use_duration_quaver_rest_tuplet, 4, 7);

    m_use_all_quavers = new CheckButton("golden-arrow-left", false);
    grid->addWidget(m_use_all_quavers, 4, 8);


    // semiquaver buttons
    m_use_duration_semiquaver = new CheckButton("semiquaver");
    grid->addWidget(m_use_duration_semiquaver, 5, 0);

    m_use_duration_semiquaver_dotted = new CheckButton("dotted-semiquaver");
    grid->addWidget(m_use_duration_semiquaver_dotted, 5, 1);

    m_use_duration_semiquaver_double_dotted = new CheckButton("double-dotted-semiquaver");
    grid->addWidget(m_use_duration_semiquaver_double_dotted, 5, 2);

    m_use_duration_semiquaver_tuplet = new CheckButton("tuplet-semiquaver");
    grid->addWidget(m_use_duration_semiquaver_tuplet, 5, 3);

    m_use_duration_semiquaver_rest = new CheckButton("rest-semiquaver");
    grid->addWidget(m_use_duration_semiquaver_rest, 5, 4);

    m_use_duration_semiquaver_dotted_rest = new CheckButton("dotted-rest-semiquaver");
    grid->addWidget(m_use_duration_semiquaver_dotted_rest, 5, 5);

    m_use_duration_semiquaver_double_dotted_rest = new CheckButton("double-dotted-rest-semiquaver");
    grid->addWidget(m_use_duration_semiquaver_double_dotted_rest, 5, 6);

    m_use_duration_semiquaver_rest_tuplet = new CheckButton("rest-tuplet-semiquaver");
    grid->addWidget(m_use_duration_semiquaver_rest_tuplet, 5, 7);

    m_use_all_semiquavers = new CheckButton("golden-arrow-left", false);
    grid->addWidget(m_use_all_semiquavers, 5, 8);


    // demisemi buttons
    m_use_duration_demisemi = new CheckButton("demisemi");
    grid->addWidget(m_use_duration_demisemi, 6, 0);

    m_use_duration_demisemi_dotted = new CheckButton("dotted-demisemi");
    grid->addWidget(m_use_duration_demisemi_dotted, 6, 1);

    m_use_duration_demisemi_double_dotted = new CheckButton("double-dotted-demisemi");
    grid->addWidget(m_use_duration_demisemi_double_dotted, 6, 2);

    m_use_duration_demisemi_tuplet = new CheckButton("tuplet-demisemi");
    grid->addWidget(m_use_duration_demisemi_tuplet, 6, 3);

    m_use_duration_demisemi_rest = new CheckButton("rest-demisemi");
    grid->addWidget(m_use_duration_demisemi_rest, 6, 4);

    m_use_duration_demisemi_dotted_rest = new CheckButton("dotted-rest-demisemi");
    grid->addWidget(m_use_duration_demisemi_dotted_rest, 6, 5);

    m_use_duration_demisemi_double_dotted_rest = new CheckButton("double-dotted-rest-demisemi");
    grid->addWidget(m_use_duration_demisemi_double_dotted_rest, 6, 6);

    m_use_duration_demisemi_rest_tuplet = new CheckButton("rest-tuplet-demisemi");
    grid->addWidget(m_use_duration_demisemi_rest_tuplet, 6, 7);

    m_use_all_demisemis = new CheckButton("golden-arrow-left", false);
    grid->addWidget(m_use_all_demisemis, 6, 8);


    // hemidemisemi buttons
    m_use_duration_hemidemisemi = new CheckButton("hemidemisemi");
    grid->addWidget(m_use_duration_hemidemisemi, 7, 0);

    m_use_duration_hemidemisemi_dotted = new CheckButton("dotted-hemidemisemi");
    grid->addWidget(m_use_duration_hemidemisemi_dotted, 7, 1);

    m_use_duration_hemidemisemi_double_dotted = new CheckButton("double-dotted-hemidemisemi");
    grid->addWidget(m_use_duration_hemidemisemi_double_dotted, 7, 2);

    m_use_duration_hemidemisemi_tuplet = new CheckButton("tuplet-hemidemisemi");
    grid->addWidget(m_use_duration_hemidemisemi_tuplet, 7, 3);

    m_use_duration_hemidemisemi_rest = new CheckButton("rest-hemidemisemi");
    grid->addWidget(m_use_duration_hemidemisemi_rest, 7, 4);

    m_use_duration_hemidemisemi_dotted_rest = new CheckButton("dotted-rest-hemidemisemi");
    grid->addWidget(m_use_duration_hemidemisemi_dotted_rest, 7, 5);

    m_use_duration_hemidemisemi_double_dotted_rest = new CheckButton("double-dotted-rest-hemidemisemi");
    grid->addWidget(m_use_duration_hemidemisemi_double_dotted_rest, 7, 6);

    m_use_duration_hemidemisemi_rest_tuplet = new CheckButton("rest-tuplet-hemidemisemi");
    grid->addWidget(m_use_duration_hemidemisemi_rest_tuplet, 7, 7);

    m_use_all_hemidemisemis = new CheckButton("golden-arrow-left", false);
    grid->addWidget(m_use_all_hemidemisemis, 7, 8);


    // column use all buttons
    m_use_all_normals = new CheckButton("golden-arrow-up", false);
    grid->addWidget(m_use_all_normals, 8, 0);

    m_use_all_dotteds = new CheckButton("golden-arrow-up", false);
    grid->addWidget(m_use_all_dotteds, 8, 1);

    m_use_all_double_dotteds = new CheckButton("golden-arrow-up", false);
    grid->addWidget(m_use_all_double_dotteds, 8, 2);

    m_use_all_tuplets = new CheckButton("golden-arrow-up", false);
    grid->addWidget(m_use_all_tuplets, 8, 3);

    m_use_all_rests = new CheckButton("golden-arrow-up", false);
    grid->addWidget(m_use_all_rests, 8, 4);

    m_use_all_dotted_rests = new CheckButton("golden-arrow-up", false);
    grid->addWidget(m_use_all_dotted_rests, 8, 5);

    m_use_all_double_dotted_rests = new CheckButton("golden-arrow-up", false);
    grid->addWidget(m_use_all_double_dotted_rests, 8, 6);

    m_use_all_rest_tuplets = new CheckButton("golden-arrow-up", false);
    grid->addWidget(m_use_all_rest_tuplets, 8, 7);

    m_use_all_duration = new CheckButton("golden-arrow-left-up", false);
    grid->addWidget(m_use_all_duration, 8, 8);

    grid->addWidget(new QLabel(tr("Include notes with shorter performance durations")), 9, 0, 1, 7);
    m_include_shorter_performance_durations = new QCheckBox();
    grid->addWidget(m_include_shorter_performance_durations, 9, 8);

    grid->addWidget(new QLabel(tr("Include notes with longer performance durations")), 10, 0, 1, 7);
    m_include_longer_performance_durations = new QCheckBox();
    grid->addWidget(m_include_longer_performance_durations, 10, 8);

    

    // memory for the widgets who don't manage their own
    QSettings settings;
    settings.beginGroup(SelectDialogConfigGroup);
    m_include_shorter_performance_durations->setChecked(settings.value("include_short_performance_durations", true).toBool());
    m_include_longer_performance_durations->setChecked(settings.value("include_long_performance_durations", true).toBool());
    settings.endGroup();

    // connect the use_all buttons
    connect(m_use_all_breves, &QAbstractButton::toggled, this, &SelectDialog::slotUseAllBreve);
    connect(m_use_all_semibreves, &QAbstractButton::toggled, this, &SelectDialog::slotUseAllSemiBreve);
    connect(m_use_all_minims, &QAbstractButton::toggled, this, &SelectDialog::slotUseAllMinim);
    connect(m_use_all_crotchets, &QAbstractButton::toggled, this, &SelectDialog::slotUseAllCrotchet);
    connect(m_use_all_quavers, &QAbstractButton::toggled, this, &SelectDialog::slotUseAllQuaver);
    connect(m_use_all_semiquavers, &QAbstractButton::toggled, this, &SelectDialog::slotUseAllSemiQuaver);
    connect(m_use_all_demisemis, &QAbstractButton::toggled, this, &SelectDialog::slotUseAllDemiSemi);
    connect(m_use_all_hemidemisemis, &QAbstractButton::toggled, this, &SelectDialog::slotUseAllHemiDemiSemi);
    connect(m_use_all_normals, &QAbstractButton::toggled, this, &SelectDialog::slotUseAllNormal);
    connect(m_use_all_dotteds, &QAbstractButton::toggled, this, &SelectDialog::slotUseAllDotted);
    connect(m_use_all_double_dotteds, &QAbstractButton::toggled, this, &SelectDialog::slotUseAllDoubleDotted);
    connect(m_use_all_tuplets, &QAbstractButton::toggled, this, &SelectDialog::slotUseAllTuplet);
    connect(m_use_all_rests, &QAbstractButton::toggled, this, &SelectDialog::slotUseAllRestNormal);
    connect(m_use_all_dotted_rests, &QAbstractButton::toggled, this, &SelectDialog::slotUseAllRestDotted);
    connect(m_use_all_double_dotted_rests, &QAbstractButton::toggled, this, &SelectDialog::slotUseAllRestDoubleDotted);
    connect(m_use_all_rest_tuplets, &QAbstractButton::toggled, this, &SelectDialog::slotUseAllRestTuplet);
    connect(m_use_all_duration, &QAbstractButton::toggled, this, &SelectDialog::slotUseAllDuration);
  
}

void
SelectDialog::slotUseAllBreve(bool state)
{
    m_use_duration_breve->setChecked(state);
    m_use_duration_breve_dotted->setChecked(state);
    m_use_duration_breve_double_dotted->setChecked(state);
    m_use_duration_breve_tuplet->setChecked(state);
    m_use_duration_breve_rest->setChecked(state);
    m_use_duration_breve_dotted_rest->setChecked(state);
    m_use_duration_breve_double_dotted_rest->setChecked(state);
    m_use_duration_breve_rest_tuplet->setChecked(state);
}

void
SelectDialog::slotUseAllSemiBreve(bool state)
{
    m_use_duration_semibreve->setChecked(state);
    m_use_duration_semibreve_dotted->setChecked(state);
    m_use_duration_semibreve_double_dotted->setChecked(state);
    m_use_duration_semibreve_tuplet->setChecked(state);
    m_use_duration_semibreve_rest->setChecked(state);
    m_use_duration_semibreve_dotted_rest->setChecked(state);
    m_use_duration_semibreve_double_dotted_rest->setChecked(state);
    m_use_duration_semibreve_rest_tuplet->setChecked(state);
}

void
SelectDialog::slotUseAllMinim(bool state)
{
    m_use_duration_minim->setChecked(state);
    m_use_duration_minim_dotted->setChecked(state);
    m_use_duration_minim_double_dotted->setChecked(state);
    m_use_duration_minim_tuplet->setChecked(state);
    m_use_duration_minim_rest->setChecked(state);
    m_use_duration_minim_dotted_rest->setChecked(state);
    m_use_duration_minim_double_dotted_rest->setChecked(state);
    m_use_duration_minim_rest_tuplet->setChecked(state);
}


void
SelectDialog::slotUseAllCrotchet(bool state)
{
    m_use_duration_crotchet->setChecked(state);
    m_use_duration_crotchet_dotted->setChecked(state);
    m_use_duration_crotchet_double_dotted->setChecked(state);
    m_use_duration_crotchet_tuplet->setChecked(state);
    m_use_duration_crotchet_rest->setChecked(state);
    m_use_duration_crotchet_dotted_rest->setChecked(state);
    m_use_duration_crotchet_double_dotted_rest->setChecked(state);
    m_use_duration_crotchet_rest_tuplet->setChecked(state);
}

void
SelectDialog::slotUseAllQuaver(bool state)
{
    m_use_duration_quaver->setChecked(state);
    m_use_duration_quaver_dotted->setChecked(state);
    m_use_duration_quaver_double_dotted->setChecked(state);
    m_use_duration_quaver_tuplet->setChecked(state);
    m_use_duration_quaver_rest->setChecked(state);
    m_use_duration_quaver_dotted_rest->setChecked(state);
    m_use_duration_quaver_double_dotted_rest->setChecked(state);
    m_use_duration_quaver_rest_tuplet->setChecked(state);
}

void
SelectDialog::slotUseAllSemiQuaver(bool state)
{
    m_use_duration_semiquaver->setChecked(state);
    m_use_duration_semiquaver_dotted->setChecked(state);
    m_use_duration_semiquaver_double_dotted->setChecked(state);
    m_use_duration_semiquaver_tuplet->setChecked(state);
    m_use_duration_semiquaver_rest->setChecked(state);
    m_use_duration_semiquaver_dotted_rest->setChecked(state);
    m_use_duration_semiquaver_double_dotted_rest->setChecked(state);
    m_use_duration_semiquaver_rest_tuplet->setChecked(state);
}

void
SelectDialog::slotUseAllDemiSemi(bool state)
{
    m_use_duration_demisemi->setChecked(state);
    m_use_duration_demisemi_dotted->setChecked(state);
    m_use_duration_demisemi_double_dotted->setChecked(state);
    m_use_duration_demisemi_tuplet->setChecked(state);
    m_use_duration_demisemi_rest->setChecked(state);
    m_use_duration_demisemi_dotted_rest->setChecked(state);
    m_use_duration_demisemi_double_dotted_rest->setChecked(state);
    m_use_duration_demisemi_rest_tuplet->setChecked(state);
}

void
SelectDialog::slotUseAllHemiDemiSemi(bool state)
{
    m_use_duration_hemidemisemi->setChecked(state);
    m_use_duration_hemidemisemi_dotted->setChecked(state);
    m_use_duration_hemidemisemi_double_dotted->setChecked(state);
    m_use_duration_hemidemisemi_tuplet->setChecked(state);
    m_use_duration_hemidemisemi_rest->setChecked(state);
    m_use_duration_hemidemisemi_dotted_rest->setChecked(state);
    m_use_duration_hemidemisemi_double_dotted_rest->setChecked(state);
    m_use_duration_hemidemisemi_rest_tuplet->setChecked(state);
}

void
SelectDialog::slotUseAllNormal(bool state)
{
    m_use_duration_breve->setChecked(state);
    m_use_duration_semibreve->setChecked(state);
    m_use_duration_minim->setChecked(state);
    m_use_duration_crotchet->setChecked(state);
    m_use_duration_quaver->setChecked(state);
    m_use_duration_semiquaver->setChecked(state);
    m_use_duration_demisemi->setChecked(state);
    m_use_duration_hemidemisemi->setChecked(state);
}

void
SelectDialog::slotUseAllDotted(bool state)
{
    m_use_duration_breve_dotted->setChecked(state);
    m_use_duration_semibreve_dotted->setChecked(state);
    m_use_duration_minim_dotted->setChecked(state);
    m_use_duration_crotchet_dotted->setChecked(state);
    m_use_duration_quaver_dotted->setChecked(state);
    m_use_duration_semiquaver_dotted->setChecked(state);
    m_use_duration_demisemi_dotted->setChecked(state);
    m_use_duration_hemidemisemi_dotted->setChecked(state);
}

void
SelectDialog::slotUseAllDoubleDotted(bool state)
{
    m_use_duration_breve_double_dotted->setChecked(state);
    m_use_duration_semibreve_double_dotted->setChecked(state);
    m_use_duration_minim_double_dotted->setChecked(state);
    m_use_duration_crotchet_double_dotted->setChecked(state);
    m_use_duration_quaver_double_dotted->setChecked(state);
    m_use_duration_semiquaver_double_dotted->setChecked(state);
    m_use_duration_demisemi_double_dotted->setChecked(state);
    m_use_duration_hemidemisemi_double_dotted->setChecked(state);
}

void
SelectDialog::slotUseAllTuplet(bool state)
{
    m_use_duration_breve_tuplet->setChecked(state);
    m_use_duration_semibreve_tuplet->setChecked(state);
    m_use_duration_minim_tuplet->setChecked(state);
    m_use_duration_crotchet_tuplet->setChecked(state);
    m_use_duration_quaver_tuplet->setChecked(state);
    m_use_duration_semiquaver_tuplet->setChecked(state);
    m_use_duration_demisemi_tuplet->setChecked(state);
    m_use_duration_hemidemisemi_tuplet->setChecked(state);

}

void
SelectDialog::slotUseAllRestNormal(bool state)
{
    m_use_duration_breve_rest->setChecked(state);
    m_use_duration_semibreve_rest->setChecked(state);
    m_use_duration_minim_rest->setChecked(state);
    m_use_duration_crotchet_rest->setChecked(state);
    m_use_duration_quaver_rest->setChecked(state);
    m_use_duration_semiquaver_rest->setChecked(state);
    m_use_duration_demisemi_rest->setChecked(state);
    m_use_duration_hemidemisemi_rest->setChecked(state);

}

void
SelectDialog::slotUseAllRestDotted(bool state)
{
    m_use_duration_breve_dotted_rest->setChecked(state);
    m_use_duration_semibreve_dotted_rest->setChecked(state);
    m_use_duration_minim_dotted_rest->setChecked(state);
    m_use_duration_crotchet_dotted_rest->setChecked(state);
    m_use_duration_quaver_dotted_rest->setChecked(state);
    m_use_duration_semiquaver_dotted_rest->setChecked(state);
    m_use_duration_demisemi_dotted_rest->setChecked(state);
    m_use_duration_hemidemisemi_dotted_rest->setChecked(state);

}

void
SelectDialog::slotUseAllRestDoubleDotted(bool state)
{
    m_use_duration_breve_double_dotted_rest->setChecked(state);
    m_use_duration_semibreve_double_dotted_rest->setChecked(state);
    m_use_duration_minim_double_dotted_rest->setChecked(state);
    m_use_duration_crotchet_double_dotted_rest->setChecked(state);
    m_use_duration_quaver_double_dotted_rest->setChecked(state);
    m_use_duration_semiquaver_double_dotted_rest->setChecked(state);
    m_use_duration_demisemi_double_dotted_rest->setChecked(state);
    m_use_duration_hemidemisemi_double_dotted_rest->setChecked(state);

}

void
SelectDialog::slotUseAllRestTuplet(bool state)
{
    m_use_duration_breve_rest_tuplet->setChecked(state);
    m_use_duration_semibreve_rest_tuplet->setChecked(state);
    m_use_duration_minim_rest_tuplet->setChecked(state);
    m_use_duration_crotchet_rest_tuplet->setChecked(state);
    m_use_duration_quaver_rest_tuplet->setChecked(state);
    m_use_duration_semiquaver_rest_tuplet->setChecked(state);
    m_use_duration_demisemi_rest_tuplet->setChecked(state);
    m_use_duration_hemidemisemi_rest_tuplet->setChecked(state);
}

void
SelectDialog::slotUseAllDuration(bool state)
{
    slotUseAllBreve(state);
    slotUseAllSemiBreve(state);
    slotUseAllMinim(state);
    slotUseAllCrotchet(state);
    slotUseAllQuaver(state);
    slotUseAllSemiQuaver(state);
    slotUseAllDemiSemi(state);
    slotUseAllHemiDemiSemi(state);
    slotUseAllNormal(state);
    slotUseAllDotted(state);
    slotUseAllDoubleDotted(state);
    slotUseAllTuplet(state);
    slotUseAllRestNormal(state);
    slotUseAllRestDotted(state);
    slotUseAllRestDoubleDotted(state);
    slotUseAllRestTuplet(state);
}


// Pitch Tab ////////////////////////////////////////////////////////////////////
void
SelectDialog::makePitchTab()
{
    // button grid
    QGridLayout *grid = new QGridLayout();
    m_pitchTab = new QWidget();
    // pad the grid a slight amount horizontally so the labels don't get cut off
    grid->setHorizontalSpacing(15);
    m_pitchTab->setLayout(grid);

    grid->addWidget(new QLabel("My creator has so little clue what form I shall take that he isn't sure whether I'll even have a grid layout yet.  Joy joy."), 1, 1);
}

// Special Tab //////////////////////////////////////////////////////////////////
void
SelectDialog::makeSpecialTab()
{
    // button grid
    QGridLayout *grid = new QGridLayout();
    m_specialTab = new QWidget();
    // pad the grid a slight amount horizontally so the labels don't get cut off
    grid->setHorizontalSpacing(15);
    m_specialTab->setLayout(grid);
    
    // row 1 buttons
    m_useTenuto = new CheckButton("tenuto");
    grid->addWidget(m_useTenuto, 1, 1);

    m_useStaccato = new CheckButton("staccato");
    grid->addWidget(m_useStaccato, 1, 2);

    m_useStaccatissimo = new CheckButton("staccatissimo");
    grid->addWidget(m_useStaccatissimo, 1, 3);

    m_useMarcato = new CheckButton("marcato");
    grid->addWidget(m_useMarcato, 1, 4);

    m_useOpen = new CheckButton("open");
    grid->addWidget(m_useOpen, 1, 5);

    m_useStopped = new CheckButton("stopped");
    grid->addWidget(m_useStopped, 1, 6);

    m_useHarmonic = new CheckButton("harmonic");
    grid->addWidget(m_useHarmonic, 1, 7);

    m_useRow1 = new CheckButton("golden-arrow-left", false);
    grid->addWidget(m_useRow1, 1, 8);


    // row 2 buttons
    m_useUpBow = new CheckButton("up-bow");
    grid->addWidget(m_useUpBow, 2, 1);

    m_useDownBow = new CheckButton("down-bow");
    grid->addWidget(m_useDownBow, 2, 2);

    m_useOneSlash = new CheckButton("slash1");
    grid->addWidget(m_useOneSlash, 2, 3);

    m_useTwoSlash = new CheckButton("slash2");
    grid->addWidget(m_useTwoSlash, 2, 4);

    m_useThreeSlash = new CheckButton("slash3");
    grid->addWidget(m_useThreeSlash, 2, 5);

    m_useFourSlash = new CheckButton("slash4");
    grid->addWidget(m_useFourSlash, 2, 6);

    m_useFiveSlash = new CheckButton("slash5");
    grid->addWidget(m_useFiveSlash, 2, 7);

    m_useRow2 = new CheckButton("golden-arrow-left", false);
    grid->addWidget(m_useRow2, 2, 8);


    // row 3 buttons
    m_useSforzando = new CheckButton("sf");
    grid->addWidget(m_useSforzando, 3, 1);

    m_useRinforzando = new CheckButton("rf");
    grid->addWidget(m_useRinforzando, 3, 2);

    m_useTrill = new CheckButton("trill");
    grid->addWidget(m_useTrill, 3, 3);

    m_useTrillLineIndication = new CheckButton("trill-line");
    grid->addWidget(m_useTrillLineIndication, 3, 4);

    m_useTurn = new CheckButton("turn");
    grid->addWidget(m_useTurn, 3, 5);

    m_useMordent = new CheckButton("mordent");
    grid->addWidget(m_useMordent, 3, 6);

    m_useMordentInverted = new CheckButton("inverted-mordent");
    grid->addWidget(m_useMordentInverted, 3, 7);

    m_useRow3 = new CheckButton("golden-arrow-left", false);
    grid->addWidget(m_useRow3, 3, 8);


    // row 4 buttons
    m_useMordentLong = new CheckButton("long-mordent");
    grid->addWidget(m_useMordentLong, 4, 1);

    m_useMordentLongInverted = new CheckButton("long-inverted-mordent");
    grid->addWidget(m_useMordentLongInverted, 4, 2);

    m_useCrescendo = new CheckButton("group-crescendo");
    grid->addWidget(m_useCrescendo, 4, 3);

    m_useDecrescendo = new CheckButton("group-decrescendo");
    grid->addWidget(m_useDecrescendo, 4, 4);

    m_useSlur = new CheckButton("group-slur");
    grid->addWidget(m_useSlur, 4, 5);

    m_usePhrasingSlur = new CheckButton("phrasing-slur");
    grid->addWidget(m_usePhrasingSlur, 4, 6);

    m_usePause = new CheckButton("pause");
    grid->addWidget(m_usePause, 4, 7);

    m_useRow4 = new CheckButton("golden-arrow-left", false);
    grid->addWidget(m_useRow4, 4, 8);


    // row 5 buttons
    m_useQuindicesimaUp = new CheckButton("15ma-up");
    grid->addWidget(m_useQuindicesimaUp, 5, 1);

    m_useOttavaUp = new CheckButton("8va-up");
    grid->addWidget(m_useOttavaUp, 5, 2);

    m_useOttavaDown = new CheckButton("8va-down");
    grid->addWidget(m_useOttavaDown, 5, 3);

    m_useQuindicesimaDown = new CheckButton("15ma-down");
    grid->addWidget(m_useQuindicesimaDown, 5, 4);

    m_useSegno = new CheckButton("segno");
    grid->addWidget(m_useSegno, 5, 5);

    m_useCoda = new CheckButton("coda");
    grid->addWidget(m_useCoda, 5, 6);

    m_useBreath = new CheckButton("breath");
    grid->addWidget(m_useBreath, 5, 7);

    m_useRow5 = new CheckButton("golden-arrow-left", false);
    grid->addWidget(m_useRow5, 5, 8);


    // row 6 buttons
    m_usePedalDown = new CheckButton("notation-pedal-down");
    grid->addWidget(m_usePedalDown, 6, 1);

    m_usePedalUp = new CheckButton("notation-pedal-up");
    grid->addWidget(m_usePedalUp, 6, 2);

    m_useNatural = new CheckButton("accidental-natural");
    grid->addWidget(m_useNatural, 6, 3);

    m_useSharp = new CheckButton("accidental-sharp");
    grid->addWidget(m_useSharp, 6, 4);

    m_useDoubleSharp = new CheckButton("accidental-double-sharp");
    grid->addWidget(m_useDoubleSharp, 6, 5);

    m_useFlat = new CheckButton("accidental-flat");
    grid->addWidget(m_useFlat, 6, 6);

    m_useDoubleFlat = new CheckButton("accidental-double-flat");
    grid->addWidget(m_useDoubleFlat, 6, 7);

    m_useRow6 = new CheckButton("golden-arrow-left", false);
    grid->addWidget(m_useRow6, 6, 8);


    // row 7 buttons
    m_useNoAccidental = new CheckButton("accidental-none");
    grid->addWidget(m_useNoAccidental, 7, 1);

    m_useTextEvents = new CheckButton("text");
    grid->addWidget(m_useTextEvents, 7, 2);

    m_useFingering = new CheckButton("fingering");
    grid->addWidget(m_useFingering, 7, 3);

    m_useTextMark = new CheckButton("text-mark");
    grid->addWidget(m_useTextMark, 7, 4);

    m_useGuitarChord = new CheckButton("guitarchord");
    grid->addWidget(m_useGuitarChord, 7, 5);

    m_useRow7 = new CheckButton("golden-arrow-left", false);
    grid->addWidget(m_useRow7, 7, 8);

    // row 8 buttons
    m_useAllSpecial = new CheckButton("golden-arrow-up", false);
    grid->addWidget(m_useAllSpecial, 8, 8);

    connect(m_useRow1, &QAbstractButton::toggled, this, &SelectDialog::slotUseRow1);
    connect(m_useRow2, &QAbstractButton::toggled, this, &SelectDialog::slotUseRow2);
    connect(m_useRow3, &QAbstractButton::toggled, this, &SelectDialog::slotUseRow3);
    connect(m_useRow4, &QAbstractButton::toggled, this, &SelectDialog::slotUseRow4);
    connect(m_useRow5, &QAbstractButton::toggled, this, &SelectDialog::slotUseRow5);
    connect(m_useRow6, &QAbstractButton::toggled, this, &SelectDialog::slotUseRow6);
    connect(m_useRow7, &QAbstractButton::toggled, this, &SelectDialog::slotUseRow7);
    connect(m_useAllSpecial, &QAbstractButton::toggled, this, &SelectDialog::slotUseAllSpecial);
}

void
SelectDialog::slotUseRow1(bool state)
{
    m_useTenuto->setChecked(state);       
    m_useStaccato->setChecked(state);     
    m_useStaccatissimo->setChecked(state);
    m_useMarcato->setChecked(state);      
    m_useOpen->setChecked(state);         
    m_useStopped->setChecked(state);      
    m_useHarmonic->setChecked(state);     
}


void
SelectDialog::slotUseRow2(bool state)
{
    m_useUpBow->setChecked(state);
    m_useDownBow->setChecked(state);
    m_useOneSlash->setChecked(state);
    m_useTwoSlash->setChecked(state);
    m_useThreeSlash->setChecked(state);
    m_useFourSlash->setChecked(state);
    m_useFiveSlash->setChecked(state);
 }

void
SelectDialog::slotUseRow3(bool state)
{
    m_useSforzando->setChecked(state);
    m_useRinforzando->setChecked(state);
    m_useTrill->setChecked(state);
    m_useTrillLineIndication->setChecked(state);
    m_useTurn->setChecked(state);
    m_useMordent->setChecked(state);
    m_useMordentInverted->setChecked(state);
}

void
SelectDialog::slotUseRow4(bool state)
{
    m_useMordentLong->setChecked(state);
    m_useMordentLongInverted->setChecked(state);
    m_useCrescendo->setChecked(state);
    m_useDecrescendo->setChecked(state);
    m_useSlur->setChecked(state);
    m_usePhrasingSlur->setChecked(state);
    m_usePause->setChecked(state);
}

void
SelectDialog::slotUseRow5(bool state)
{
    m_useQuindicesimaUp->setChecked(state);
    m_useOttavaUp->setChecked(state);
    m_useOttavaDown->setChecked(state);
    m_useQuindicesimaDown->setChecked(state);
    m_useSegno->setChecked(state);
    m_useCoda->setChecked(state);
    m_useBreath->setChecked(state);
}

void
SelectDialog::slotUseRow6(bool state)
{
    m_usePedalDown->setChecked(state);
    m_usePedalUp->setChecked(state);
    m_useNatural->setChecked(state);
    m_useSharp->setChecked(state);
    m_useDoubleSharp->setChecked(state);
    m_useFlat->setChecked(state);
    m_useDoubleFlat->setChecked(state);
}

void
SelectDialog::slotUseRow7(bool state)
{
    m_useNoAccidental->setChecked(state);
    m_useTextEvents->setChecked(state);
    m_useFingering->setChecked(state);
    m_useTextMark->setChecked(state);
    m_useGuitarChord->setChecked(state);
}

void
SelectDialog::slotUseAllSpecial(bool state)
{
    m_useRow1->setChecked(state);
    m_useRow2->setChecked(state);
    m_useRow3->setChecked(state);
    m_useRow4->setChecked(state);
    m_useRow5->setChecked(state);
    m_useRow6->setChecked(state);
    m_useRow7->setChecked(state);
}


// Advanced Tab /////////////////////////////////////////////////////////////////
void
SelectDialog::makeAdvancedTab()
{
    // button grid
    QGridLayout *grid = new QGridLayout();
    m_advancedTab = new QWidget();
    // pad the grid a slight amount horizontally so the labels don't get cut off
    grid->setHorizontalSpacing(15);
    m_advancedTab->setLayout(grid);

    grid->addWidget(new QLabel("I'm the future Advanced tab!  Look at me!"), 1, 1);
}

} // namespace

