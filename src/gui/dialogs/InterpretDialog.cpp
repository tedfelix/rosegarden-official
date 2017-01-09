/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "InterpretDialog.h"
#include <QApplication>

#include "misc/ConfigGroups.h"
#include "commands/notation/InterpretCommand.h"
#include "misc/Strings.h"
#include <QSettings>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QWidget>
#include <QVBoxLayout>
#include <QUrl>
#include <QDesktopServices>


namespace Rosegarden
{

InterpretDialog::InterpretDialog(QWidget *parent) :
        QDialog(parent)
{
    //setHelp("nv-interpret");

    setModal(true);
    setWindowTitle(tr("Interpret"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QGroupBox *vbox = new QGroupBox(tr("Interpretations to apply"), this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    metagrid->addWidget(vbox, 0, 0);

    m_applyTextDynamics = new QCheckBox
                          (tr("Apply text dynamics (p, mf, ff etc)"));
    vboxLayout->addWidget(m_applyTextDynamics);
    m_applyHairpins = new QCheckBox
                      (tr("Apply hairpin dynamics"));
    vboxLayout->addWidget(m_applyHairpins);
    m_stressBeats = new QCheckBox
                    (tr("Stress beats"));
    vboxLayout->addWidget(m_stressBeats);
    m_articulate = new QCheckBox
                   (tr("Articulate slurs, staccato, tenuto etc"));
    vboxLayout->addWidget(m_articulate);
    m_allInterpretations = new QCheckBox
                           (tr("All available interpretations"));
    vboxLayout->addWidget(m_allInterpretations);

    vbox->setLayout(vboxLayout);

    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    m_allInterpretations->setChecked
    ( qStrToBool( settings.value("interpretall", "true" ) ) );
    m_applyTextDynamics->setChecked
    ( qStrToBool( settings.value("interprettextdynamics", "true" ) ) );
    m_applyHairpins->setChecked
    ( qStrToBool( settings.value("interprethairpins", "true" ) ) );
    m_stressBeats->setChecked
    ( qStrToBool( settings.value("interpretstressbeats", "true" ) ) );
    m_articulate->setChecked
    ( qStrToBool( settings.value("interpretarticulate", "true" ) ) );

    connect(m_allInterpretations,
            SIGNAL(clicked()), this, SLOT(slotAllBoxChanged()));

    slotAllBoxChanged();
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(buttonBox, SIGNAL(helpRequested()), this, SLOT(slotHelpRequested()));

    settings.endGroup();
}

void
InterpretDialog::slotAllBoxChanged()
{
    bool all = m_allInterpretations->isChecked();
    m_applyTextDynamics->setEnabled(!all);
    m_applyHairpins->setEnabled(!all);
    m_stressBeats->setEnabled(!all);
    m_articulate->setEnabled(!all);
}

int
InterpretDialog::getInterpretations()
{
    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    settings.setValue("interpretall", m_allInterpretations->isChecked());
    settings.setValue("interprettextdynamics", m_applyTextDynamics->isChecked());
    settings.setValue("interprethairpins", m_applyHairpins->isChecked());
    settings.setValue("interpretstressbeats", m_stressBeats->isChecked());
    settings.setValue("interpretarticulate", m_articulate->isChecked());

    settings.endGroup();

    if (m_allInterpretations->isChecked()) {
        return InterpretCommand::AllInterpretations;
    } else {
        int in = 0;
        if (m_applyTextDynamics->isChecked())
            in |= InterpretCommand::ApplyTextDynamics;
        if (m_applyHairpins->isChecked())
            in |= InterpretCommand::ApplyHairpins;
        if (m_stressBeats->isChecked())
            in |= InterpretCommand::StressBeats;
        if (m_articulate->isChecked()) {
            in |= InterpretCommand::Articulate;
        }
        return in;
    }
}


void
InterpretDialog::slotHelpRequested()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:interpretDialog-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:interpretDialog-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}
}
